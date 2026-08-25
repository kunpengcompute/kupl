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