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

#ifndef _QEMU_YAGL_COMPILED_TRANSFER_H
#define _QEMU_YAGL_COMPILED_TRANSFER_H

#include "yagl_types.h"

struct yagl_compiled_transfer_section
{
    /*
     * For 'cpy_physical_memory_unmap'.
     */
    void *map_base;
    hwaddr map_len;

    /*
     * For actual I/O.
     */
    void *base;
    uint32_t len;
};

struct yagl_compiled_transfer
{
    struct yagl_compiled_transfer_section *sections;
    int num_sections;
    bool is_write;
};

struct yagl_compiled_transfer
    *yagl_compiled_transfer_create(target_ulong va,
                                   uint32_t len,
                                   bool is_write);

void yagl_compiled_transfer_destroy(struct yagl_compiled_transfer *ct);

void yagl_compiled_transfer_exec(struct yagl_compiled_transfer *ct, void* data);

#endif
