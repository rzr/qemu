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

#ifndef _QEMU_VIGS_SERVER_H
#define _QEMU_VIGS_SERVER_H

#include "vigs_types.h"
#include "winsys.h"
#include <glib.h>

struct vigs_comm;
struct vigs_backend;
struct work_queue;

struct vigs_display_ops
{
    /*
     * These are only called from 'vigs_server_update_display'.
     * @{
     */

    void (*resize)(void */*user_data*/,
                   uint32_t /*width*/,
                   uint32_t /*height*/);

    uint32_t (*get_stride)(void */*user_data*/);

    /*
     * Returns display's bytes-per-pixel.
     */
    uint32_t (*get_bpp)(void */*user_data*/);

    uint8_t *(*get_data)(void */*user_data*/);

    /*
     * @}
     */

    void (*fence_ack)(void */*user_data*/,
                      uint32_t /*fence_seq*/);
};

struct vigs_server
{
    struct winsys_interface wsi;

    uint8_t *vram_ptr;

    struct vigs_display_ops *display_ops;
    void *display_user_data;

    struct vigs_backend *backend;

    struct work_queue *render_queue;

    struct vigs_comm *comm;

    /*
     * The following can be modified during
     * server operation.
     * @{
     */

    bool initialized;

    GHashTable *surfaces;

    struct vigs_surface *root_sfc;
    uint8_t *root_sfc_ptr;

    QemuMutex capture_mutex;

    bool is_capturing;

    uint32_t capture_fence_seq;

    struct
    {
        uint8_t *data;
        uint32_t width;
        uint32_t height;
        uint32_t stride;
        vigsp_surface_format format;
    } captured;

    /*
     * @}
     */
};

struct vigs_server *vigs_server_create(uint8_t *vram_ptr,
                                       uint8_t *ram_ptr,
                                       struct vigs_display_ops *display_ops,
                                       void *display_user_data,
                                       struct vigs_backend *backend,
                                       struct work_queue *render_queue);

void vigs_server_destroy(struct vigs_server *server);

void vigs_server_reset(struct vigs_server *server);

void vigs_server_dispatch(struct vigs_server *server,
                          uint32_t ram_offset);

void vigs_server_update_display(struct vigs_server *server);

#endif
