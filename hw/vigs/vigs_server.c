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

#include "vigs_server.h"
#include "vigs_log.h"
#include "vigs_comm.h"
#include "vigs_backend.h"
#include "vigs_surface.h"
#include "vigs_utils.h"
#include "work_queue.h"

struct vigs_server_work_item
{
    struct work_queue_item base;

    struct vigs_server *server;
};

struct vigs_server_set_root_surface_work_item
{
    struct work_queue_item base;

    struct vigs_server *server;

    vigsp_surface_id id;
    vigsp_offset offset;
};

static void vigs_server_surface_destroy_func(gpointer data)
{
    struct vigs_surface *sfc = data;

    sfc->destroy(sfc);
}

static void vigs_server_unuse_surface(struct vigs_server *server,
                                      struct vigs_surface *sfc)
{
    /*
     * If it was root surface then root surface is now NULL.
     */

    if (server->root_sfc == sfc) {
        server->root_sfc = NULL;
        server->root_sfc_ptr = NULL;
    }
}

static struct winsys_surface
    *vigs_server_acquire_surface(struct winsys_interface *wsi,
                                 winsys_id id)
{
    struct vigs_server *server =
        container_of(wsi, struct vigs_server, wsi);
    struct vigs_surface *res;

    res = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(id));

    if (!res) {
        return NULL;
    }

    res->ws_sfc->acquire(res->ws_sfc);

    return res->ws_sfc;
}

static void vigs_server_fence_ack(struct winsys_interface *wsi,
                                  uint32_t fence_seq)
{
    struct vigs_server *server =
        container_of(wsi, struct vigs_server, wsi);

    if (fence_seq) {
        server->display_ops->fence_ack(server->display_user_data, fence_seq);
    }
}

static void vigs_server_dispatch_batch_start(void *user_data)
{
    struct vigs_server *server = user_data;

    server->backend->batch_start(server->backend);
}

static void vigs_server_dispatch_create_surface(void *user_data,
                                                uint32_t width,
                                                uint32_t height,
                                                uint32_t stride,
                                                vigsp_surface_format format,
                                                vigsp_surface_id id)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    sfc = server->backend->create_surface(server->backend,
                                          width,
                                          height,
                                          stride,
                                          format,
                                          id);

    if (!sfc) {
        return;
    }

    if (g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc->id))) {
        VIGS_LOG_CRITICAL("surface %u already exists", sfc->id);
        assert(false);
        sfc->destroy(sfc);
        return;
    }

    g_hash_table_insert(server->surfaces, GUINT_TO_POINTER(sfc->id), sfc);

    VIGS_LOG_TRACE("num_surfaces = %u", g_hash_table_size(server->surfaces));
}

static void vigs_server_dispatch_destroy_surface(void *user_data,
                                                 vigsp_surface_id id)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(id));

    if (!sfc) {
        VIGS_LOG_ERROR("surface %u not found", id);
        return;
    }

    vigs_server_unuse_surface(server, sfc);

    g_hash_table_remove(server->surfaces, GUINT_TO_POINTER(id));

    VIGS_LOG_TRACE("num_surfaces = %u", g_hash_table_size(server->surfaces));
}

static void vigs_server_dispatch_update_vram(void *user_data,
                                             vigsp_surface_id sfc_id,
                                             vigsp_offset offset)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    vigs_sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc_id));

    if (!vigs_sfc) {
        VIGS_LOG_ERROR("surface %u not found", sfc_id);
        return;
    }

    vigs_sfc->read_pixels(vigs_sfc,
                          0,
                          0,
                          vigs_sfc->ws_sfc->width,
                          vigs_sfc->ws_sfc->height,
                          server->vram_ptr + offset);

    vigs_sfc->is_dirty = false;
}

static void vigs_server_dispatch_update_gpu(void *user_data,
                                            vigsp_surface_id sfc_id,
                                            vigsp_offset offset,
                                            const struct vigsp_rect *entries,
                                            uint32_t num_entries)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    vigs_sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc_id));

    if (!vigs_sfc) {
        VIGS_LOG_ERROR("surface %u not found", sfc_id);
        return;
    }

    vigs_sfc->draw_pixels(vigs_sfc,
                          server->vram_ptr + offset,
                          entries,
                          num_entries);

    vigs_sfc->is_dirty = true;
}

static void vigs_server_dispatch_copy(void *user_data,
                                      vigsp_surface_id src_id,
                                      vigsp_surface_id dst_id,
                                      const struct vigsp_copy *entries,
                                      uint32_t num_entries)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *src;
    struct vigs_surface *dst;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    src = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(src_id));

    if (!src) {
        VIGS_LOG_ERROR("src surface %u not found", src_id);
        return;
    }

    if (src_id == dst_id) {
        dst = src;
    } else {
        dst = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(dst_id));

        if (!dst) {
            VIGS_LOG_ERROR("dst surface %u not found", dst_id);
            return;
        }
    }

    dst->copy(dst, src, entries, num_entries);

    dst->is_dirty = true;
}

static void vigs_server_dispatch_solid_fill(void *user_data,
                                            vigsp_surface_id sfc_id,
                                            vigsp_color color,
                                            const struct vigsp_rect *entries,
                                            uint32_t num_entries)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc_id));

    if (!sfc) {
        VIGS_LOG_ERROR("surface %u not found", sfc_id);
        return;
    }

    sfc->solid_fill(sfc, color, entries, num_entries);

    sfc->is_dirty = true;
}

static void vigs_server_dispatch_batch_end(void *user_data,
                                           vigsp_fence_seq fence_seq)
{
    struct vigs_server *server = user_data;

    server->backend->batch_end(server->backend);

    if (fence_seq) {
        server->display_ops->fence_ack(server->display_user_data, fence_seq);
    }
}

static struct vigs_comm_batch_ops vigs_server_dispatch_batch_ops =
{
    .start = &vigs_server_dispatch_batch_start,
    .create_surface = &vigs_server_dispatch_create_surface,
    .destroy_surface = &vigs_server_dispatch_destroy_surface,
    .update_vram = &vigs_server_dispatch_update_vram,
    .update_gpu = &vigs_server_dispatch_update_gpu,
    .copy = &vigs_server_dispatch_copy,
    .solid_fill = &vigs_server_dispatch_solid_fill,
    .end = &vigs_server_dispatch_batch_end
};

static void vigs_server_work(struct work_queue_item *wq_item)
{
    struct vigs_server_work_item *item = (struct vigs_server_work_item*)wq_item;

    vigs_comm_dispatch_batch(item->server->comm,
                             (uint8_t*)(item + 1),
                             &vigs_server_dispatch_batch_ops,
                             item->server);

    g_free(item);
}

static void vigs_server_set_root_surface_work(struct work_queue_item *wq_item)
{
    struct vigs_server_set_root_surface_work_item *item =
        (struct vigs_server_set_root_surface_work_item*)wq_item;
    struct vigs_server *server = item->server;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        goto out;
    }

    if (item->id == 0) {
        server->root_sfc = NULL;
        server->root_sfc_ptr = NULL;

        VIGS_LOG_TRACE("root surface reset");

        goto out;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(item->id));

    if (!sfc) {
        VIGS_LOG_ERROR("surface %u not found", item->id);
        goto out;
    }

    server->root_sfc = sfc;
    server->root_sfc_ptr = server->vram_ptr + item->offset;

out:
    g_free(item);
}

static void vigs_server_update_display_work(struct work_queue_item *wq_item)
{
    struct vigs_server_work_item *item = (struct vigs_server_work_item*)wq_item;
    struct vigs_server *server = item->server;
    struct vigs_surface *root_sfc = server->root_sfc;
    uint32_t capture_fence_seq;

    if (!root_sfc) {
        qemu_mutex_lock(&server->capture_mutex);
        goto out;
    }

    if (root_sfc->is_dirty) {
        server->backend->batch_start(server->backend);
        root_sfc->read_pixels(root_sfc,
                              0,
                              0,
                              root_sfc->ws_sfc->width,
                              root_sfc->ws_sfc->height,
                              server->root_sfc_ptr);
        server->backend->batch_end(server->backend);
        root_sfc->is_dirty = false;
    }

    qemu_mutex_lock(&server->capture_mutex);

    if ((server->captured.stride != root_sfc->stride) ||
        (server->captured.height != root_sfc->ws_sfc->height)) {
        g_free(server->captured.data);
        server->captured.data = g_malloc(root_sfc->stride *
                                         root_sfc->ws_sfc->height);
    }

    memcpy(server->captured.data,
           server->root_sfc_ptr,
           root_sfc->stride * root_sfc->ws_sfc->height);

    server->captured.width = root_sfc->ws_sfc->width;
    server->captured.height = root_sfc->ws_sfc->height;
    server->captured.stride = root_sfc->stride;
    server->captured.format = root_sfc->format;

out:
    server->is_capturing = false;
    capture_fence_seq = server->capture_fence_seq;
    server->capture_fence_seq = 0;

    qemu_mutex_unlock(&server->capture_mutex);

    if (capture_fence_seq) {
        server->display_ops->fence_ack(server->display_user_data,
                                       capture_fence_seq);
    }

    g_free(item);
}

static void vigs_server_dispatch_init(void *user_data)
{
    struct vigs_server *server = user_data;

    work_queue_wait(server->render_queue);

    if (server->initialized) {
        VIGS_LOG_ERROR("already initialized");
        return;
    }

    server->initialized = true;
}

static void vigs_server_dispatch_reset(void *user_data)
{
    struct vigs_server *server = user_data;
    GHashTableIter iter;
    gpointer key, value;

    work_queue_wait(server->render_queue);

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    g_hash_table_iter_init(&iter, server->surfaces);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        struct vigs_surface *sfc = value;

        if (sfc != server->root_sfc) {
            vigs_server_unuse_surface(server, sfc);
            g_hash_table_iter_remove(&iter);
        }
    }
}

static void vigs_server_dispatch_exit(void *user_data)
{
    struct vigs_server *server = user_data;

    work_queue_wait(server->render_queue);

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    vigs_server_reset(server);
}

static void vigs_server_dispatch_set_root_surface(void *user_data,
                                                  vigsp_surface_id id,
                                                  vigsp_offset offset,
                                                  vigsp_fence_seq fence_seq)
{
    struct vigs_server *server = user_data;
    struct vigs_server_set_root_surface_work_item *item;
    uint32_t capture_fence_seq = 0;

    item = g_malloc(sizeof(*item));

    work_queue_item_init(&item->base, &vigs_server_set_root_surface_work);

    item->server = server;
    item->id = id;
    item->offset = offset;

    work_queue_add_item(server->render_queue, &item->base);

    qemu_mutex_lock(&server->capture_mutex);

    if (server->is_capturing) {
        capture_fence_seq = server->capture_fence_seq;
        server->capture_fence_seq = fence_seq;
    } else {
        capture_fence_seq = fence_seq;
    }

    qemu_mutex_unlock(&server->capture_mutex);

    if (capture_fence_seq) {
        server->display_ops->fence_ack(server->display_user_data,
                                       capture_fence_seq);
    }
}

static void vigs_server_dispatch_batch(void *user_data,
                                       const uint8_t *data,
                                       uint32_t size)
{
    struct vigs_server *server = user_data;
    struct vigs_server_work_item *item;

    item = g_malloc(sizeof(*item) + size);

    work_queue_item_init(&item->base, &vigs_server_work);

    item->server = server;
    memcpy((item + 1), data, size);

    work_queue_add_item(server->render_queue, &item->base);
}

static struct vigs_comm_ops vigs_server_dispatch_ops =
{
    .init = &vigs_server_dispatch_init,
    .reset = &vigs_server_dispatch_reset,
    .exit = &vigs_server_dispatch_exit,
    .set_root_surface = &vigs_server_dispatch_set_root_surface,
    .batch = &vigs_server_dispatch_batch
};

struct vigs_server *vigs_server_create(uint8_t *vram_ptr,
                                       uint8_t *ram_ptr,
                                       struct vigs_display_ops *display_ops,
                                       void *display_user_data,
                                       struct vigs_backend *backend,
                                       struct work_queue *render_queue)
{
    struct vigs_server *server = NULL;

    server = g_malloc0(sizeof(*server));

    server->wsi.ws_info = backend->ws_info;
    server->wsi.acquire_surface = &vigs_server_acquire_surface;
    server->wsi.fence_ack = &vigs_server_fence_ack;

    server->vram_ptr = vram_ptr;
    server->display_ops = display_ops;
    server->display_user_data = display_user_data;
    server->backend = backend;
    server->render_queue = render_queue;

    server->comm = vigs_comm_create(ram_ptr);

    if (!server->comm) {
        goto fail;
    }

    server->surfaces = g_hash_table_new_full(g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             vigs_server_surface_destroy_func);

    qemu_mutex_init(&server->capture_mutex);

    return server;

fail:
    g_free(server);

    return NULL;
}

void vigs_server_destroy(struct vigs_server *server)
{
    vigs_server_reset(server);

    g_hash_table_destroy(server->surfaces);
    vigs_comm_destroy(server->comm);
    server->backend->destroy(server->backend);
    qemu_mutex_destroy(&server->capture_mutex);
    g_free(server->captured.data);
    g_free(server);
}

void vigs_server_reset(struct vigs_server *server)
{
    GHashTableIter iter;
    gpointer key, value;

    work_queue_wait(server->render_queue);

    server->backend->batch_start(server->backend);

    g_hash_table_iter_init(&iter, server->surfaces);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        struct vigs_surface *sfc = value;

        vigs_server_unuse_surface(server, sfc);

        g_hash_table_iter_remove(&iter);
    }

    server->backend->batch_end(server->backend);

    server->initialized = false;
}

void vigs_server_dispatch(struct vigs_server *server,
                          uint32_t ram_offset)
{
    vigs_comm_dispatch(server->comm,
                       ram_offset,
                       &vigs_server_dispatch_ops,
                       server);
}

void vigs_server_update_display(struct vigs_server *server)
{
    uint32_t sfc_bpp;
    uint32_t display_stride, display_bpp;
    uint8_t *display_data;

    qemu_mutex_lock(&server->capture_mutex);

    if (!server->captured.data) {
        goto out;
    }

    sfc_bpp = vigs_format_bpp(server->captured.format);

    server->display_ops->resize(server->display_user_data,
                                server->captured.width,
                                server->captured.height);

    display_stride = server->display_ops->get_stride(server->display_user_data);
    display_bpp = server->display_ops->get_bpp(server->display_user_data);
    display_data = server->display_ops->get_data(server->display_user_data);

    if (sfc_bpp != display_bpp) {
        VIGS_LOG_CRITICAL("bpp mismatch");
        assert(false);
        exit(1);
    }

    if (display_stride == server->captured.stride) {
        switch (server->captured.format) {
        case vigsp_surface_bgrx8888:
        case vigsp_surface_bgra8888:
            memcpy(display_data,
                   server->captured.data,
                   server->captured.height * display_stride);
            break;
        default:
            assert(false);
            VIGS_LOG_CRITICAL("unknown format: %d", server->captured.format);
            exit(1);
        }
    } else {
        uint32_t i;
        uint8_t *src = server->captured.data;
        uint8_t *dst = display_data;

        for (i = 0; i < server->captured.height; ++i) {
            switch (server->captured.format) {
            case vigsp_surface_bgrx8888:
            case vigsp_surface_bgra8888:
                memcpy(dst, src, server->captured.width * sfc_bpp);
                break;
            default:
                assert(false);
                VIGS_LOG_CRITICAL("unknown format: %d", server->captured.format);
                exit(1);
            }
            src += server->captured.stride;
            dst += display_stride;
        }
    }

out:
    qemu_mutex_unlock(&server->capture_mutex);

    if (!server->is_capturing) {
        struct vigs_server_work_item *item;

        item = g_malloc(sizeof(*item));

        work_queue_item_init(&item->base, &vigs_server_update_display_work);

        item->server = server;

        server->is_capturing = true;

        work_queue_add_item(server->render_queue, &item->base);
    }
}
