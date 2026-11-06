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

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include "gtest/gtest.h"
#include "kupl.h"

namespace {
/* Exit code telling the parent that the child could not set up the memory pressure. */
constexpr int KUPL_INIT_FAIL_SETUP_ERROR = 77;
/* The address space is capped just above its current size, so the runtime-sized allocations cannot fit. */
constexpr double KUPL_INIT_FAIL_HEADROOM = 1.05;
/* A child which hangs must be killed, otherwise the test would hang the whole suite. */
constexpr unsigned int KUPL_INIT_FAIL_CHILD_TIMEOUT_SEC = 60;

/*
 * Runs in the forked child, so it must not touch gtest. The child shrinks RLIMIT_AS right below the size of
 * the init path allocations: whatever fails, the runtime has to report KUPL_ERROR instead of crashing or
 * walking a released memory pool.
 */
void kupl_init_under_memory_pressure()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_AS, &limit) != 0) {
        _exit(KUPL_INIT_FAIL_SETUP_ERROR);
    }

    FILE *statm = fopen("/proc/self/statm", "r");
    if (statm == nullptr) {
        _exit(KUPL_INIT_FAIL_SETUP_ERROR);
    }
    unsigned long vm_pages = 0;
    unsigned long rss_pages = 0;
    int matched = fscanf(statm, "%lu %lu", &vm_pages, &rss_pages);
    (void)fclose(statm);
    long page_size = sysconf(_SC_PAGESIZE);
    (void)rss_pages;
    if (matched != 2 || vm_pages == 0 || page_size <= 0) {
        _exit(KUPL_INIT_FAIL_SETUP_ERROR);
    }

    limit.rlim_cur = (rlim_t)((double)vm_pages * (double)page_size * KUPL_INIT_FAIL_HEADROOM);
    if (limit.rlim_max != RLIM_INFINITY && limit.rlim_cur > limit.rlim_max) {
        limit.rlim_cur = limit.rlim_max;
    }
    if (setrlimit(RLIMIT_AS, &limit) != 0) {
        /* Without the limit the child would initialize normally and the test would prove nothing. */
        _exit(KUPL_INIT_FAIL_SETUP_ERROR);
    }
    (void)alarm(KUPL_INIT_FAIL_CHILD_TIMEOUT_SEC);

    /* kupl_get_wtime() returns 0.0 when the initialization failed, which is the expected clean failure. */
    double now = kupl_get_wtime();
    _exit(now > 0.0 ? 0 : 1);
}
} // namespace

/**
 * A failed first initialization used to escape the C API as an exception, or crash while releasing the
 * memory pool. Both must end up as a plain KUPL_ERROR in a child process which exits normally.
 */
TEST(core_init_fail, failure_reports_error_instead_of_crashing)
{
    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        kupl_init_under_memory_pressure();
        _exit(KUPL_INIT_FAIL_SETUP_ERROR);
    }

    int status = 0;
    ASSERT_EQ(waitpid(pid, &status, 0), pid);
    ASSERT_FALSE(WIFSIGNALED(status)) << "init failure killed the child with signal "
                                      << (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_NE(WEXITSTATUS(status), KUPL_INIT_FAIL_SETUP_ERROR);
}
