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

#ifndef _QEMU_YAGL_MEM_H
#define _QEMU_YAGL_MEM_H

#include "yagl_types.h"

struct yagl_mem_transfer
{
    hwaddr *pages;
    int max_pages;

    target_ulong va;
    int offset;
    int len;
    int num_pages;
};

/*
 * Guaranteed to succeed.
 */
struct yagl_mem_transfer *yagl_mem_transfer_create(void);

void yagl_mem_transfer_destroy(struct yagl_mem_transfer *mt);

bool yagl_mem_prepare(struct yagl_mem_transfer *mt,
                      target_ulong va,
                      int len);

void yagl_mem_put(struct yagl_mem_transfer *mt, const void *data, int len);

bool yagl_mem_get(target_ulong va, uint32_t len, void *data);

#endif
