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
#ifndef KUPL_CORE_H
#define KUPL_CORE_H

#include <pthread.h>
#include <atomic>
#include "utils/sys/kupl_compiler.h"
#include "utils/type/kupl_status.h"

namespace kupl {
/**
 * @brief Lazy initialization state machine shared by all public entry points.
 *
 * The ready flag is published with release semantics and read with acquire semantics, so every entry point
 * either observes a fully initialized runtime or blocks until the initializing thread finishes.
 */
class Init final {
public:
    Init() = delete;
    Init(const Init &) = delete;
    Init &operator=(const Init &) = delete;

    /**
     * @brief Whether the runtime is ready.
     *
     * Hot path, inlined into a single acquire load.
     */
    static kupl_always_inline bool IsReady(void)
    {
        return state_.load(std::memory_order_acquire) == KUPL_INIT_READY;
    }

    /**
     * @brief Ensure the runtime is initialized.
     *
     * Cold path. The first caller performs the initialization, other callers wait for it, and a failed
     * initialization stays failed for the rest of the process.
     *
     * @return KUPL_OK when ready, KUPL_ERROR when the initialization failed or was aborted.
     */
    static int Ensure(void);

    /**
     * @brief Cancel a running initialization and wake up all threads waiting in Ensure().
     *
     * Only used by the failure path of the module teardown, so that worker threads blocked in Ensure()
     * can be joined. It never turns a ready runtime into a failed one.
     */
    static void Cancel(void);

    /**
     * @brief Finalize the runtime and move the state to finished. Idempotent.
     */
    static void DoFini(void);

private:
    enum State : int {
        /* READY is zero so that IsReady() compiles to a plain test-against-zero, same as the old bool. */
        KUPL_INIT_READY = 0,
        KUPL_INIT_UNINITIALIZED,
        KUPL_INIT_INITIALIZING,
        KUPL_INIT_FINISHING,
        KUPL_INIT_FAILED,
        KUPL_INIT_FINISHED,
    };

    static int DoInit(void);

    /*
     * Init-only synchronization. Constant initialized and never destroyed, so neither heap allocation
     * nor static destruction order can affect the initialization path.
     */
    static pthread_mutex_t mutex_;
    static pthread_cond_t cv_;

    static std::atomic<int> state_;
    static thread_local bool owner_;
};
} // namespace kupl

static_assert(std::atomic<int>::is_always_lock_free,
              "kupl::Init::state_ must stay lock free, otherwise the entry guard hits libatomic");

/**
 * @brief Lazy initialization guard shared by all public entry points which return a value.
 *
 * @param [in] _ret         value returned when the initialization failed
 */
#define KUPL_ENSURE_INITED(_ret)                                                           \
    do {                                                                                   \
        if (kupl_unlikely(!kupl::Init::IsReady()) && kupl::Init::Ensure() == KUPL_ERROR) { \
            return _ret;                                                                   \
        }                                                                                  \
    } while (0)

/**
 * @brief Lazy initialization guard shared by all public entry points which return void.
 */
#define KUPL_ENSURE_INITED_VOID()                                                          \
    do {                                                                                   \
        if (kupl_unlikely(!kupl::Init::IsReady()) && kupl::Init::Ensure() == KUPL_ERROR) { \
            return;                                                                        \
        }                                                                                  \
    } while (0)

#endif
