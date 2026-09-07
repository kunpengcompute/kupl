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
#ifndef KUPL_PLACES_H
#define KUPL_PLACES_H

#include <climits>
#include <sched.h>
#include "kupl.h"
#include "utils/sys/kupl_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KUPL_PLACES_DEFAULT (-1)
#define KUPL_PROC_BIND_POLICY_NUM 5

#define KUPL_SKIP_WS(_x)                      \
    {                                         \
        while (*(_x) == ' ' || *(_x) == '\t') \
            (_x)++;                           \
    }

#define KUPL_SKIP_DIGITS(_x)                 \
    {                                        \
        while (*(_x) >= '0' && *(_x) <= '9') \
            (_x)++;                          \
    }

typedef enum kupl_places_proc_bind {
    KUPL_PLACES_PROC_BIND_MASTER,
    KUPL_PLACES_PROC_BIND_CLOSE,
    KUPL_PLACES_PROC_BIND_SPREAD,
    KUPL_PLACES_PROC_BIND_FALSE,
    KUPL_PLACES_PROC_BIND_TRUE
} kupl_places_proc_bind_t;

extern cpu_set_t full_mask;
extern int g_num_places;
extern cpu_set_t *g_places;
extern int kupl_enable_display_affinity;
extern kupl_places_proc_bind_t g_proc_bind;

static kupl_always_inline int kupl_str_to_int(char const *str)
{
    int result;
    char const *t;
    result = 0;
    for (t = str; *t != '\0'; ++t) {
        if (*t < '0' || *t > '9') {
            break;
        }
        if (result >= (INT_MAX - (*t - '0')) / 10) {
            result = INT_MAX;
            break;
        }
        result = (result * 10) + (*t - '0');
    }

    return result;
}

static kupl_always_inline int kupl_match_string(const char *token, char *buf, char **end)
{
    while (*token && *buf) {
        char ct = *token, cb = *buf;
        if (ct >= 'a' && ct <= 'z') {
            ct -= 'a' - 'A';
        }
        if (cb >= 'a' && cb <= 'z') {
            cb -= 'a' - 'A';
        }
        if (ct != cb) {
            return 0;
        }
        ++token;
        ++buf;
    }
    if (*token) {
        return 0;
    }
    *end = buf;
    return 1;
}

void kupl_process_places_affinity();

void kupl_set_proc_bind(kupl_places_proc_bind_t proc_bind);

int kupl_places_init();

void kupl_places_fini();

#ifdef __cplusplus
}
#endif

#endif
