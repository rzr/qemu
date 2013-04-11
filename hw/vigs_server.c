#include "vigs_server.h"
#include "vigs_log.h"
#include "vigs_comm.h"
#include "vigs_backend.h"
#include "vigs_surface.h"
#include "vigs_resource.h"
#include "vigs_utils.h"
#ifdef CONFIG_MARU
#include "../tizen/src/hw/maru_brightness.h"
#endif

static void vigs_server_surface_destroy_func(gpointer data)
{
    struct vigs_surface *sfc = data;

    sfc->destroy(sfc);
}

static void vigs_server_resource_destroy_func(gpointer data)
{
    struct vigs_resource *res = data;

    vigs_resource_destroy(res);
}

static struct vigs_surface *vigs_server_get_surface(struct vigs_server *server,
                                                    const struct vigsp_surface *protocol_sfc)
{
    struct vigs_surface *sfc;

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(protocol_sfc->id));

    if (sfc) {
        if (protocol_sfc->vram_offset >= 0) {
            sfc->update(sfc,
                        protocol_sfc->vram_offset,
                        server->vram_ptr + protocol_sfc->vram_offset);
        }

        return sfc;
    }

    VIGS_LOG_ERROR("surface %u not found", protocol_sfc->id);

    return NULL;
}

static void vigs_server_unuse_surface(struct vigs_server *server,
                                      struct vigs_surface *sfc)
{
    GHashTableIter iter;
    gpointer key, value;
    vigsp_resource_id *res_ids;
    int i = 0;

    /*
     * Remove all associated resources.
     */

    vigs_vector_resize(&server->v1,
                       (sizeof(*res_ids) * g_hash_table_size(sfc->resource_ids)));

    res_ids = vigs_vector_data(&server->v1);

    g_hash_table_iter_init(&iter, sfc->resource_ids);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        vigsp_resource_id id = GPOINTER_TO_UINT(key);
        res_ids[i++] = id;
    }

    while (i-- > 0) {
        if (!g_hash_table_remove(server->resources, GUINT_TO_POINTER(res_ids[i]))) {
            VIGS_LOG_CRITICAL("no resource for 0x%X", res_ids[i]);
            assert(false);
        }
    }

    assert(g_hash_table_size(sfc->resource_ids) == 0);

    /*
     * If it was root surface then root surface is now NULL.
     */

    if (server->root_sfc == sfc) {
        server->root_sfc = NULL;
    }
}

static struct winsys_resource
    *vigs_server_acquire_resource(struct winsys_interface *wsi,
                                  winsys_id id)
{
    struct vigs_server *server =
        container_of(wsi, struct vigs_server, wsi);
    struct vigs_resource *res;

    res = g_hash_table_lookup(server->resources, GUINT_TO_POINTER(id));

    if (!res) {
        return NULL;
    }

    res->ws_res->acquire(res->ws_res);

    return res->ws_res;
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

static bool vigs_server_dispatch_reset(void *user_data)
{
    struct vigs_server *server = user_data;
    GHashTableIter iter;
    gpointer key, value;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    g_hash_table_iter_init(&iter, server->surfaces);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        struct vigs_surface *sfc = value;

        if (sfc != server->root_sfc) {
            vigs_server_unuse_surface(server, sfc);
            g_hash_table_iter_remove(&iter);
        }
    }

    return true;
}

static bool vigs_server_dispatch_exit(void *user_data)
{
    struct vigs_server *server = user_data;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_server_reset(server);

    return true;
}

static bool vigs_server_dispatch_create_surface(void *user_data,
                                                uint32_t width,
                                                uint32_t height,
                                                uint32_t stride,
                                                vigsp_surface_format format,
                                                vigsp_offset vram_offset,
                                                vigsp_surface_id *id)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    sfc = server->backend->create_surface(server->backend,
              width,
              height,
              stride,
              format,
              vram_offset,
              ((vram_offset >= 0) ? server->vram_ptr + vram_offset : NULL));

    if (!sfc) {
        return false;
    }

    if (g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc->id))) {
        VIGS_LOG_CRITICAL("surface %u already exists", sfc->id);
        assert(false);
        sfc->destroy(sfc);
        return false;
    }

    g_hash_table_insert(server->surfaces, GUINT_TO_POINTER(sfc->id), sfc);

    *id = sfc->id;

    VIGS_LOG_TRACE("num_surfaces = %u", g_hash_table_size(server->surfaces));

    return true;
}

static bool vigs_server_dispatch_destroy_surface(void *user_data,
                                                 vigsp_surface_id id)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(id));

    if (sfc) {
        vigs_server_unuse_surface(server, sfc);

        g_hash_table_remove(server->surfaces, GUINT_TO_POINTER(id));

        VIGS_LOG_TRACE("num_surfaces = %u", g_hash_table_size(server->surfaces));
        VIGS_LOG_TRACE("num_resources = %u", g_hash_table_size(server->resources));

        return true;
    }

    VIGS_LOG_ERROR("surface %u not found", id);

    return false;
}

static bool vigs_server_dispatch_set_root_surface(void *user_data,
                                                  vigsp_surface_id id)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    if (id == 0) {
        server->root_sfc = NULL;

        VIGS_LOG_TRACE("root surface reset");

        return true;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(id));

    if (sfc) {
        if (!sfc->data) {
            VIGS_LOG_ERROR("surface %u has no data, cannot set it as root surface", id);
            return false;
        }

        server->root_sfc = sfc;
        return true;
    }

    VIGS_LOG_ERROR("surface %u not found", id);

    return false;
}

static bool vigs_server_dispatch_copy(void *user_data,
                                      const struct vigsp_surface *src,
                                      const struct vigsp_surface *dst,
                                      const struct vigsp_copy *entries,
                                      uint32_t num_entries)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_src;
    struct vigs_surface *vigs_dst;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_src = vigs_server_get_surface(server, src);

    if (!vigs_src) {
        return false;
    }

    if (src->id == dst->id) {
        vigs_dst = vigs_src;
    } else {
        vigs_dst = vigs_server_get_surface(server, dst);

        if (!vigs_dst) {
            return false;
        }
    }

    vigs_dst->copy(vigs_dst, vigs_src, entries, num_entries);

    return true;
}

static bool vigs_server_dispatch_solid_fill(void *user_data,
                                            const struct vigsp_surface *sfc,
                                            vigsp_color color,
                                            const struct vigsp_rect *entries,
                                            uint32_t num_entries)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_sfc = vigs_server_get_surface(server, sfc);

    if (!vigs_sfc) {
        return false;
    }

    vigs_sfc->solid_fill(vigs_sfc, color, entries, num_entries);

    return true;
}

static bool vigs_server_dispatch_update_vram(void *user_data,
                                             const struct vigsp_surface *sfc)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc->id));

    if (vigs_sfc) {
        assert(sfc->vram_offset >= 0);
        vigs_sfc->set_data(vigs_sfc,
                           sfc->vram_offset,
                           server->vram_ptr + sfc->vram_offset);

        vigs_surface_update_vram(vigs_sfc);

        return true;
    }

    VIGS_LOG_ERROR("surface %u not found", sfc->id);

    return false;
}

static bool vigs_server_dispatch_put_image(void *user_data,
                                           const struct vigsp_surface *sfc,
                                           target_ulong src_va,
                                           uint32_t src_stride,
                                           const struct vigsp_rect *rect,
                                           bool *is_pf)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;
    uint32_t size = src_stride * rect->size.h;
    int ret;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_vector_resize(&server->v1, size);

    ret = cpu_memory_rw_debug(cpu_single_env,
                              src_va,
                              vigs_vector_data(&server->v1),
                              size,
                              0);

    if (ret == -1) {
        VIGS_LOG_WARN("page fault at 0x%X:%u", (uint32_t)src_va, size);
        *is_pf = true;
        return true;
    }

    vigs_sfc = vigs_server_get_surface(server, sfc);

    if (!vigs_sfc) {
        return false;
    }

    vigs_sfc->put_image(vigs_sfc,
                        vigs_vector_data(&server->v1),
                        src_stride,
                        rect);

    *is_pf = false;

    return true;
}

static bool vigs_server_dispatch_get_image(void *user_data,
                                           vigsp_surface_id sfc_id,
                                           target_ulong dst_va,
                                           uint32_t dst_stride,
                                           const struct vigsp_rect *rect,
                                           bool *is_pf)
{
    struct vigs_server *server = user_data;
    struct vigs_surface *vigs_sfc;
    uint32_t size = dst_stride * rect->size.h;
    int ret;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    vigs_vector_resize(&server->v1, size);

    vigs_sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc_id));

    if (!vigs_sfc) {
        VIGS_LOG_ERROR("surface %u not found", sfc_id);
        return false;
    }

    vigs_sfc->read_pixels(vigs_sfc, rect->pos.x, rect->pos.y,
                          rect->size.w, rect->size.h, dst_stride,
                          vigs_vector_data(&server->v1));

    ret = cpu_memory_rw_debug(cpu_single_env,
                              dst_va,
                              vigs_vector_data(&server->v1),
                              size,
                              1);

    if (ret == -1) {
        VIGS_LOG_WARN("page fault at 0x%X:%u", (uint32_t)dst_va, size);
        *is_pf = true;
        return true;
    }

    *is_pf = false;

    return true;
}

static bool vigs_server_dispatch_assign_resource(void *user_data,
                                                 vigsp_resource_id res_id,
                                                 vigsp_resource_type res_type,
                                                 vigsp_surface_id sfc_id)
{
    struct vigs_server *server = user_data;
    struct vigs_resource *res;
    struct vigs_surface *sfc;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    sfc = g_hash_table_lookup(server->surfaces, GUINT_TO_POINTER(sfc_id));

    if (!sfc) {
        VIGS_LOG_ERROR("surface %u not found", sfc_id);
        return false;
    }

    res = g_hash_table_lookup(server->resources, GUINT_TO_POINTER(res_id));

    if (res) {
        /*
         * Update resource.
         */

        if (res->type != res_type) {
            VIGS_LOG_ERROR("resource type mismatch");
            return false;
        }

        if (res->type != vigsp_resource_window) {
            VIGS_LOG_ERROR("cannot reassign resource other than window");
            return false;
        }
    } else {
        /*
         * Create resource.
         */

        res = vigs_resource_create(res_id, res_type);
        assert(res);

        if (!res) {
            VIGS_LOG_CRITICAL("cannot create resource");
            return false;
        }

        g_hash_table_insert(server->resources, GUINT_TO_POINTER(res_id), res);

        VIGS_LOG_TRACE("num_resources = %u", g_hash_table_size(server->resources));
    }

    vigs_resource_assign(res, sfc);

    return true;
}

static bool vigs_server_dispatch_destroy_resource(void *user_data,
                                                  vigsp_resource_id res_id)
{
    struct vigs_server *server = user_data;

    if (!server->initialized) {
        VIGS_LOG_ERROR("not initialized");
        return false;
    }

    if (!g_hash_table_remove(server->resources, GUINT_TO_POINTER(res_id))) {
        /*
         * This can happen with old X.Org servers, this is normal.
         */
        VIGS_LOG_WARN("resource 0x%X not found", res_id);
    }

    VIGS_LOG_TRACE("num_resources = %u", g_hash_table_size(server->resources));

    return true;
}

static struct vigs_comm_ops vigs_server_dispatch_ops =
{
    .init = &vigs_server_dispatch_init,
    .reset = &vigs_server_dispatch_reset,
    .exit = &vigs_server_dispatch_exit,
    .create_surface = &vigs_server_dispatch_create_surface,
    .destroy_surface = &vigs_server_dispatch_destroy_surface,
    .set_root_surface = &vigs_server_dispatch_set_root_surface,
    .copy = &vigs_server_dispatch_copy,
    .solid_fill = &vigs_server_dispatch_solid_fill,
    .update_vram = &vigs_server_dispatch_update_vram,
    .put_image = &vigs_server_dispatch_put_image,
    .get_image = &vigs_server_dispatch_get_image,
    .assign_resource = &vigs_server_dispatch_assign_resource,
    .destroy_resource = &vigs_server_dispatch_destroy_resource,
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
    server->wsi.acquire_resource = &vigs_server_acquire_resource;

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

    server->resources = g_hash_table_new_full(g_direct_hash,
                                              g_direct_equal,
                                              NULL,
                                              vigs_server_resource_destroy_func);

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
    g_hash_table_destroy(server->resources);
    vigs_comm_destroy(server->comm);
    vigs_vector_cleanup(&server->v1);
    server->backend->destroy(server->backend);
    g_free(server);
}

void vigs_server_reset(struct vigs_server *server)
{
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, server->surfaces);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        struct vigs_surface *sfc = value;

        vigs_server_unuse_surface(server, sfc);

        g_hash_table_iter_remove(&iter);
    }

    /*
     * There can be no resources without surfaces.
     */
    assert(g_hash_table_size(server->resources) == 0);

    server->root_sfc = NULL;

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
    uint8_t *sfc_data;
    uint32_t i, j;

    if (!root_sfc) {
        return;
    }

    vigs_surface_update_vram(root_sfc);

    sfc_bpp = vigs_format_bpp(root_sfc->format);

    server->display_ops->resize(server->display_user_data,
                                root_sfc->width,
                                root_sfc->height);

    display_stride = server->display_ops->get_stride(server->display_user_data);
    display_bpp = server->display_ops->get_bpp(server->display_user_data);
    display_data = server->display_ops->get_data(server->display_user_data);

    if (sfc_bpp != display_bpp) {
        VIGS_LOG_CRITICAL("bpp mismatch");
        assert(false);
        exit(1);
    }

    sfc_data = root_sfc->data;

    for (i = 0; i < root_sfc->height; ++i) {
        uint8_t *src = sfc_data;
        uint8_t *dst = display_data;

        switch (root_sfc->format) {
        case vigsp_surface_bgrx8888:
        case vigsp_surface_bgra8888:
#ifdef CONFIG_MARU
            if (brightness_level < BRIGHTNESS_MAX) {
                uint32_t level = brightness_tbl[brightness_level];
                for (j = 0; j < root_sfc->width; ++j) {
                    *dst++ = ((uint32_t)(*src++) * level) >> 8;
                    *dst++ = ((uint32_t)(*src++) * level) >> 8;
                    *dst++ = ((uint32_t)(*src++) * level) >> 8;
                    *dst++ = ((uint32_t)(*src++) * level) >> 8;
                }
            } else {
#endif
            memcpy(dst, src, root_sfc->width * 4);
#ifdef CONFIG_MARU
            }
#endif
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
