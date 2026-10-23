/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd. All Rights Reserved.
 *
 * KUPL is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "gtest/gtest.h"
#include "kupl.h"

static int a;

void query_and_modify(void *args)
{
    void *hbw = kupl_mem_query(&a);
    ASSERT_TRUE(hbw != nullptr);
    int ret = kupl_hbw_verify(hbw, sizeof(int), 0);
    ASSERT_TRUE(ret == KUPL_IS_HBW_MEMORY);
    ASSERT_TRUE(*(int *)hbw == 1);
    *(int *)hbw = 2;
}

TEST(test_mem, kupl_mem_copyin_copyout)
{
    a = 1;
    auto queue = kupl_queue_acquire(1);
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    kupl_queue_item_desc_t desc {
        .field_mask = KUPL_QUEUE_ITEM_DESC_FIELD_NAME,
        .func = query_and_modify,
        .args = nullptr,
        .name = "query_and_modify",
    };
    ret = kupl_queue_submit(queue, &desc);
    ASSERT_TRUE(ret == KUPL_OK);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_OUT, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    kupl_queue_wait(queue);
    ASSERT_TRUE(a == 2);
}

TEST(test_mem, kupl_mem_query_invalid)
{
    void *hbw = kupl_mem_query(nullptr);
    ASSERT_TRUE(hbw == nullptr);
}

TEST(test_mem, kupl_mem_copyin_copyout_sync)
{
    a = 1;
    auto queue = kupl_queue_acquire(KUPL_ASYNC_SYNC);
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    query_and_modify(nullptr);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_OUT, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    ASSERT_TRUE(a == 2);
}

TEST(test_mem, kupl_mem_create_delete_pull_push)
{
    a = 1;
    auto queue = kupl_queue_acquire(1);
    // create
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_CREATE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    auto hbw = kupl_mem_query(&a);
    ASSERT_TRUE(hbw == nullptr);
    auto present = kupl_mem_is_present(&a);
    ASSERT_TRUE(present);
    // push
    ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_PUSH, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    // query_and_modify
    kupl_queue_item_desc_t desc {
        .field_mask = KUPL_QUEUE_ITEM_DESC_FIELD_NAME,
        .func = query_and_modify,
        .args = nullptr,
        .name = "query_and_modify",
    };
    ret = kupl_queue_submit(queue, &desc);
    ASSERT_TRUE(ret == KUPL_OK);
    // sync
    kupl_queue_wait(queue);
    // pull
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_PULL, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    // delete
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_DELETE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    hbw = kupl_mem_query(&a);
    ASSERT_TRUE(hbw != nullptr);
    present = kupl_mem_is_present(&a);
    ASSERT_TRUE(present == false);
    // sync
    kupl_queue_wait(queue);
    ASSERT_TRUE(a == 2);
}

TEST(test_mem, kupl_mem_create_delete_pull_push_sync)
{
    a = 1;
    auto queue = kupl_queue_acquire(KUPL_ASYNC_SYNC);
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_CREATE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_PUSH, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    query_and_modify(nullptr);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_PULL, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_DELETE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    kupl_queue_wait(queue);
}

TEST(test_mem, kupl_mem_copyin_invalid)
{
    int ret = kupl_mem_copyin(nullptr, sizeof(int), KUPL_MEM_CREATE, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
}

TEST(test_mem, kupl_mem_copyout_invalid)
{
    int ret = kupl_mem_copyout(nullptr, 0, KUPL_MEM_DELETE, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
}

TEST(test_mem, kupl_mem_copy_twice_sync)
{
    a = 1;
    auto queue = kupl_queue_acquire(KUPL_ASYNC_SYNC);
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    a = 2;
    ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    auto hbw = kupl_mem_query(&a);
    ASSERT_TRUE(*(int *)hbw == 1);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_OUT, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    ASSERT_TRUE(a == 2);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_OUT, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    ASSERT_TRUE(a == 1);
    hbw = kupl_mem_query(&a);
    ASSERT_TRUE(hbw == nullptr);
}

TEST(test_mem, kupl_mem_copyout_finalize_sync)
{
    a = 1;
    auto queue = kupl_queue_acquire(KUPL_ASYNC_SYNC);
    int ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    a = 2;
    ret = kupl_mem_copyin(&a, sizeof(int), KUPL_MEM_IN, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    auto hbw = kupl_mem_query(&a);
    ASSERT_TRUE(*(int *)hbw == 1);
    ret = kupl_mem_copyout(&a, sizeof(int), KUPL_MEM_OUT_FINALIZE, queue);
    ASSERT_TRUE(a == 1);
    hbw = kupl_mem_query(&a);
    ASSERT_TRUE(hbw == nullptr);
}

static char range_buf[64];
static int b;
static int c;
static int d;

TEST(test_mem, kupl_mem_copyin_size_zero)
{
    int ret = kupl_mem_copyin(&a, 0, KUPL_MEM_IN, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
}

TEST(test_mem, kupl_mem_is_present_null)
{
    bool present = kupl_mem_is_present(nullptr);
    ASSERT_TRUE(present == false);
}

TEST(test_mem, kupl_mem_query_sub_range)
{
    auto queue = kupl_queue_acquire(KUPL_ASYNC_SYNC);
    int ret = kupl_mem_copyin(range_buf, sizeof(range_buf), KUPL_MEM_CREATE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
    void *hbw = kupl_mem_query(range_buf + 16);
    ASSERT_TRUE(hbw != nullptr);
    ASSERT_TRUE(hbw == (char *)kupl_mem_query(range_buf) + 16);
    ASSERT_TRUE(kupl_mem_query(range_buf + sizeof(range_buf)) == nullptr);
    ASSERT_TRUE(kupl_mem_query(range_buf - 1) == nullptr);
    ret = kupl_mem_copyout(range_buf, sizeof(range_buf), KUPL_MEM_DELETE, queue);
    ASSERT_TRUE(ret == KUPL_OK);
}

TEST(test_mem, kupl_mem_invalid_flag_reuse)
{
    int ret = kupl_mem_copyin(&b, sizeof(b), (kupl_mem_copyin_flag_t)99, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
    ret = kupl_mem_copyin(&b, sizeof(b), KUPL_MEM_IN, nullptr);
    ASSERT_TRUE(ret == KUPL_OK);
    ret = kupl_mem_copyout(&b, sizeof(b), KUPL_MEM_DELETE_FINALIZE, nullptr);
    ASSERT_TRUE(ret == KUPL_OK);
}

TEST(test_mem, kupl_mem_detach_default_flag)
{
    int ret = kupl_mem_copyin(&c, sizeof(c), KUPL_MEM_CREATE, nullptr);
    ASSERT_TRUE(ret == KUPL_OK);
    ret = kupl_mem_copyout(&c, sizeof(c), (kupl_mem_copyout_flag_t)99, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
    ret = kupl_mem_copyout(&c, sizeof(c), KUPL_MEM_DELETE, nullptr);
    ASSERT_TRUE(ret == KUPL_OK);
}

TEST(test_mem, kupl_mem_copyout_unmapped)
{
    int ret = kupl_mem_copyout(&d, sizeof(d), KUPL_MEM_DELETE, nullptr);
    ASSERT_TRUE(ret == KUPL_ERROR);
}
