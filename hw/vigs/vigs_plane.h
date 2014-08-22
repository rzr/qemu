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

#ifndef _QEMU_VIGS_PLANE_H
#define _QEMU_VIGS_PLANE_H

#include "vigs_types.h"
#include "vigs_surface.h"

struct vigs_plane
{
    uint32_t width;
    uint32_t height;

    vigsp_plane_format format;

    struct vigs_surface *surfaces[4];

    struct vigsp_rect src_rect;

    int dst_x;
    int dst_y;
    struct vigsp_size dst_size;

    int z_pos;
    bool hflip;
    bool vflip;
    vigsp_rotation rotation;

    /*
     * Plane moved/resized, need to recomposite.
     */
    bool is_dirty;
};

static __inline bool vigs_plane_enabled(const struct vigs_plane *plane)
{
    return plane->surfaces[0] != NULL;
}

static __inline bool vigs_plane_dirty(const struct vigs_plane *plane)
{
    int i;

    if (plane->is_dirty) {
        return true;
    }

    for (i = 0; i < 4; ++i) {
        if (plane->surfaces[i] && plane->surfaces[i]->is_dirty) {
            return true;
        }
    }

    return false;
}

static __inline void vigs_plane_reset_dirty(struct vigs_plane *plane)
{
    int i;

    plane->is_dirty = false;

    for (i = 0; i < 4; ++i) {
        if (plane->surfaces[i]) {
            plane->surfaces[i]->is_dirty = false;
        }
    }
}

static __inline void vigs_plane_detach_surface(struct vigs_plane *plane,
                                               struct vigs_surface *sfc)
{
    int i;

    for (i = 0; i < 4; ++i) {
        if (plane->surfaces[i] == sfc) {
            /*
             * If at least one surface gets detached - entire plane
             * gets disabled.
             */

            memset(plane->surfaces, 0, sizeof(plane->surfaces));
            plane->is_dirty = true;

            return;
        }
    }
}

#endif
