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
#include "kupl_places.h"
#include <numa.h>
#include "utils/debug/kupl_assert.h"
#include "executor/kupl_executor.h"
#include "core/kupl_core.h"

#define KUPL_PLACES_FILE_LENGTH 256
#define KUPL_PLACES_BUFFER_LENGTH 16
int kupl_enable_display_affinity = 0;
cpu_set_t full_mask;
cpu_set_t *g_places = nullptr;
int g_num_places = 0;
kupl_places_proc_bind_t g_proc_bind = KUPL_PLACES_PROC_BIND_TRUE;

static void kupl_set_cores_places(int place_num)
{
    auto host_info = kupl_get_host_info();
    int core_num = host_info->avail_pu_cnt;
    core_num = core_num > place_num ? place_num : core_num;
    g_num_places = core_num;
    int current_num = 0;
    for (int i = 0; i < host_info->pu_conf && current_num < core_num; ++i) {
        if (CPU_ISSET(i, &host_info->avail_set)) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(i, &cpuset);
            g_places[current_num] = cpuset;
            current_num++;
        }
    }
}

static int kupl_get_socket_id(int core_id)
{
    char path[KUPL_PLACES_FILE_LENGTH];
    char buffer[KUPL_PLACES_BUFFER_LENGTH];
    FILE *fp;
    sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", core_id);
    fp = fopen(path, "r");
    if (fp == nullptr) {
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), fp) == nullptr) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return atoi(buffer);
}

static int kupl_set_sockets_places(int place_num)
{
    auto host_info = kupl_get_host_info();
    int current_num = 0;
    int current_socket_id = -1;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < host_info->pu_conf; ++i) {
        if (current_num >= place_num) {
            g_num_places = current_num;
            return KUPL_OK;
        }
        if (CPU_ISSET(i, &host_info->avail_set)) {
            int socket_id = kupl_get_socket_id(i);
            if (kupl_unlikely(socket_id == -1)) {
                kupl_error("cannot get socket id of cpu %d", i);
                return KUPL_ERROR;
            }
            if (current_socket_id == -1) {
                current_socket_id = socket_id;
            }
            if (socket_id != current_socket_id) {
                g_places[current_num] = cpuset;
                CPU_ZERO(&cpuset);
                current_num++;
                current_socket_id = socket_id;
            }
            CPU_SET(i, &cpuset);
        }
    }
    g_places[current_num] = cpuset;
    g_num_places = current_num + 1;
    return KUPL_OK;
}

static void kupl_set_numa_places(int place_num)
{
    auto host_info = kupl_get_host_info();
    int current_num = 0;
    int current_numa_id = -1;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < host_info->pu_conf; ++i) {
        if (current_num >= place_num) {
            g_num_places = current_num;
            return;
        }
        if (CPU_ISSET(i, &host_info->avail_set)) {
            int cpu_numa_id = numa_node_of_cpu(i);
            if (current_numa_id == -1) {
                current_numa_id = cpu_numa_id;
            }
            if (cpu_numa_id != current_numa_id) {
                g_places[current_num] = cpuset;
                CPU_ZERO(&cpuset);
                current_num++;
                current_numa_id = cpu_numa_id;
            }
            CPU_SET(i, &cpuset);
        }
    }
    g_places[current_num] = cpuset;
    g_num_places = current_num + 1;
}

static int kupl_process_subplace_list(const char **scan, cpu_set_t *current_mask, int *set_size)
{
    const char *next;

    for (;;) {
        int start, count, i, stride;

        // Read in the starting proc id
        KUPL_SKIP_WS(*scan);
        if (**scan < '0' || **scan > '9') {
            return KUPL_ERROR;
        }
        next = *scan;
        KUPL_SKIP_DIGITS(next);
        start = kupl_str_to_int(*scan);
        if (start < 0) {
            return KUPL_ERROR;
        }
        *scan = next;

        // valid follow sets are ',' ':' and '}'
        KUPL_SKIP_WS(*scan);
        if (**scan == '}' || **scan == ',') {
            if ((start >= CPU_SETSIZE) || (!CPU_ISSET(start, &full_mask))) {
                kupl_warn("invalid os proc id %d", start);
            } else {
                CPU_SET(start, current_mask);
                (*set_size)++;
            }
            if (**scan == '}') {
                break;
            }
            (*scan)++; // skip ','
            continue;
        }
        if (**scan != ':') {
            return KUPL_ERROR;
        }
        (*scan)++; // skip ':'

        // Read count parameter
        KUPL_SKIP_WS(*scan);
        if (**scan < '0' || **scan > '9') {
            return KUPL_ERROR;
        }
        next = *scan;
        KUPL_SKIP_DIGITS(next);
        count = kupl_str_to_int(*scan);
        if (count < 0) {
            return KUPL_ERROR;
        }
        *scan = next;

        // valid follow sets are ',' ':' and '}'
        KUPL_SKIP_WS(*scan);
        if (**scan == '}' || **scan == ',') {
            for (i = 0; i < count; i++) {
                if ((start >= CPU_SETSIZE) || (!CPU_ISSET(start, &full_mask))) {
                    kupl_warn("invalid os proc id %d", start);
                } else {
                    CPU_SET(start, current_mask);
                    (*set_size)++;
                }
                start++;
            }
            if (**scan == '}') {
                break;
            }
            (*scan)++; // skip ','
            continue;
        }
        if (**scan != ':') {
            return KUPL_ERROR;
        }
        (*scan)++; // skip ':'

        // Read stride parameter
        int sign = +1;
        for (;;) {
            KUPL_SKIP_WS(*scan);
            if (**scan == '+') {
                (*scan)++; // skip '+'
                continue;
            }
            if (**scan == '-') {
                sign *= -1;
                (*scan)++; // skip '-'
                continue;
            }
            break;
        }
        KUPL_SKIP_WS(*scan);
        if (**scan < '0' || **scan > '9') {
            return KUPL_ERROR;
        }
        next = *scan;
        KUPL_SKIP_DIGITS(next);
        stride = kupl_str_to_int(*scan);
        if (stride < 0) {
            return KUPL_ERROR;
        }
        *scan = next;
        stride *= sign;

        // valid follow sets are ',' and '}'
        KUPL_SKIP_WS(*scan);
        if (**scan == '}' || **scan == ',') {
            for (i = 0; i < count; i++) {
                if ((start >= CPU_SETSIZE) || (!CPU_ISSET(start, &full_mask))) {
                    kupl_warn("invalid os proc id %d", start);
                } else {
                    CPU_SET(start, current_mask);
                    (*set_size)++;
                    start += stride;
                }
            }
            if (**scan == '}') {
                break;
            }
            (*scan)++; // skip ','
            continue;
        }
        return KUPL_ERROR;
    }
    return KUPL_OK;
}

static int kupl_process_place(const char **scan, cpu_set_t *current_mask, int *set_size)
{
    const char *next;

    // valid follow sets are '{' '!' and num
    KUPL_SKIP_WS(*scan);
    if (**scan == '{') {
        (*scan)++; // skip '{'
        if (kupl_process_subplace_list(scan, current_mask, set_size) == KUPL_ERROR) {
            return KUPL_ERROR;
        }
        if (**scan != '}') {
            return KUPL_ERROR;
        }
        (*scan)++; // skip '}'
    } else if (**scan == '!') {
        (*scan)++; // skip '!'
        if (kupl_process_place(scan, current_mask, set_size) == KUPL_ERROR) {
            return KUPL_ERROR;
        }
        CPU_XOR(current_mask, current_mask, &full_mask);
        CPU_AND(current_mask, current_mask, &full_mask);
        *set_size = CPU_COUNT(current_mask);
    } else if ((**scan >= '0') && (**scan <= '9')) {
        next = *scan;
        KUPL_SKIP_DIGITS(next);
        int num = kupl_str_to_int(*scan);
        if (num < 0) {
            return KUPL_ERROR;
        }
        if ((num >= CPU_SETSIZE) || (!CPU_ISSET(num, &full_mask))) {
            kupl_warn("invalid os proc id %d", num);
        } else {
            CPU_SET(num, current_mask);
            (*set_size)++;
        }
        *scan = next; // skip num
    } else {
        return KUPL_ERROR;
    }
    return KUPL_OK;
}

/*-----------------------------------------------------------------------------
place_list := place
place_list := place , place_list
place := num
place := place : num
place := place : num : signed
place := { subplacelist }
place := ! place
subplace_list := subplace
subplace_list := subplace , subplace_list
subplace := num
subplace := num : num
subplace := num : num : signed
signed := num
signed := + signed
signed := - signed
-----------------------------------------------------------------------------*/
static int kupl_process_placelist(char *placelist)
{
    int i, j, count, stride, sign;
    const char *scan = placelist;
    const char *next = placelist;

    g_num_places = 0;
    cpu_set_t current_mask;
    cpu_set_t pre_mask;
    CPU_ZERO(&current_mask);
    CPU_ZERO(&pre_mask);
    int set_size = 0;

    for (;;) {
        if (kupl_process_place(&scan, &current_mask, &set_size) == KUPL_ERROR) {
            return KUPL_ERROR;
        }

        // valid follow sets are ',' ':' and EOL
        KUPL_SKIP_WS(scan);
        if (*scan == '\0' || *scan == ',') {
            if (set_size > 0) {
                if (kupl_unlikely(g_num_places >= CPU_SETSIZE)) {
                    return KUPL_ERROR;
                }
                g_places[g_num_places] = current_mask;
                g_num_places++;
            }
            CPU_ZERO(&current_mask);
            set_size = 0;
            if (*scan == '\0') {
                break;
            }
            scan++; // skip ','
            continue;
        }

        if (*scan != ':') {
            return KUPL_ERROR;
        }

        scan++; // skip ':'

        // Read count parameter
        KUPL_SKIP_WS(scan);
        if (*scan < '0' || *scan > '9') {
            return KUPL_ERROR;
        }
        next = scan;
        KUPL_SKIP_DIGITS(next);
        count = kupl_str_to_int(scan);
        if (count < 0) {
            return KUPL_ERROR;
        }
        scan = next;

        // valid follow sets are ',' ':' and EOL
        KUPL_SKIP_WS(scan);
        if (*scan == '\0' || *scan == ',') {
            stride = 1;
        } else {
            if (*scan != ':') {
                return KUPL_ERROR;
            }
            scan++; // skip ':'

            // Read stride parameter
            sign = 1;
            for (;;) {
                KUPL_SKIP_WS(scan);
                if (*scan == '+') {
                    scan++; // skip '+'
                    continue;
                }
                if (*scan == '-') {
                    sign *= -1;
                    scan++; // skip '-'
                    continue;
                }
                break;
            }
            KUPL_SKIP_WS(scan);
            if (*scan < '0' || *scan > '9') {
                return KUPL_ERROR;
            }
            next = scan;
            KUPL_SKIP_DIGITS(next);
            stride = kupl_str_to_int(scan);
            scan = next;
            stride *= sign;
        }

        // Add places determined by initial_place : count : stride
        for (i = 0; i < count; i++) {
            if (set_size == 0) {
                break;
            }
            // Add the current place, then build the next place (current_mask) from that
            pre_mask = current_mask;
            if (kupl_unlikely(g_num_places >= CPU_SETSIZE)) {
                return KUPL_ERROR;
            }
            g_places[g_num_places] = pre_mask;
            g_num_places++;
            CPU_ZERO(&current_mask);
            set_size = 0;
            for (j = 0; j < CPU_SETSIZE; j++) {
                if (!CPU_ISSET(j, &pre_mask)) {
                    continue;
                }
                if ((j + stride >= CPU_SETSIZE) || (j + stride < 0) || (!CPU_ISSET(j + stride, &full_mask))) {
                    kupl_warn("invalid os proc id %d", j + stride);
                    continue;
                }
                CPU_SET(j + stride, &current_mask);
                set_size++;
            }
        }
        CPU_ZERO(&current_mask);
        set_size = 0;

        // valid follow sets are ',' and EOL
        KUPL_SKIP_WS(scan);
        if (*scan == '\0') {
            break;
        }
        if (*scan == ',') {
            scan++; // skip ','
            continue;
        }
        return KUPL_ERROR;
    }
    return KUPL_OK;
}

void kupl_process_places_affinity()
{
    char *placelist = strdup(kupl_config_get_value_str(KUPL_PLACES));
    if (kupl_unlikely(placelist == nullptr)) {
        kupl_warn("cannot get KUPL_PLACES, using cores");
        kupl_set_cores_places(CPU_SETSIZE);
        return;
    }
    char *scan = placelist;
    char *next = scan;
    int match_place_type = 0;

    const char *place_type[] = {"cores", "sockets", "numa_domains"};
    for (size_t i = 0; i < sizeof(place_type) / sizeof(place_type[0]); i++) {
        if (kupl_match_string(place_type[i], scan, &next)) {
            match_place_type = 1;
            int place_num = CPU_SETSIZE;
            scan = next;
            if (*scan == '(') {
                scan++;
                place_num = kupl_str_to_int(scan);
                KUPL_SKIP_DIGITS(scan);
                if (place_num == 0 || *scan != ')') {
                    goto err;
                }
            } else if (*scan != '\0') {
                goto err;
            }
            if (i == 0) {
                kupl_set_cores_places(place_num);
            } else if (i == 1) {
                if (kupl_set_sockets_places(place_num) == KUPL_ERROR) {
                    goto err;
                }
            } else {
                kupl_set_numa_places(place_num);
            }
        }
    }

    if (match_place_type == 0) {
        if (kupl_process_placelist(placelist) == KUPL_ERROR) {
            goto err;
        }
    }
    free(placelist);
    return;
err:
    free(placelist);
    g_num_places = 0;
    kupl_warn("invalid KUPL_PLACES, using cores");
    kupl_set_cores_places(CPU_SETSIZE);
}

int kupl_places_init()
{
    kupl_enable_display_affinity = kupl_config_get_value(KUPL_DISPLAY_AFFINITY);

    g_places = (cpu_set_t *)kupl_calloc(static_cast<size_t>(CPU_SETSIZE), sizeof(cpu_set_t));
    if (kupl_unlikely(g_places == nullptr)) {
        return KUPL_ERROR;
    }

    const kupl_host_info_t *info = kupl_get_host_info();

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int i = 0; i < info->pu_conf; ++i) {
        if (!CPU_ISSET(i, &info->avail_set)) {
            continue;
        }
        CPU_SET(i, &cpuset);
    }

    full_mask = cpuset;
    kupl_process_places_affinity();
    kupl_executor_count_init();
    if (kupl_unlikely(g_num_places == 0)) {
        kupl_places_fini();
        return kupl_log_error_return(ERROR, "kupl places init failed");
    }

    std::string proc_bind_policy[KUPL_PROC_BIND_POLICY_NUM] = {"master", "close", "spread", "false", "true"};
    std::string proc_bind = kupl_config_get_value_str(KUPL_PROC_BIND);
    for (int i = 0; i < KUPL_PROC_BIND_POLICY_NUM; i++) {
        if (proc_bind == proc_bind_policy[i]) {
            g_proc_bind = (kupl_places_proc_bind_t)i;
            break;
        }
    }

    return KUPL_OK;
}

void kupl_places_fini()
{
    if (g_places == nullptr) {
        return;
    }
    kupl_safe_free(g_places);
}

static void kupl_set_insufficient_places()
{
    int places_per_exe = g_real_executor_count / g_num_places;
    int places_count = 0;
    int rem_places = g_real_executor_count - (places_per_exe * g_num_places);
    int place_id = 0;
    if (rem_places == 0) {
        for (int i = 0; i < g_real_executor_count; ++i) {
            kupl_executor_set_place_id(place_id, i);
            places_count++;
            if (places_count == places_per_exe) {
                place_id++;
                places_count = 0;
            }
        }
        return;
    }
    int gap = g_num_places / rem_places;
    int gap_count = gap;
    for (int i = 0; i < g_real_executor_count; ++i) {
        kupl_executor_set_place_id(place_id, i);
        places_count++;
        if ((places_count == places_per_exe) && rem_places && (gap_count == gap)) {
        } else if ((places_count == places_per_exe + 1) && rem_places && (gap_count == gap)) {
            place_id++;
            places_count = 0;
            gap_count = 1;
            rem_places--;
        } else if (places_count == places_per_exe) {
            place_id++;
            gap_count++;
            places_count = 0;
        }
    }
}

void kupl_set_proc_bind(kupl_places_proc_bind_t proc_bind)
{
    switch (proc_bind) {
        case KUPL_PLACES_PROC_BIND_FALSE:
            for (int i = 0; i < g_real_executor_count; ++i) {
                kupl_executor_set_place_id(KUPL_PLACES_DEFAULT, i);
            }
            break;
        case KUPL_PLACES_PROC_BIND_MASTER:
            for (int i = 0; i < g_real_executor_count; ++i) {
                kupl_executor_set_place_id(0, i);
            }
            break;
        case KUPL_PLACES_PROC_BIND_CLOSE:
            if (g_num_places >= g_real_executor_count) {
                for (int i = 0; i < g_real_executor_count; ++i) {
                    kupl_executor_set_place_id(i, i);
                }
            } else {
                kupl_set_insufficient_places();
            }
            break;
        case KUPL_PLACES_PROC_BIND_TRUE:
        case KUPL_PLACES_PROC_BIND_SPREAD:
            if (g_num_places >= g_real_executor_count) {
                double current = 0;
                double spacing = (double)(g_num_places + 1) / (double)g_real_executor_count;
                for (int i = 0; i < g_real_executor_count; ++i) {
                    int first = (int)current;
                    int last = (int)(current + spacing) - 1;
                    if (last >= g_num_places) {
                        last = g_num_places - 1;
                    }
                    kupl_executor_set_place_id(first, i);
                    current += spacing;
                }

            } else {
                kupl_set_insufficient_places();
            }
            break;
        default:
            break;
    }
}

static inline void task_set_executor_affinity(kupl_nd_range_t *nd_range, void *args, int tid, int tnum)
{
    (void)nd_range;
    (void)args;
    (void)tid;
    (void)tnum;
    int geid = kupl_get_executor_num();
    int place_id = kupl_executor_get_place_id(geid);
    kupl_set_affinity(place_id);
}

void kupl_push_proc_bind(kupl_proc_bind_t proc_bind)
{
    if (!g_core_inited && kupl_init() == KUPL_ERROR) {
        return;
    }

    if (g_proc_bind == KUPL_PLACES_PROC_BIND_FALSE) {
        return;
    }
    kupl_set_proc_bind((kupl_places_proc_bind_t)proc_bind);
    kupl_parallel_for_desc_t desc = {.field_mask = KUPL_PARALLEL_FOR_DESC_FIELD_DEFAULT,
                                     .range = nullptr,
                                     .egroup = nullptr,
                                     .concurrency = KUPL_CONCURRENCY_DEFAULT,
                                     .policy = KUPL_LOOP_POLICY_STATIC};
    kupl_parallel_for(&desc, task_set_executor_affinity, nullptr);
}
