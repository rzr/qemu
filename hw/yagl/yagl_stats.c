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

#include "yagl_stats.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

#ifdef CONFIG_YAGL_STATS

#define YAGL_STATS_MAX_BATCHES 100

static uint32_t g_num_refs = 0;

static uint32_t g_num_batches = 0;

static uint32_t g_num_calls_min = UINT32_MAX;
static uint32_t g_num_calls_max = 0;

static uint32_t g_bytes_used_min = UINT32_MAX;
static uint32_t g_bytes_used_max = 0;

void yagl_stats_new_ref(void)
{
    ++g_num_refs;
}

void yagl_stats_delete_ref(void)
{
    if (g_num_refs-- == 0) {
        assert(0);
    }
}

void yagl_stats_batch(uint32_t num_calls, uint32_t bytes_used)
{
    if (num_calls > g_num_calls_max) {
        g_num_calls_max = num_calls;
    }

    if (bytes_used > g_bytes_used_max) {
        g_bytes_used_max = bytes_used;
    }

    if (num_calls < g_num_calls_min) {
        g_num_calls_min = num_calls;
    }

    if (bytes_used < g_bytes_used_min) {
        g_bytes_used_min = bytes_used;
    }

    if (++g_num_batches >= YAGL_STATS_MAX_BATCHES) {
        yagl_stats_dump();

        g_num_batches = 0;
        g_num_calls_min = UINT32_MAX;
        g_num_calls_max = 0;
        g_bytes_used_min = UINT32_MAX;
        g_bytes_used_max = 0;
    }
}

void yagl_stats_dump(void)
{
    YAGL_LOG_FUNC_ENTER(yagl_stats_dump, NULL);

    YAGL_LOG_DEBUG("<<STATS");
    YAGL_LOG_DEBUG("# yagl_ref's: %u", g_num_refs);
    YAGL_LOG_DEBUG("# of calls per batch: %u - %u",
                   ((g_num_calls_min == UINT32_MAX) ? 0 : g_num_calls_min),
                   g_num_calls_max);
    YAGL_LOG_DEBUG("# of bytes used per batch: %u - %u",
                   ((g_bytes_used_min == UINT32_MAX) ? 0 : g_bytes_used_min),
                   g_bytes_used_max);
    YAGL_LOG_DEBUG(">>STATS");

    YAGL_LOG_FUNC_EXIT(NULL);
}

#endif
