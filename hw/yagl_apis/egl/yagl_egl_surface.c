#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_egl_driver.h"
#include "yagl_compiled_transfer.h"

static void yagl_egl_surface_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_surface *sfc = (struct yagl_egl_surface*)ref;

    g_free(sfc->host_pixels);
    sfc->host_pixels = NULL;

    sfc->dpy->driver_ps->pbuffer_surface_destroy(sfc->dpy->driver_ps,
                                                 sfc->dpy->native_dpy,
                                                 sfc->native_sfc);

    if (sfc->bimage_ct) {
        yagl_compiled_transfer_destroy(sfc->bimage_ct);
    }

    yagl_egl_config_release(sfc->cfg);

    qemu_mutex_destroy(&sfc->bimage_mtx);

    yagl_resource_cleanup(&sfc->res);

    g_free(sfc);
}

static struct yagl_egl_surface
    *yagl_egl_surface_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             EGLenum type,
                             const struct yagl_egl_pbuffer_attribs *pbuffer_attribs,
                             struct yagl_compiled_transfer *bimage_ct,
                             uint32_t width,
                             uint32_t height,
                             uint32_t bpp)
{
    EGLSurface native_sfc;
    struct yagl_egl_surface *sfc;

    native_sfc = dpy->driver_ps->pbuffer_surface_create(dpy->driver_ps,
                                                        dpy->native_dpy,
                                                        &cfg->native,
                                                        width,
                                                        height,
                                                        pbuffer_attribs);

    if (!native_sfc) {
        return NULL;
    }

    sfc = g_malloc0(sizeof(*sfc));

    yagl_resource_init(&sfc->res, &yagl_egl_surface_destroy);

    sfc->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    sfc->cfg = cfg;
    sfc->type = type;
    sfc->bimage_ct = bimage_ct;
    sfc->width = width;
    sfc->height = height;
    sfc->bpp = bpp;
    sfc->host_pixels = g_malloc(width * height * bpp);
    sfc->native_sfc = native_sfc;
    qemu_mutex_init(&sfc->bimage_mtx);

    memcpy(&sfc->native_sfc_attribs, pbuffer_attribs, sizeof(*pbuffer_attribs));

    return sfc;
}

struct yagl_egl_surface
    *yagl_egl_surface_create_window(struct yagl_egl_display *dpy,
                                    struct yagl_egl_config *cfg,
                                    const struct yagl_egl_window_attribs *attribs,
                                    struct yagl_compiled_transfer *bimage_ct,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t bpp)
{
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;
    struct yagl_egl_surface *sfc;

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    sfc = yagl_egl_surface_create(dpy,
                                  cfg,
                                  EGL_WINDOW_BIT,
                                  &pbuffer_attribs,
                                  bimage_ct,
                                  width,
                                  height,
                                  bpp);

    if (sfc) {
        memcpy(&sfc->attribs.window, attribs, sizeof(*attribs));
    }

    return sfc;
}

struct yagl_egl_surface
    *yagl_egl_surface_create_pixmap(struct yagl_egl_display *dpy,
                                    struct yagl_egl_config *cfg,
                                    const struct yagl_egl_pixmap_attribs *attribs,
                                    struct yagl_compiled_transfer *bimage_ct,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t bpp)
{
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;
    struct yagl_egl_surface *sfc;

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    sfc = yagl_egl_surface_create(dpy,
                                  cfg,
                                  EGL_PIXMAP_BIT,
                                  &pbuffer_attribs,
                                  bimage_ct,
                                  width,
                                  height,
                                  bpp);

    if (sfc) {
        memcpy(&sfc->attribs.pixmap, attribs, sizeof(*attribs));
    }

    return sfc;
}

struct yagl_egl_surface
    *yagl_egl_surface_create_pbuffer(struct yagl_egl_display *dpy,
                                     struct yagl_egl_config *cfg,
                                     const struct yagl_egl_pbuffer_attribs *attribs,
                                     struct yagl_compiled_transfer *bimage_ct,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t bpp)
{
    struct yagl_egl_surface *sfc = yagl_egl_surface_create(dpy,
                                                           cfg,
                                                           EGL_PBUFFER_BIT,
                                                           attribs,
                                                           bimage_ct,
                                                           width,
                                                           height,
                                                           bpp);

    if (sfc) {
        memcpy(&sfc->attribs.pbuffer, attribs, sizeof(*attribs));
    }

    return sfc;
}

void yagl_egl_surface_update(struct yagl_egl_surface *sfc,
                             EGLSurface native_sfc,
                             struct yagl_compiled_transfer *bimage_ct,
                             uint32_t width,
                             uint32_t height,
                             uint32_t bpp)
{
    assert(sfc->bimage_ct);

    sfc->dpy->driver_ps->pbuffer_surface_destroy(sfc->dpy->driver_ps,
                                                 sfc->dpy->native_dpy,
                                                 sfc->native_sfc);

    yagl_compiled_transfer_destroy(sfc->bimage_ct);

    sfc->native_sfc = native_sfc;
    sfc->bimage_ct = bimage_ct;
    sfc->width = width;
    sfc->height = height;
    sfc->bpp = bpp;

    g_free(sfc->host_pixels);
    sfc->host_pixels = g_malloc(width * height * bpp);
}

void yagl_egl_surface_invalidate(struct yagl_egl_surface *sfc)
{
    assert(sfc->bimage_ct);

    yagl_compiled_transfer_destroy(sfc->bimage_ct);
    sfc->bimage_ct = NULL;

    sfc->width = 0;
    sfc->height = 0;
    sfc->bpp = 0;
}

void yagl_egl_surface_lock(struct yagl_egl_surface *sfc)
{
    qemu_mutex_lock(&sfc->bimage_mtx);
}

void yagl_egl_surface_unlock(struct yagl_egl_surface *sfc)
{
    qemu_mutex_unlock(&sfc->bimage_mtx);
}

void yagl_egl_surface_acquire(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_acquire(&sfc->res);
    }
}

void yagl_egl_surface_release(struct yagl_egl_surface *sfc)
{
    if (sfc) {
        yagl_resource_release(&sfc->res);
    }
}
