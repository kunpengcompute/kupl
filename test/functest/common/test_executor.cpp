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
#include "memory/mpool/kupl_mpool.h"

TEST(test_executor, kupl_get_max_concurrency_without_init)
{
    ASSERT_FALSE(kupl_memory_is_inited());

    int pre = kupl_get_max_concurrency();
    ASSERT_FALSE(kupl_memory_is_inited());
    ASSERT_GT(pre, 0);

    int num = kupl_get_num_executors();
    ASSERT_TRUE(kupl_memory_is_inited());

    ASSERT_EQ(kupl_get_max_concurrency(), num);
    ASSERT_EQ(pre, num);
}

TEST(test_executor, kupl_executor_num)
{
    int num = kupl_get_num_executors();

    int eid = kupl_get_executor_num();
    ASSERT_TRUE(eid == 0);
}

static inline void task_wait_to_set_affinity(kupl_nd_range_t *nd_range, void *args, int tid, int tnum)
{
    usleep(1000);
}

TEST(test_executor, kupl_proc_bind)
{
    kupl_parallel_for_desc_t desc = {
        .field_mask = KUPL_PARALLEL_FOR_DESC_FIELD_DEFAULT,
        .range = nullptr,
        .egroup = nullptr,
        .concurrency = KUPL_CONCURRENCY_DEFAULT,
        .policy = KUPL_LOOP_POLICY_STATIC
    };
    kupl_parallel_for(&desc, task_wait_to_set_affinity, nullptr);
    printf("proc bind spread\n");
    kupl_push_proc_bind(KUPL_PROC_BIND_SPREAD);
    printf("proc bind master\n");
    kupl_push_proc_bind(KUPL_PROC_BIND_MASTER);
    printf("proc bind close\n");
    kupl_push_proc_bind(KUPL_PROC_BIND_CLOSE);
}
