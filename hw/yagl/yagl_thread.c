/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_log.h"
#include "yagl_stats.h"
#include "yagl_transport.h"
#include "yagl_object_map.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"

YAGL_DEFINE_TLS(struct yagl_thread_state*, cur_ts);

#ifdef CONFIG_KVM
static __inline void yagl_cpu_synchronize_state(struct yagl_process_state *ps)
{
    if (kvm_enabled()) {
        memcpy(&((CPUX86State*)current_cpu->env_ptr)->cr[0], &ps->cr[0], sizeof(ps->cr));
    }
}
#else
static __inline void yagl_cpu_synchronize_state(struct yagl_process_state *ps)
{
}
#endif

struct yagl_thread_state
    *yagl_thread_state_create(struct yagl_process_state *ps,
                              yagl_tid id,
                              bool is_first)
{
    int i;

    struct yagl_thread_state *ts =
        g_malloc0(sizeof(struct yagl_thread_state));

    ts->ps = ps;
    ts->id = id;

    cur_ts = ts;

    if (is_first) {
        /*
         * Init APIs (process).
         */

        for (i = 0; i < YAGL_NUM_APIS; ++i) {
            if (ts->ps->ss->apis[i]) {
                ts->ps->api_states[i] = ts->ps->ss->apis[i]->process_init(ts->ps->ss->apis[i]);
                assert(ts->ps->api_states[i]);
            }
        }
    }

    /*
     * Init APIs (thread).
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_init(ts->ps->api_states[i]);
        }
    }

    return ts;
}

void yagl_thread_state_destroy(struct yagl_thread_state *ts,
                               bool is_last)
{
    int i;

    cur_ts = ts;

    /*
     * Fini APIs (thread).
     */

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->thread_fini(ts->ps->api_states[i]);
        }
    }

    if (is_last) {
        /*
         * First, remove all objects.
         */

        yagl_object_map_remove_all(ts->ps->object_map);

        /*
         * Destroy APIs (process).
         */

        for (i = 0; i < YAGL_NUM_APIS; ++i) {
            if (ts->ps->api_states[i]) {
                ts->ps->api_states[i]->destroy(ts->ps->api_states[i]);
                ts->ps->api_states[i] = NULL;
            }
        }
    }

    cur_ts = NULL;

    yagl_thread_set_buffer(ts, NULL);

    g_free(ts);
}

void yagl_thread_set_buffer(struct yagl_thread_state *ts, uint8_t **pages)
{
    if (ts->pages) {
        uint8_t **tmp;

        for (tmp = ts->pages; *tmp; ++tmp) {
            cpu_physical_memory_unmap(*tmp,
                                      TARGET_PAGE_SIZE,
                                      false,
                                      TARGET_PAGE_SIZE);
        }

        g_free(ts->pages);
        ts->pages = NULL;
    }

    ts->pages = pages;
}

void yagl_thread_call(struct yagl_thread_state *ts, uint32_t offset)
{
    int i;
    uint32_t num_calls = 0;
    struct yagl_transport *t = ts->ps->ss->t;

    YAGL_LOG_FUNC_SET(yagl_thread_call);

    assert(current_cpu);

    yagl_cpu_synchronize_state(ts->ps);

    YAGL_LOG_TRACE("batch started");

    cur_ts = ts;

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->batch_start(ts->ps->api_states[i]);
        }
    }

    yagl_transport_begin(t, ts->pages, offset);

    while (true) {
        yagl_api_id api_id;
        yagl_func_id func_id;
        struct yagl_api_ps *api_ps;
        yagl_api_func func;

        yagl_transport_begin_call(t, &api_id, &func_id);

        if (api_id == 0) {
            /*
             * Batch ended.
             */
            break;
        }

        if ((api_id <= 0) || (api_id > YAGL_NUM_APIS)) {
            YAGL_LOG_CRITICAL("target-host protocol error, bad api_id - %u", api_id);
            break;
        }

        api_ps = ts->ps->api_states[api_id - 1];

        if (!api_ps) {
            YAGL_LOG_CRITICAL("uninitialized api - %u. host logic error", api_id);
            break;
        }

        func = api_ps->get_func(api_ps, func_id);

        if (func) {
            if (func(t)) {
                yagl_transport_end_call(t);
            } else {
                /*
                 * Retry is requested.
                 */
                break;
            }
        } else {
            YAGL_LOG_CRITICAL("bad function call (api = %u, func = %u)",
                              api_id,
                              func_id);
            break;
        }

        ++num_calls;
    }

    YAGL_LOG_TRACE("batch ended: %u calls", num_calls);

    yagl_stats_batch(num_calls, yagl_transport_bytes_processed(t));

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ts->ps->api_states[i]) {
            ts->ps->api_states[i]->batch_end(ts->ps->api_states[i]);
        }
    }
}
