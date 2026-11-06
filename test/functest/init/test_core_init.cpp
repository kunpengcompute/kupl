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
#include <atomic>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "kupl.h"

namespace {
constexpr int KUPL_INIT_TEST_THREADS = 32;
} // namespace

/**
 * This binary only contains the first-initialization tests, so the concurrent wave below really is the
 * first touch of the runtime and the process state is not shared with the other functest binaries.
 *
 * Note that the thread which wins the initialization becomes executor 0, which is a documented property
 * of the runtime, so this test must not assert anything about the executor id of the main thread.
 */
TEST(core_init, concurrent_first_wave)
{
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(KUPL_INIT_TEST_THREADS);

    for (int i = 0; i < KUPL_INIT_TEST_THREADS; ++i) {
        threads.emplace_back([&ready, &go, &failures] {
            ready.fetch_add(1);
            while (!go.load()) {
                std::this_thread::yield();
            }
            double now = kupl_get_wtime();
            if (!(now > 0.0)) {
                failures.fetch_add(1);
            }
        });
    }

    while (ready.load() != KUPL_INIT_TEST_THREADS) {
        std::this_thread::yield();
    }
    go.store(true);
    for (auto &thread : threads) {
        thread.join();
    }
    ASSERT_EQ(failures.load(), 0);
    ASSERT_GT(kupl_get_num_executors(), 0);
}

TEST(core_init, repeated_calls_are_stable)
{
    double first = kupl_get_wtime();
    ASSERT_GT(first, 0.0);
    for (int i = 0; i < 1000; ++i) {
        ASSERT_GT(kupl_get_wtime(), 0.0);
    }
    ASSERT_GE(kupl_get_wtime(), first);
}
