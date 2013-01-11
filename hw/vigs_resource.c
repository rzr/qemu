#include "vigs_resource.h"
#include "vigs_ref.h"
#include "vigs_surface.h"
#include "vigs_log.h"
#include "winsys.h"
#include "qemu-queue.h"

/*
 * vigs_winsys_resource
 * @{
 */

struct vigs_winsys_resource_cb
{
    QLIST_ENTRY(vigs_winsys_resource_cb) entry;

    winsys_resource_cb cb;
    void *user_data;
};

struct vigs_winsys_resource
{
    struct winsys_resource base;

    struct vigs_ref ref;

    struct winsys_surface *ws_sfc;

    uint32_t width;

    uint32_t height;

    QLIST_HEAD(, vigs_winsys_resource_cb) callbacks;
};

static void vigs_winsys_resource_acquire(struct winsys_resource *res)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;
    vigs_ref_acquire(&vigs_res->ref);
}

static void vigs_winsys_resource_release(struct winsys_resource *res)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;
    vigs_ref_release(&vigs_res->ref);
}

static void *vigs_winsys_resource_add_callback(struct winsys_resource *res,
                                               winsys_resource_cb cb,
                                               void *user_data)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;
    struct vigs_winsys_resource_cb *vigs_cb;

    vigs_cb = g_malloc0(sizeof(*vigs_cb));

    vigs_cb->cb = cb;
    vigs_cb->user_data = user_data;

    QLIST_INSERT_HEAD(&vigs_res->callbacks, vigs_cb, entry);

    return vigs_cb;
}

static void vigs_winsys_resource_remove_callback(struct winsys_resource *res,
                                                 void *cookie)
{
    struct vigs_winsys_resource_cb *vigs_cb = cookie;
    QLIST_REMOVE(vigs_cb, entry);
    g_free(vigs_cb);
}

static uint32_t vigs_winsys_resource_get_width(struct winsys_resource *res)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;
    return vigs_res->width;
}

static uint32_t vigs_winsys_resource_get_height(struct winsys_resource *res)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;
    return vigs_res->height;
}

static struct winsys_surface
    *vigs_winsys_resource_acquire_surface(struct winsys_resource *res)
{
    struct vigs_winsys_resource *vigs_res = (struct vigs_winsys_resource*)res;

    if (vigs_res->ws_sfc) {
        vigs_res->ws_sfc->acquire(vigs_res->ws_sfc);
    }

    return vigs_res->ws_sfc;
}

static void vigs_winsys_resource_destroy(struct vigs_ref *ref)
{
    struct vigs_winsys_resource *vigs_res =
        container_of(ref, struct vigs_winsys_resource, ref);
    struct vigs_winsys_resource_cb *cb, *next;

    QLIST_FOREACH_SAFE(cb, &vigs_res->callbacks, entry, next) {
        QLIST_REMOVE(cb, entry);
        g_free(cb);
    }

    if (vigs_res->ws_sfc) {
        vigs_res->ws_sfc->release(vigs_res->ws_sfc);
        vigs_res->ws_sfc = NULL;
    }

    vigs_ref_cleanup(&vigs_res->ref);

    g_free(vigs_res);
}

static struct vigs_winsys_resource
    *vigs_winsys_resource_create(winsys_id id,
                                 winsys_res_type type)
{
    struct vigs_winsys_resource *ws_res;

    ws_res = g_malloc0(sizeof(*ws_res));

    ws_res->base.id = id;
    ws_res->base.type = type;
    ws_res->base.acquire = &vigs_winsys_resource_acquire;
    ws_res->base.release = &vigs_winsys_resource_release;
    ws_res->base.add_callback = &vigs_winsys_resource_add_callback;
    ws_res->base.remove_callback = &vigs_winsys_resource_remove_callback;
    ws_res->base.get_width = &vigs_winsys_resource_get_width;
    ws_res->base.get_height = &vigs_winsys_resource_get_height;
    ws_res->base.acquire_surface = &vigs_winsys_resource_acquire_surface;

    vigs_ref_init(&ws_res->ref, &vigs_winsys_resource_destroy);

    QLIST_INIT(&ws_res->callbacks);

    return ws_res;
}

static void vigs_winsys_resource_assign(struct vigs_winsys_resource *ws_res,
                                        struct winsys_surface *ws_sfc,
                                        uint32_t width,
                                        uint32_t height)
{
    struct vigs_winsys_resource_cb *vigs_cb;

    if (ws_res->ws_sfc == ws_sfc) {
        return;
    }

    if (ws_sfc) {
        ws_sfc->acquire(ws_sfc);
    }

    if (ws_res->ws_sfc) {
        ws_res->ws_sfc->release(ws_res->ws_sfc);
    }

    ws_res->ws_sfc = ws_sfc;
    ws_res->width = width;
    ws_res->height = height;

    QLIST_FOREACH(vigs_cb, &ws_res->callbacks, entry) {
        vigs_cb->cb(&ws_res->base, vigs_cb->user_data);
    }
}

/*
 * @}
 */

/*
 * vigs_resource
 * @{
 */

struct vigs_resource
    *vigs_resource_create(vigsp_resource_id id,
                          vigsp_resource_type type)
{
    struct vigs_resource *res;

    res = g_malloc0(sizeof(*res));

    switch (type) {
    case vigsp_resource_window:
        res->ws_res = &vigs_winsys_resource_create(id, winsys_res_type_window)->base;
        break;
    case vigsp_resource_pixmap:
        res->ws_res = &vigs_winsys_resource_create(id, winsys_res_type_pixmap)->base;
        break;
    default:
        assert(false);
        g_free(res);
        return NULL;
    }

    res->id = id;
    res->type = type;

    return res;
}

void vigs_resource_assign(struct vigs_resource *res,
                          struct vigs_surface *sfc,
                          uint32_t width,
                          uint32_t height)
{
    assert(sfc);

    if (res->sfc == sfc) {
        return;
    }

    if (res->sfc) {
        if (!g_hash_table_remove(res->sfc->resource_ids, GUINT_TO_POINTER(res->id))) {
            VIGS_LOG_CRITICAL("no resource id for 0x%X", res->id);
            assert(false);
        }
    }

    g_hash_table_insert(sfc->resource_ids, GUINT_TO_POINTER(res->id), NULL);

    res->sfc = sfc;
    vigs_winsys_resource_assign((struct vigs_winsys_resource*)res->ws_res,
                                sfc->ws_sfc,
                                width,
                                height);
}

void vigs_resource_destroy(struct vigs_resource *res)
{
    struct vigs_winsys_resource *ws_res;

    if (res->sfc) {
        if (!g_hash_table_remove(res->sfc->resource_ids, GUINT_TO_POINTER(res->id))) {
            VIGS_LOG_CRITICAL("no resource id for 0x%X", res->id);
            assert(false);
        }
    }

    ws_res = (struct vigs_winsys_resource*)res->ws_res;

    res->sfc = NULL;
    vigs_winsys_resource_assign(ws_res,
                                NULL,
                                ws_res->width,
                                ws_res->height);

    res->ws_res->release(res->ws_res);
    res->ws_res = NULL;

    g_free(res);
}

/*
 * @}
 */
