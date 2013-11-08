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

struct yagl_mem_transfer *yagl_mem_transfer_create(void)
{
    struct yagl_mem_transfer *mt;

    mt = g_malloc0(sizeof(*mt));

    return mt;
}

void yagl_mem_transfer_destroy(struct yagl_mem_transfer *mt)
{
    g_free(mt->pages);
    g_free(mt);
}

bool yagl_mem_prepare(struct yagl_mem_transfer *mt,
                      target_ulong va,
                      int len)
{
    bool res = true;
    int l;
    hwaddr page_pa;
    target_ulong page_va;

    YAGL_LOG_FUNC_ENTER(yagl_mem_prepare, "va = 0x%X, len = %d", (uint32_t)va, len);

    if (len >= 0) {
        int max_pages = ((len + TARGET_PAGE_SIZE - 1) / TARGET_PAGE_SIZE) + 1;

        if (max_pages > mt->max_pages) {
            g_free(mt->pages);
            mt->pages = g_malloc(sizeof(*mt->pages) * max_pages);
        }

        mt->max_pages = max_pages;
    }

    mt->va = va;
    mt->offset = (va & ~TARGET_PAGE_MASK);
    mt->len = len;
    mt->num_pages = 0;

    if (va) {
        while (len > 0) {
            page_va = va & TARGET_PAGE_MASK;
            page_pa = cpu_get_phys_page_debug(current_cpu, page_va);

            if (page_pa == -1) {
                YAGL_LOG_WARN("page fault at 0x%X", (uint32_t)page_va);
                res = false;
                break;
            }

            l = (page_va + TARGET_PAGE_SIZE) - va;
            if (l > len) {
                l = len;
            }

            len -= l;
            va += l;

            assert(mt->num_pages < mt->max_pages);

            mt->pages[mt->num_pages++] = page_pa;
        }
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return res;
}

void yagl_mem_put(struct yagl_mem_transfer *mt, const void *data, int len)
{
    int offset = mt->offset, i = 0;

    YAGL_LOG_FUNC_ENTER(yagl_mem_put, "va = 0X%X, data = %p, len = %d", (uint32_t)mt->va, data, len);

    if (!mt->va) {
        assert(false);
        YAGL_LOG_CRITICAL("mt->va is 0");
        YAGL_LOG_FUNC_EXIT(NULL);
        return;
    }

    while (len > 0) {
        int l = MIN(len, (TARGET_PAGE_SIZE - offset));

        assert(i < mt->num_pages);

        cpu_physical_memory_write_rom(mt->pages[i++] + offset, data, l);

        offset = 0;

        data += l;
        len -= l;
    }

    YAGL_LOG_FUNC_EXIT(NULL);
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
