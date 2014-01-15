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

#ifndef _QEMU_VIGS_COMM_H
#define _QEMU_VIGS_COMM_H

#include "vigs_types.h"

struct vigs_comm_ops
{
    void (*init)(void */*user_data*/);

    void (*reset)(void */*user_data*/);

    void (*exit)(void */*user_data*/);

    void (*set_root_surface)(void */*user_data*/,
                             vigsp_surface_id /*id*/,
                             vigsp_offset /*offset*/,
                             vigsp_fence_seq /*fence_seq*/);

    void (*batch)(void */*user_data*/,
                  const uint8_t */*data*/,
                  uint32_t /*size*/);
};

struct vigs_comm_batch_ops
{
    void (*start)(void */*user_data*/);

    void (*create_surface)(void */*user_data*/,
                           uint32_t /*width*/,
                           uint32_t /*height*/,
                           uint32_t /*stride*/,
                           vigsp_surface_format /*format*/,
                           vigsp_surface_id /*id*/);

    void (*destroy_surface)(void */*user_data*/,
                            vigsp_surface_id /*id*/);

    void (*update_vram)(void */*user_data*/,
                        vigsp_surface_id /*sfc_id*/,
                        vigsp_offset /*offset*/);

    void (*update_gpu)(void */*user_data*/,
                       vigsp_surface_id /*sfc_id*/,
                       vigsp_offset /*offset*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*copy)(void */*user_data*/,
                 vigsp_surface_id /*src_id*/,
                 vigsp_surface_id /*dst_id*/,
                 const struct vigsp_copy */*entries*/,
                 uint32_t /*num_entries*/);

    void (*solid_fill)(void */*user_data*/,
                       vigsp_surface_id /*sfc_id*/,
                       vigsp_color /*color*/,
                       const struct vigsp_rect */*entries*/,
                       uint32_t /*num_entries*/);

    void (*end)(void */*user_data*/, vigsp_fence_seq /*fence_seq*/);
};

struct vigs_comm
{
    uint8_t *ram_ptr;
};

struct vigs_comm *vigs_comm_create(uint8_t *ram_ptr);

void vigs_comm_destroy(struct vigs_comm *comm);

void vigs_comm_dispatch(struct vigs_comm *comm,
                        uint32_t ram_offset,
                        struct vigs_comm_ops *ops,
                        void *user_data);

void vigs_comm_dispatch_batch(struct vigs_comm *comm,
                              uint8_t *batch,
                              struct vigs_comm_batch_ops *ops,
                              void *user_data);

#endif
