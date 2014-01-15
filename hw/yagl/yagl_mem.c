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

#include "yagl_mem.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_log.h"
#include "exec/cpu-all.h"

bool yagl_mem_put(target_ulong va, const void* data, uint32_t len)
{
    int ret;

    YAGL_LOG_FUNC_ENTER(yagl_mem_put, "va = 0x%X, len = %u", (uint32_t)va, len);

    ret = cpu_memory_rw_debug(current_cpu, va, (void*)data, len, 1);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va, len);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return ret != -1;
}

bool yagl_mem_get(target_ulong va, uint32_t len, void* data)
{
    int ret;

    YAGL_LOG_FUNC_ENTER(yagl_mem_get, "va = 0x%X, len = %u", (uint32_t)va, len);

    ret = cpu_memory_rw_debug(current_cpu, va, data, len, 0);

    if (ret == -1) {
        YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)va, len);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return ret != -1;
}
