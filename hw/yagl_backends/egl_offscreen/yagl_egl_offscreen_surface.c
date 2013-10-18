#include <GL/gl.h>
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_tls.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_compiled_transfer.h"

YAGL_DECLARE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static void yagl_egl_offscreen_surface_cleanup(struct yagl_egl_offscreen_surface *sfc)
{
    struct yagl_egl_offscreen_display *dpy =
        (struct yagl_egl_offscreen_display*)sfc->base.dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)sfc->base.dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_surface_cleanup, NULL);

    g_free(sfc->host_pixels);
    sfc->host_pixels = NULL;

    egl_offscreen->egl_driver->pbuffer_surface_destroy(egl_offscreen->egl_driver,
                                                       dpy->native_dpy,
                                                       sfc->native_sfc);
    sfc->native_sfc = EGL_NO_SURFACE;

    if (sfc->bimage_ct) {
        yagl_compiled_transfer_destroy(sfc->bimage_ct);
        sfc->bimage_ct = NULL;
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_offscreen_surface_invalidate(struct yagl_eglb_surface *sfc,
                                                  yagl_winsys_id id)
{
}

static void yagl_egl_offscreen_surface_replace(struct yagl_eglb_surface *sfc,
                                               struct yagl_eglb_surface *with)
{
    struct yagl_egl_offscreen_surface *osfc =
        (struct yagl_egl_offscreen_surface*)sfc;
    struct yagl_egl_offscreen_surface *owith =
        (struct yagl_egl_offscreen_surface*)with;

    yagl_egl_offscreen_surface_cleanup(osfc);

    osfc->bimage_ct = owith->bimage_ct;
    osfc->width = owith->width;
    osfc->height = owith->height;
    osfc->bpp = owith->bpp;
    osfc->host_pixels = owith->host_pixels;
    osfc->native_sfc = owith->native_sfc;
    osfc->native_sfc_attribs = owith->native_sfc_attribs;

    yagl_eglb_surface_cleanup(with);

    g_free(owith);
}

static bool yagl_egl_offscreen_surface_query(struct yagl_eglb_surface *sfc,
                                             EGLint attribute,
                                             EGLint *value)
{
    struct yagl_egl_offscreen_surface *osfc =
        (struct yagl_egl_offscreen_surface*)sfc;

    switch (attribute) {
    case EGL_HEIGHT:
        *value = osfc->height;
        break;
    case EGL_WIDTH:
        *value = osfc->width;
        break;
    default:
        return false;
    }

    return true;
}

static bool yagl_egl_offscreen_surface_swap_buffers(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_offscreen_surface *osfc =
        (struct yagl_egl_offscreen_surface*)sfc;
    struct yagl_egl_offscreen_context *octx = egl_offscreen_ts->ctx;

    YAGL_LOG_FUNC_SET(yagl_egl_offscreen_surface_swap_buffers);

    assert(octx);

    if (!yagl_egl_offscreen_context_read_pixels(octx,
                                                osfc->width,
                                                osfc->height,
                                                osfc->bpp,
                                                osfc->host_pixels)) {
        YAGL_LOG_ERROR("read_pixels failed");
        return false;
    }

    assert(osfc->bimage_ct);

    yagl_compiled_transfer_exec(osfc->bimage_ct, osfc->host_pixels);

    return true;
}

static bool yagl_egl_offscreen_surface_copy_buffers(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_offscreen_surface *osfc =
        (struct yagl_egl_offscreen_surface*)sfc;
    struct yagl_egl_offscreen_context *octx = egl_offscreen_ts->ctx;

    YAGL_LOG_FUNC_SET(yagl_egl_offscreen_surface_copy_buffers);

    assert(octx);

    if (!yagl_egl_offscreen_context_read_pixels(octx,
                                                osfc->width,
                                                osfc->height,
                                                osfc->bpp,
                                                osfc->host_pixels)) {
        YAGL_LOG_ERROR("read_pixels failed");
        return false;
    }

    assert(osfc->bimage_ct);

    yagl_compiled_transfer_exec(osfc->bimage_ct, osfc->host_pixels);

    return true;
}

static void yagl_egl_offscreen_surface_wait_gl(struct yagl_eglb_surface *sfc)
{
}

static void yagl_egl_offscreen_surface_destroy(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_offscreen_surface *egl_offscreen_sfc =
        (struct yagl_egl_offscreen_surface*)sfc;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_surface_destroy, NULL);

    yagl_egl_offscreen_surface_cleanup(egl_offscreen_sfc);

    yagl_eglb_surface_cleanup(sfc);

    g_free(egl_offscreen_sfc);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_surface
    *yagl_egl_offscreen_surface_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       EGLenum type,
                                       const void *attribs,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t bpp,
                                       target_ulong pixels)
{
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->base.backend;
    struct yagl_egl_offscreen_surface *sfc;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;
    EGLSurface native_sfc;
    struct yagl_compiled_transfer *bimage_ct;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_surface_create,
                        "dpy = %p, cfg = %d, type = %u, width = %u, height = %u, bpp = %u",
                        dpy,
                        cfg->config_id,
                        type,
                        width,
                        height,
                        bpp);

    switch (type) {
    case EGL_PBUFFER_BIT:
        memcpy(&pbuffer_attribs, attribs, sizeof(pbuffer_attribs));
        break;
    case EGL_PIXMAP_BIT:
    case EGL_WINDOW_BIT:
        yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);
        break;
    default:
        assert(0);
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    bimage_ct = yagl_compiled_transfer_create(pixels,
                                              (width * height * bpp),
                                              true);

    if (!bimage_ct) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    native_sfc = egl_offscreen->egl_driver->pbuffer_surface_create(egl_offscreen->egl_driver,
                                                                   dpy->native_dpy,
                                                                   cfg,
                                                                   width,
                                                                   height,
                                                                   &pbuffer_attribs);

    if (!native_sfc) {
        yagl_compiled_transfer_destroy(bimage_ct);
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    sfc = g_malloc0(sizeof(*sfc));

    yagl_eglb_surface_init(&sfc->base, &dpy->base, type, attribs);

    sfc->bimage_ct = bimage_ct;
    sfc->width = width;
    sfc->height = height;
    sfc->bpp = bpp;
    sfc->host_pixels = g_malloc(width * height * bpp);
    sfc->native_sfc = native_sfc;
    memcpy(&sfc->native_sfc_attribs, &pbuffer_attribs, sizeof(pbuffer_attribs));

    sfc->base.invalidate = &yagl_egl_offscreen_surface_invalidate;
    sfc->base.replace = &yagl_egl_offscreen_surface_replace;
    sfc->base.query = &yagl_egl_offscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_offscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_offscreen_surface_copy_buffers;
    sfc->base.wait_gl = &yagl_egl_offscreen_surface_wait_gl;
    sfc->base.destroy = &yagl_egl_offscreen_surface_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;
}
