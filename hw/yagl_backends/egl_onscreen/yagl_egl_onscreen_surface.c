#include <GL/gl.h>
#include "yagl_egl_onscreen_surface.h"
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_display.h"
#include "yagl_egl_onscreen_ts.h"
#include "yagl_egl_onscreen.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_gles_driver.h"
#include "winsys_gl.h"

YAGL_DECLARE_TLS(struct yagl_egl_onscreen_ts*, egl_onscreen_ts);

static void yagl_egl_onscreen_surface_reset(struct yagl_eglb_surface *sfc)
{
}

static void yagl_egl_onscreen_surface_invalidate(struct yagl_eglb_surface *sfc,
                                                 yagl_winsys_id id)
{
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;
    struct winsys_gl_surface *ws_sfc = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_invalidate, NULL);

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, id);

    if (!ws_sfc) {
        YAGL_LOG_WARN("winsys surface %u not found", id);
        YAGL_LOG_FUNC_EXIT(NULL);
        return;
    }

    if (ws_sfc == osfc->ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
        YAGL_LOG_FUNC_EXIT(NULL);
        return;
    }

    YAGL_LOG_DEBUG("winsys surface changed to %u", id);

    if (osfc->rb) {
        egl_onscreen->gles_driver->DeleteRenderbuffers(1, &osfc->rb);
        osfc->rb = 0;
    }

    osfc->ws_sfc->base.release(&osfc->ws_sfc->base);
    osfc->ws_sfc = ws_sfc;

    if (egl_onscreen_ts->sfc_draw == osfc) {
        yagl_egl_onscreen_surface_setup(osfc);
        yagl_egl_onscreen_surface_attach_to_framebuffer(osfc);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_egl_onscreen_surface_query(struct yagl_eglb_surface *sfc,
                                             EGLint attribute,
                                             EGLint *value)
{
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;

    switch (attribute) {
    case EGL_HEIGHT:
        *value = yagl_egl_onscreen_surface_height(osfc);
        break;
    case EGL_WIDTH:
        *value = yagl_egl_onscreen_surface_width(osfc);
        break;
    default:
        return false;
    }

    return true;
}

static bool yagl_egl_onscreen_surface_swap_buffers(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;

    egl_onscreen->gles_driver->Finish();

    return true;
}

static bool yagl_egl_onscreen_surface_copy_buffers(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;

    egl_onscreen->gles_driver->Finish();

    return true;
}

static void yagl_egl_onscreen_surface_destroy(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_destroy, NULL);

    egl_onscreen->egl_driver->pbuffer_surface_destroy(
        egl_onscreen->egl_driver,
        ((struct yagl_egl_onscreen_display*)sfc->dpy)->native_dpy,
        osfc->dummy_native_sfc);
    osfc->dummy_native_sfc = EGL_NO_SURFACE;

    if (osfc->ws_sfc) {
        osfc->ws_sfc->base.release(&osfc->ws_sfc->base);
        osfc->ws_sfc = NULL;
    }

    if (osfc->rb) {
        yagl_ensure_ctx();
        egl_onscreen->gles_driver->DeleteRenderbuffers(1, &osfc->rb);
        yagl_unensure_ctx();
    }

    yagl_eglb_surface_cleanup(sfc);

    g_free(osfc);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_window(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_window_attribs *attribs,
                                             yagl_winsys_id id)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_window,
                        "dpy = %p, cfg = %d, id = %u",
                        dpy,
                        cfg->config_id,
                        id);

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, id);

    if (!ws_sfc) {
        goto fail;
    }

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    dummy_native_sfc = egl_onscreen->egl_driver->pbuffer_surface_create(egl_onscreen->egl_driver,
                                                                        dpy->native_dpy,
                                                                        cfg,
                                                                        1,
                                                                        1,
                                                                        &pbuffer_attribs);

    if (!dummy_native_sfc) {
        goto fail;
    }

    sfc = g_malloc0(sizeof(*sfc));

    yagl_eglb_surface_init(&sfc->base, &dpy->base, EGL_WINDOW_BIT, attribs);

    sfc->dummy_native_sfc = dummy_native_sfc;
    sfc->ws_sfc = ws_sfc;

    sfc->base.reset = &yagl_egl_onscreen_surface_reset;
    sfc->base.invalidate = &yagl_egl_onscreen_surface_invalidate;
    sfc->base.query = &yagl_egl_onscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_onscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_onscreen_surface_copy_buffers;
    sfc->base.destroy = &yagl_egl_onscreen_surface_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;

fail:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pixmap(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pixmap_attribs *attribs,
                                             yagl_winsys_id id)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_pixmap,
                        "dpy = %p, cfg = %d, id = %u",
                        dpy,
                        cfg->config_id,
                        id);

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, id);

    if (!ws_sfc) {
        goto fail;
    }

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    dummy_native_sfc = egl_onscreen->egl_driver->pbuffer_surface_create(egl_onscreen->egl_driver,
                                                                        dpy->native_dpy,
                                                                        cfg,
                                                                        1,
                                                                        1,
                                                                        &pbuffer_attribs);

    if (!dummy_native_sfc) {
        goto fail;
    }

    sfc = g_malloc0(sizeof(*sfc));

    yagl_eglb_surface_init(&sfc->base, &dpy->base, EGL_PIXMAP_BIT, attribs);

    sfc->dummy_native_sfc = dummy_native_sfc;
    sfc->ws_sfc = ws_sfc;

    sfc->base.reset = &yagl_egl_onscreen_surface_reset;
    sfc->base.invalidate = &yagl_egl_onscreen_surface_invalidate;
    sfc->base.query = &yagl_egl_onscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_onscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_onscreen_surface_copy_buffers;
    sfc->base.destroy = &yagl_egl_onscreen_surface_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;

fail:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pbuffer(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pbuffer_attribs *attribs,
                                             yagl_winsys_id id)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_pbuffer,
                        "dpy = %p, cfg = %d, id = %u",
                        dpy,
                        cfg->config_id,
                        id);

    ws_sfc = (struct winsys_gl_surface*)egl_onscreen->wsi->acquire_surface(egl_onscreen->wsi, id);

    if (!ws_sfc) {
        goto fail;
    }

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    dummy_native_sfc = egl_onscreen->egl_driver->pbuffer_surface_create(egl_onscreen->egl_driver,
                                                                        dpy->native_dpy,
                                                                        cfg,
                                                                        1,
                                                                        1,
                                                                        &pbuffer_attribs);

    if (!dummy_native_sfc) {
        goto fail;
    }

    sfc = g_malloc0(sizeof(*sfc));

    yagl_eglb_surface_init(&sfc->base, &dpy->base, EGL_PBUFFER_BIT, attribs);

    sfc->dummy_native_sfc = dummy_native_sfc;
    sfc->ws_sfc = ws_sfc;

    sfc->base.reset = &yagl_egl_onscreen_surface_reset;
    sfc->base.invalidate = &yagl_egl_onscreen_surface_invalidate;
    sfc->base.query = &yagl_egl_onscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_onscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_onscreen_surface_copy_buffers;
    sfc->base.destroy = &yagl_egl_onscreen_surface_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;

fail:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void yagl_egl_onscreen_surface_setup(struct yagl_egl_onscreen_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->base.dpy->backend;
    GLuint cur_rb = 0;

    if (sfc->rb) {
        return;
    }

    egl_onscreen->gles_driver->GenRenderbuffers(1, &sfc->rb);

    egl_onscreen->gles_driver->GetIntegerv(GL_RENDERBUFFER_BINDING,
                                           (GLint*)&cur_rb);

    egl_onscreen->gles_driver->BindRenderbuffer(GL_RENDERBUFFER, sfc->rb);

    egl_onscreen->gles_driver->RenderbufferStorage(GL_RENDERBUFFER,
                                                   GL_DEPTH24_STENCIL8,
                                                   sfc->ws_sfc->base.width,
                                                   sfc->ws_sfc->base.height);

    egl_onscreen->gles_driver->BindRenderbuffer(GL_RENDERBUFFER, cur_rb);
}

void yagl_egl_onscreen_surface_attach_to_framebuffer(struct yagl_egl_onscreen_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->base.dpy->backend;

    egl_onscreen->gles_driver->FramebufferTexture2D(GL_FRAMEBUFFER,
                                                    GL_COLOR_ATTACHMENT0,
                                                    GL_TEXTURE_2D,
                                                    sfc->ws_sfc->get_texture(sfc->ws_sfc),
                                                    0);
    egl_onscreen->gles_driver->FramebufferRenderbuffer(GL_FRAMEBUFFER,
                                                       GL_DEPTH_ATTACHMENT,
                                                       GL_RENDERBUFFER,
                                                       sfc->rb);
    egl_onscreen->gles_driver->FramebufferRenderbuffer(GL_FRAMEBUFFER,
                                                       GL_STENCIL_ATTACHMENT,
                                                       GL_RENDERBUFFER,
                                                       sfc->rb);
}

uint32_t yagl_egl_onscreen_surface_width(struct yagl_egl_onscreen_surface *sfc)
{
    return sfc->ws_sfc->base.width;
}

uint32_t yagl_egl_onscreen_surface_height(struct yagl_egl_onscreen_surface *sfc)
{
    return sfc->ws_sfc->base.height;
}
