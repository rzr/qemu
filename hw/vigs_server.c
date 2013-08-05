#include "vigs_server.h"
#include "vigs_log.h"
#include "vigs_comm.h"
#include "vigs_backend.h"
#include "vigs_surface.h"
#include "vigs_utils.h"

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
        server->root_sfc_data = NULL;
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

static void vigs_server_end_batch(struct vigs_server *server)
{
    if (server->in_batch) {
        server->backend->batch_end(server->backend);

        server->in_batch = false;
    }
}

static void vigs_server_dispatch_batch_start(void *user_data)
{
    struct vigs_server *server = user_data;

    server->in_batch = false;

    if (server->initialized) {
        server->backend->batch_start(server->backend);

        server->in_batch = true;
    }
}

static bool vigs_server_dispatch_init(void *user_data)
{
    struct vigs_server *server = user_data;

    if (server->initialized) {
        VIGS_LOG_ERROR("already initialized");
        return false;
    }

    server->initialized = true;

    return true;
}

static void vigs_server_dispatch_reset(void *user_data)
{
    struct vigs_server *server = user_data;
    GHashTableIter iter;
    gpointer key, value;

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

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    vigs_server_end_batch(server);

    vigs_server_reset(server);
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

static void vigs_server_dispatch_set_root_surface(void *user_data,
                                                  vigsp_surface_id id,
                                                  vigsp_offset offset)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return;
    }

    if (id == 0) {
        server->root_sfc = NULL;
        server->root_sfc_data = NULL;

        VIGS_LOG_TRACE("root surface reset");

        return;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(id));

    if (!sfc) {
        VIGS_LOG_ERROR("surface %u not found", id);
        return;
    }

    server->root_sfc = sfc;
    server->root_sfc_data = server->vram_ptr + offset;
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

static void vigs_server_dispatch_batch_end(void *user_data)
{
    struct vigs_server *server = user_data;

    vigs_server_end_batch(server);
}

static struct vigs_comm_ops vigs_server_dispatch_ops =
{
    .batch_start = &vigs_server_dispatch_batch_start,
    .init = &vigs_server_dispatch_init,
    .reset = &vigs_server_dispatch_reset,
    .exit = &vigs_server_dispatch_exit,
    .create_surface = &vigs_server_dispatch_create_surface,
    .destroy_surface = &vigs_server_dispatch_destroy_surface,
    .set_root_surface = &vigs_server_dispatch_set_root_surface,
    .update_vram = &vigs_server_dispatch_update_vram,
    .update_gpu = &vigs_server_dispatch_update_gpu,
    .copy = &vigs_server_dispatch_copy,
    .solid_fill = &vigs_server_dispatch_solid_fill,
    .batch_end = &vigs_server_dispatch_batch_end
};

struct vigs_server *vigs_server_create(uint8_t *vram_ptr,
                                       uint8_t *ram_ptr,
                                       struct vigs_display_ops *display_ops,
                                       void *display_user_data,
                                       struct vigs_backend *backend)
{
    struct vigs_server *server = NULL;

    server = g_malloc0(sizeof(*server));

    server->wsi.ws_info = backend->ws_info;
    server->wsi.acquire_surface = &vigs_server_acquire_surface;

    server->vram_ptr = vram_ptr;
    server->display_ops = display_ops;
    server->display_user_data = display_user_data;
    server->backend = backend;

    server->comm = vigs_comm_create(ram_ptr,
                                    &vigs_server_dispatch_ops,
                                    server);

    if (!server->comm) {
        goto fail;
    }

    server->surfaces = g_hash_table_new_full(g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             vigs_server_surface_destroy_func);

    vigs_vector_init(&server->v1, 0);

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
    vigs_vector_cleanup(&server->v1);
    server->backend->destroy(server->backend);
    g_free(server);
}

void vigs_server_reset(struct vigs_server *server)
{
    GHashTableIter iter;
    gpointer key, value;

    server->backend->batch_start(server->backend);

    g_hash_table_iter_init(&iter, server->surfaces);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        struct vigs_surface *sfc = value;

        vigs_server_unuse_surface(server, sfc);

        g_hash_table_iter_remove(&iter);
    }

    server->backend->batch_end(server->backend);

    server->root_sfc = NULL;
    server->root_sfc_data = NULL;

    server->initialized = false;
}

void vigs_server_dispatch(struct vigs_server *server,
                          uint32_t ram_offset)
{
    vigs_comm_dispatch(server->comm, ram_offset);
}

void vigs_server_update_display(struct vigs_server *server)
{
    struct vigs_surface *root_sfc = server->root_sfc;
    uint32_t sfc_bpp;
    uint32_t display_stride, display_bpp;
    uint8_t *display_data;
    uint8_t *sfc_data = server->root_sfc_data;
    uint32_t i;

    if (!root_sfc) {
        return;
    }

    if (root_sfc->is_dirty) {
        server->backend->batch_start(server->backend);
        root_sfc->read_pixels(root_sfc,
                              0,
                              0,
                              root_sfc->ws_sfc->width,
                              root_sfc->ws_sfc->height,
                              sfc_data);
        server->backend->batch_end(server->backend);
        root_sfc->is_dirty = false;
    }

    sfc_bpp = vigs_format_bpp(root_sfc->format);

    server->display_ops->resize(server->display_user_data,
                                root_sfc->ws_sfc->width,
                                root_sfc->ws_sfc->height);

    display_stride = server->display_ops->get_stride(server->display_user_data);
    display_bpp = server->display_ops->get_bpp(server->display_user_data);
    display_data = server->display_ops->get_data(server->display_user_data);

    if (sfc_bpp != display_bpp) {
        VIGS_LOG_CRITICAL("bpp mismatch");
        assert(false);
        exit(1);
    }

    for (i = 0; i < root_sfc->ws_sfc->height; ++i) {
        uint8_t *src = sfc_data;
        uint8_t *dst = display_data;

        switch (root_sfc->format) {
        case vigsp_surface_bgrx8888:
        case vigsp_surface_bgra8888:
            memcpy(dst, src, root_sfc->ws_sfc->width * 4);
            break;
        default:
            assert(false);
            VIGS_LOG_CRITICAL("unknown format: %d", root_sfc->format);
            exit(1);
        }
        sfc_data += root_sfc->stride;
        display_data += display_stride;
    }
}
