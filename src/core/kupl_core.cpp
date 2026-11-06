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
#include "kupl_core.h"
#include <atomic>
#include <cstdio>
#include <exception>
#include "kupl.h"
#include "utils/debug/kupl_assert.h"
#include "dm/memcpy/kupl_memcpy.h"
#include "memory/mpool/kupl_mpool.h"
#include "memory/hbw/kupl_hbw.h"
#include "memory/mem/kupl_mem.h"
#include "executor/kupl_executor.h"
#include "executor/kupl_places.h"
#include "mt/scheduler/kupl_sched.h"
#include "mt/kupl_graph.h"
#include "mt/kupl_queue.h"
#include "mt/kupl_parallel_for.h"
#include "memory/shm/kupl_shm.h"
#include "utils/kupl_utils.h"
#include "tools/kupl_tools.h"
#include "tools/profile/kupl_profile.h"
#include "tools/profile/kupl_profile_trace.h"

namespace kupl {
pthread_mutex_t Init::mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Init::cv_ = PTHREAD_COND_INITIALIZER;
std::atomic<int> Init::state_{Init::KUPL_INIT_UNINITIALIZED};
thread_local bool Init::owner_ = false;
} // namespace kupl

static const kupl_version_t g_version = {
    .product_name = "Kunpeng HPCKit",
    .product_version = "26.2.0",
    .component_name = "KUPL",
    .component_abi_version = "1",
    .component_version = "26.2.0",
#if defined(__clang__)
    .component_appendinfo = "bisheng",
#elif defined(__GNUC__)
    .component_appendinfo = "gcc",
#endif
};

int kupl_get_version(kupl_version_t *version)
{
    if (version == nullptr) {
        return KUPL_ERROR;
    }
    *version = g_version;
    return KUPL_OK;
}

static kupl_always_inline int kupl_dm_module_init()
{
    if (kupl_unlikely(kupl_memcpy_init() != KUPL_OK)) {
        return KUPL_ERROR;
    }
    return KUPL_OK;
}

static kupl_always_inline void kupl_dm_module_fini()
{
    kupl_memcpy_fini();
}

static kupl_always_inline int kupl_memory_module_init()
{
    if (!g_mpool_inited && kupl_mpool_init() == KUPL_ERROR) {
        goto err_mpool_init;
    }

    if (kupl_unlikely(kupl_mem_init() != KUPL_OK)) {
        goto err_mem_init;
    }

    return KUPL_OK;

err_mem_init:
    kupl_mpool_fini();
err_mpool_init:
    return KUPL_ERROR;
}

static kupl_always_inline void kupl_memory_module_fini()
{
    kupl_mem_fini();
    kupl_mpool_fini();
}

static kupl_always_inline int kupl_mt_module_init()
{
    if (kupl_unlikely(kupl_sched_init() != KUPL_OK)) {
        goto err_sched_init;
    }

    if (kupl_unlikely(kupl_queue_init() != KUPL_OK)) {
        goto err_queue_init;
    }

    return KUPL_OK;

err_queue_init:
    kupl_sched_fini();
err_sched_init:
    return KUPL_ERROR;
}

static kupl_always_inline void kupl_mt_module_fini()
{
    kupl_sched_fini();
    kupl_queue_fini();
}

static kupl_always_inline int kupl_executor_module_init()
{
    if (kupl_unlikely(kupl_executor_init() != KUPL_OK)) {
        goto err_executor_init;
    }

    if (kupl_unlikely(kupl_executor_start() != KUPL_OK)) {
        goto err_executor_start;
    }
    kupl_barrier_init();

    return KUPL_OK;

err_executor_start:
    kupl_executor_fini();
err_executor_init:
    return KUPL_ERROR;
}

static kupl_always_inline void kupl_executor_module_fini()
{
    kupl_executor_stop();
    kupl_executor_fini();
}

int kupl::Init::DoInit()
{
    /*
     * Keep this function free of throwing operations after the executor step: kupl::Init::Ensure() converts an
     * escaped exception into KUPL_ERROR but rolls nothing back, so worker threads started by
     * kupl_executor_module_init() would be leaked if an exception were thrown after that point.
     */
    if (kupl_unlikely(!g_utils_inited && kupl_utils_init() != KUPL_OK)) {
        kupl_error("Initialize utils module failed");
        goto err_utils_init;
    }

    if (kupl_unlikely(kupl_places_init() != KUPL_OK)) {
        goto err_places_init;
    }

    if (kupl_unlikely(!g_tools_inited && kupl_tools_init() != KUPL_OK)) {
        kupl_error("Initialize tools module failed");
        goto err_tools_init;
    }

    if (kupl_unlikely(kupl_sdma_module_init() != KUPL_OK)) {
        kupl_error("Initialize sdma module failed");
        goto err_sdma_init;
    }

    if (kupl_unlikely(kupl_memory_module_init() != KUPL_OK)) {
        kupl_error("Initialize memory module failed");
        goto err_memory_init;
    }

    if (kupl_unlikely(kupl_dm_module_init() != KUPL_OK)) {
        kupl_error("Initialize dm module failed");
        goto err_dm_init;
    }

    if (kupl_unlikely(kupl_mt_module_init() != KUPL_OK)) {
        kupl_error("Initialize mt module failed");
        goto err_mt_init;
    }

    if (kupl_unlikely(kupl_executor_module_init() != KUPL_OK)) {
        kupl_error("Initialize executor module failed");
        goto err_executor_init;
    }

    if (kupl_unlikely(kupl_pf_init() != KUPL_OK)) {
        kupl_error("Initialize parallel for failed");
        goto err_pf_init;
    }

    return KUPL_OK;

err_pf_init:
    kupl_executor_module_fini();
err_executor_init:
    kupl_mt_module_fini();
err_mt_init:
    kupl_dm_module_fini();
err_dm_init:
    kupl_memory_module_fini();
err_memory_init:
    kupl_sdma_module_fini();
err_sdma_init:
    kupl_tools_fini();
err_tools_init:
    kupl_places_fini();
err_places_init:
    kupl_utils_fini();
err_utils_init:
    return KUPL_ERROR;
}

int kupl::Init::Ensure()
{
    for (;;) {
        int state = state_.load(std::memory_order_acquire);
        if (state == KUPL_INIT_READY) {
            return KUPL_OK;
        }
        if (state == KUPL_INIT_FAILED || state == KUPL_INIT_FINISHING || state == KUPL_INIT_FINISHED) {
            return KUPL_ERROR;
        }
        if (state == KUPL_INIT_UNINITIALIZED) {
            int expected = KUPL_INIT_UNINITIALIZED;
            if (state_.compare_exchange_strong(expected, KUPL_INIT_INITIALIZING, std::memory_order_acq_rel,
                                               std::memory_order_acquire)) {
                owner_ = true;
                break;
            }
            continue;
        }

        /* Another thread is running DoInit(), wait until it is done. */
        if (kupl_unlikely(owner_)) {
            kupl_assertv(false, "recursive kupl::Init::Ensure detected");
            return KUPL_ERROR;
        }
        (void)pthread_mutex_lock(&mutex_);
        while (state_.load(std::memory_order_acquire) == KUPL_INIT_INITIALIZING) {
            (void)pthread_cond_wait(&cv_, &mutex_);
        }
        (void)pthread_mutex_unlock(&mutex_);
    }

    /*
     * DoInit() must not throw: an escaped exception would leave state_ in KUPL_INIT_INITIALIZING, so every
     * other thread would wait forever in the loop above and there would be no way to recover. This is the
     * only exception boundary of the runtime; it converts std::bad_alloc and friends into the usual
     * KUPL_ERROR so the state machine still reaches KUPL_INIT_FAILED and the waiters are woken up.
     */
    int ret = KUPL_ERROR;
    try {
        ret = DoInit();
    } catch (const std::exception &e) {
        kupl_error("Initialize kupl runtime failed: %s", e.what());
    } catch (...) {
        kupl_error("Initialize kupl runtime failed: unknown exception");
    }

    (void)pthread_mutex_lock(&mutex_);
    state_.store(ret == KUPL_OK ? KUPL_INIT_READY : KUPL_INIT_FAILED, std::memory_order_release);
    (void)pthread_cond_broadcast(&cv_);
    (void)pthread_mutex_unlock(&mutex_);

    owner_ = false;
    return ret;
}

void kupl::Init::Cancel()
{
    (void)pthread_mutex_lock(&mutex_);
    int expected = KUPL_INIT_INITIALIZING;
    (void)state_.compare_exchange_strong(expected, KUPL_INIT_FAILED, std::memory_order_acq_rel,
                                         std::memory_order_acquire);
    (void)pthread_cond_broadcast(&cv_);
    (void)pthread_mutex_unlock(&mutex_);
}

void kupl::Init::DoFini()
{
    int expected = KUPL_INIT_READY;
    if (!state_.compare_exchange_strong(expected, KUPL_INIT_FINISHING, std::memory_order_acq_rel,
                                        std::memory_order_acquire)) {
        return;
    }

    kupl_global_graph_destroy();
    /* Stop and join the executor threads before releasing the parallel-for and scheduler data they use. */
    kupl_executor_module_fini();
    kupl_pf_fini();
    kupl_mt_module_fini();
    if (g_mpool_inited) {
        kupl_mpool_fini();
    }
    kupl_dm_module_fini();
    kupl_memory_module_fini();
    kupl_sdma_module_fini();
    if (g_tools_inited) {
        kupl_tools_fini();
    }
    kupl_places_fini();
    if (g_utils_inited) {
        kupl_utils_fini();
    }
    state_.store(KUPL_INIT_FINISHED, std::memory_order_release);
}

__attribute__((destructor)) void kupl_fini()
{
    kupl::Init::DoFini();
}
