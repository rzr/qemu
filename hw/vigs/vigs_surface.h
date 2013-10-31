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

#ifndef _QEMU_VIGS_SURFACE_H
#define _QEMU_VIGS_SURFACE_H

#include "vigs_types.h"

struct winsys_surface;
struct vigs_backend;

struct vigs_surface
{
    struct winsys_surface *ws_sfc;

    struct vigs_backend *backend;

    uint32_t stride;
    vigsp_surface_format format;
    vigsp_surface_id id;

    bool is_dirty;

    void (*read_pixels)(struct vigs_surface */*sfc*/,
                        uint32_t /*x*/,
                        uint32_t /*y*/,
                        uint32_t /*width*/,
                        uint32_t /*height*/,
                        uint8_t */*pixels*/);

    void (*draw_pixels)(struct vigs_surface */*sfc*/,
                        uint8_t */*pixels*/,
                        const struct vigsp_rect */*entries*/,
                        uint32_t /*num_entries*/);

    void (*copy)(struct vigs_surface */*dst*/,
                 struct vigs_surface */*src*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    void (*solid_fill)(struct vigs_surface */*sfc*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*destroy)(struct vigs_surface */*sfc*/);
};

void vigs_surface_init(struct vigs_surface *sfc,
                       struct winsys_surface *ws_sfc,
                       struct vigs_backend *backend,
                       uint32_t stride,
                       vigsp_surface_format format,
                       vigsp_surface_id id);

void vigs_surface_cleanup(struct vigs_surface *sfc);

#endif
