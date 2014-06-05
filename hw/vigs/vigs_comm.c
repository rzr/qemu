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

#include "vigs_comm.h"
#include "vigs_log.h"

/*
 * Protocol command handlers go here.
 * @{
 */

static void vigs_comm_dispatch_init(struct vigs_comm_ops *ops,
                                    void *user_data,
                                    struct vigsp_cmd_init_request *request)
{
    request->server_version = VIGS_PROTOCOL_VERSION;

    if (request->client_version != VIGS_PROTOCOL_VERSION) {
        VIGS_LOG_CRITICAL("protocol version mismatch, expected %u, actual %u",
                          VIGS_PROTOCOL_VERSION,
                          request->client_version);
        return;
    }

    VIGS_LOG_TRACE("client_version = %d", request->client_version);

    ops->init(user_data);
}

static void vigs_comm_dispatch_reset(struct vigs_comm_ops *ops,
                                     void *user_data)
{
    VIGS_LOG_TRACE("enter");

    ops->reset(user_data);
}

static void vigs_comm_dispatch_exit(struct vigs_comm_ops *ops,
                                    void *user_data)
{
    VIGS_LOG_TRACE("enter");

    ops->exit(user_data);
}

static void vigs_comm_dispatch_set_root_surface(struct vigs_comm_ops *ops,
                                                void *user_data,
                                                struct vigsp_cmd_set_root_surface_request *request,
                                                vigsp_fence_seq fence_seq)
{
    VIGS_LOG_TRACE("id = %u, scanout = %d, offset = %u",
                   request->id,
                   request->scanout,
                   request->offset);

    ops->set_root_surface(user_data,
                          request->id,
                          request->scanout,
                          request->offset,
                          fence_seq);
}

static void vigs_comm_dispatch_create_surface(struct vigs_comm_batch_ops *ops,
                                              void *user_data,
                                              struct vigsp_cmd_create_surface_request *request)
{
    switch (request->format) {
    case vigsp_surface_bgrx8888:
    case vigsp_surface_bgra8888:
        break;
    default:
        VIGS_LOG_CRITICAL("bad surface format = %d", request->format);
        return;
    }

    VIGS_LOG_TRACE("%ux%u, strd = %u, fmt = %d, id = %u",
                   request->width,
                   request->height,
                   request->stride,
                   request->format,
                   request->id);

    ops->create_surface(user_data,
                        request->width,
                        request->height,
                        request->stride,
                        request->format,
                        request->id);
}

static void vigs_comm_dispatch_destroy_surface(struct vigs_comm_batch_ops *ops,
                                               void *user_data,
                                               struct vigsp_cmd_destroy_surface_request *request)
{
    VIGS_LOG_TRACE("id = %u", request->id);

    ops->destroy_surface(user_data, request->id);
}

static void vigs_comm_dispatch_update_vram(struct vigs_comm_batch_ops *ops,
                                           void *user_data,
                                           struct vigsp_cmd_update_vram_request *request)
{
    if (request->sfc_id == 0) {
        VIGS_LOG_TRACE("skipped");
        return;
    } else {
        VIGS_LOG_TRACE("sfc = %u(off = %u)",
                       request->sfc_id,
                       request->offset);
    }

    ops->update_vram(user_data,
                     request->sfc_id,
                     request->offset);
}

static void vigs_comm_dispatch_update_gpu(struct vigs_comm_batch_ops *ops,
                                          void *user_data,
                                          struct vigsp_cmd_update_gpu_request *request)
{
    if (request->sfc_id == 0) {
        VIGS_LOG_TRACE("skipped");
        return;
    } else {
        VIGS_LOG_TRACE("sfc = %u(off = %u)",
                       request->sfc_id,
                       request->offset);
    }

    ops->update_gpu(user_data,
                    request->sfc_id,
                    request->offset,
                    &request->entries[0],
                    request->num_entries);
}

static void vigs_comm_dispatch_copy(struct vigs_comm_batch_ops *ops,
                                    void *user_data,
                                    struct vigsp_cmd_copy_request *request)
{
    VIGS_LOG_TRACE("src = %u, dst = %u",
                   request->src_id,
                   request->dst_id);

    ops->copy(user_data,
              request->src_id,
              request->dst_id,
              &request->entries[0],
              request->num_entries);
}

static void vigs_comm_dispatch_solid_fill(struct vigs_comm_batch_ops *ops,
                                          void *user_data,
                                          struct vigsp_cmd_solid_fill_request *request)
{
    VIGS_LOG_TRACE("sfc = %u, color = 0x%X",
                   request->sfc_id,
                   request->color);

    ops->solid_fill(user_data,
                    request->sfc_id,
                    request->color,
                    &request->entries[0],
                    request->num_entries);
}

static void vigs_comm_dispatch_set_plane(struct vigs_comm_batch_ops *ops,
                                         void *user_data,
                                         struct vigsp_cmd_set_plane_request *request)
{
    VIGS_LOG_TRACE("plane = %u, sfc_id = %u, src_rect = {%u, %u, %u, %u}, dst_x = %d, dst_y = %d, dst_size = {%u, %u}, z_pos = %d",
                   request->plane,
                   request->sfc_id,
                   request->src_rect.pos.x,
                   request->src_rect.pos.y,
                   request->src_rect.size.w,
                   request->src_rect.size.h,
                   request->dst_x,
                   request->dst_y,
                   request->dst_size.w,
                   request->dst_size.h,
                   request->z_pos);

    ops->set_plane(user_data, request->plane, request->sfc_id,
                   &request->src_rect, request->dst_x, request->dst_y,
                   &request->dst_size, request->z_pos);
}

/*
 * @}
 */

#define VIGS_DISPATCH_ENTRY(cmd, func) [cmd] = (vigs_dispatch_func)func

typedef void (*vigs_dispatch_func)(struct vigs_comm_batch_ops*, void*, void*);

static const vigs_dispatch_func vigs_dispatch_table[] =
{
    VIGS_DISPATCH_ENTRY(vigsp_cmd_create_surface,
                        vigs_comm_dispatch_create_surface),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_destroy_surface,
                        vigs_comm_dispatch_destroy_surface),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_update_vram,
                        vigs_comm_dispatch_update_vram),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_update_gpu,
                        vigs_comm_dispatch_update_gpu),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_copy,
                        vigs_comm_dispatch_copy),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_solid_fill,
                        vigs_comm_dispatch_solid_fill),
    VIGS_DISPATCH_ENTRY(vigsp_cmd_set_plane,
                        vigs_comm_dispatch_set_plane)
};

#define VIGS_MIN_BATCH_CMD_ID vigsp_cmd_create_surface
#define VIGS_MAX_BATCH_CMD_ID vigsp_cmd_set_plane

struct vigs_comm *vigs_comm_create(uint8_t *ram_ptr)
{
    struct vigs_comm *comm;

    comm = g_malloc0(sizeof(*comm));

    comm->ram_ptr = ram_ptr;

    return comm;
}

void vigs_comm_destroy(struct vigs_comm *comm)
{
    g_free(comm);
}

void vigs_comm_dispatch(struct vigs_comm *comm,
                        uint32_t ram_offset,
                        struct vigs_comm_ops *ops,
                        void *user_data)
{
    struct vigsp_cmd_batch_header *batch_header =
        (struct vigsp_cmd_batch_header*)(comm->ram_ptr + ram_offset);
    struct vigsp_cmd_request_header *request_header =
        (struct vigsp_cmd_request_header*)(batch_header + 1);

    if (batch_header->size > 0) {
        switch (request_header->cmd) {
        case vigsp_cmd_init:
            vigs_comm_dispatch_init(ops,
                                    user_data,
                                    (struct vigsp_cmd_init_request*)(request_header + 1));
            return;
        case vigsp_cmd_reset:
            vigs_comm_dispatch_reset(ops, user_data);
            return;
        case vigsp_cmd_exit:
            vigs_comm_dispatch_exit(ops, user_data);
            return;
        case vigsp_cmd_set_root_surface:
            vigs_comm_dispatch_set_root_surface(ops,
                                                user_data,
                                                (struct vigsp_cmd_set_root_surface_request*)(request_header + 1),
                                                batch_header->fence_seq);
            return;
        default:
            break;
        }
    }

    ops->batch(user_data,
               (uint8_t*)batch_header,
               sizeof(*batch_header) + batch_header->size);
}

void vigs_comm_dispatch_batch(struct vigs_comm *comm,
                              uint8_t *batch,
                              struct vigs_comm_batch_ops *ops,
                              void *user_data)
{
    struct vigsp_cmd_batch_header *batch_header =
        (struct vigsp_cmd_batch_header*)batch;
    struct vigsp_cmd_request_header *request_header =
        (struct vigsp_cmd_request_header*)(batch_header + 1);

    VIGS_LOG_TRACE("batch_start");

    ops->start(user_data);

    while ((uint8_t*)request_header <
           (uint8_t*)(batch_header + 1) + batch_header->size) {
        if ((request_header->cmd < VIGS_MIN_BATCH_CMD_ID) ||
            (request_header->cmd > VIGS_MAX_BATCH_CMD_ID)) {
            VIGS_LOG_CRITICAL("bad command = %d", request_header->cmd);
        } else {
            vigs_dispatch_table[request_header->cmd](ops,
                                                     user_data,
                                                     request_header + 1);
        }

        request_header = (struct vigsp_cmd_request_header*)(
            (uint8_t*)(request_header + 1) + request_header->size);
    }

    VIGS_LOG_TRACE("batch_end(%d)", batch_header->fence_seq);

    ops->end(user_data, batch_header->fence_seq);
}
