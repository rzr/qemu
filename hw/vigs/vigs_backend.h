/*
 * vigs
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

#ifndef _QEMU_VIGS_BACKEND_H
#define _QEMU_VIGS_BACKEND_H

#include "vigs_types.h"

struct winsys_info;
struct vigs_surface;

struct vigs_backend
{
    struct winsys_info *ws_info;

    void (*batch_start)(struct vigs_backend */*backend*/);

    struct vigs_surface *(*create_surface)(struct vigs_backend */*backend*/,
                                           uint32_t /*width*/,
                                           uint32_t /*height*/,
                                           uint32_t /*stride*/,
                                           vigsp_surface_format /*format*/,
                                           vigsp_surface_id /*id*/);

    void (*batch_end)(struct vigs_backend */*backend*/);

    void (*destroy)(struct vigs_backend */*backend*/);
};

void vigs_backend_init(struct vigs_backend *backend,
                       struct winsys_info *ws_info);

void vigs_backend_cleanup(struct vigs_backend *backend);

struct vigs_backend *vigs_gl_backend_create(void *display);
struct vigs_backend *vigs_sw_backend_create(void);

#endif
