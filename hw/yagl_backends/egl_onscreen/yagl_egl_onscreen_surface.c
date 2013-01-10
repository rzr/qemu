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

static void yagl_egl_onscreen_surface_invalidate(struct yagl_eglb_surface *sfc)
{
    sfc->invalid = true;
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
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;
    GLuint cur_fb = 0;

    egl_onscreen->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                           (GLint*)&cur_fb);

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER,
                                               egl_onscreen_ts->ctx->fb);

    osfc->ws_sfc->swap_buffers(osfc->ws_sfc);

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER, cur_fb);

    return true;
}

static bool yagl_egl_onscreen_surface_copy_buffers(struct yagl_eglb_surface *sfc,
                                                   yagl_winsys_id target)
{
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;
    struct winsys_resource *ws_res = NULL;
    struct winsys_gl_surface *ws_sfc = NULL;
    GLuint cur_fb = 0;
    bool res = false;

    ws_res = egl_onscreen->wsi->acquire_resource(egl_onscreen->wsi, target);

    if (!ws_res) {
        goto out;
    }

    if (ws_res->type != winsys_res_type_pixmap) {
        goto out;
    }

    ws_sfc = (struct winsys_gl_surface*)ws_res->acquire_surface(ws_res);

    if (!ws_sfc) {
        goto out;
    }

    egl_onscreen->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                           (GLint*)&cur_fb);

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER,
                                               egl_onscreen_ts->ctx->fb);

    ws_sfc->copy_buffers(yagl_egl_onscreen_surface_width(osfc),
                         yagl_egl_onscreen_surface_height(osfc),
                         ws_sfc,
                         (ws_sfc == osfc->ws_sfc));

    egl_onscreen->gles_driver->BindFramebuffer(GL_FRAMEBUFFER, cur_fb);

    res = true;

out:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    if (ws_res) {
        ws_res->release(ws_res);
    }

    return res;
}

static void yagl_egl_onscreen_surface_update(struct winsys_resource *res,
                                             void *user_data)
{
    struct yagl_egl_onscreen_surface *sfc = user_data;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_update,
                        "id = 0x%X",
                        res->id);

    sfc->needs_update = true;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_onscreen_surface_destroy(struct yagl_eglb_surface *sfc)
{
    struct yagl_egl_onscreen_surface *osfc =
        (struct yagl_egl_onscreen_surface*)sfc;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_destroy, NULL);

    if (osfc->cookie) {
        osfc->ws_res->remove_callback(osfc->ws_res, osfc->cookie);
        osfc->cookie = NULL;
    }

    egl_onscreen->egl_driver->pbuffer_surface_destroy(
        egl_onscreen->egl_driver,
        ((struct yagl_egl_onscreen_display*)sfc->dpy)->native_dpy,
        osfc->dummy_native_sfc);
    osfc->dummy_native_sfc = EGL_NO_SURFACE;

    if (osfc->ws_sfc) {
        osfc->ws_sfc->base.release(&osfc->ws_sfc->base);
        osfc->ws_sfc = NULL;
    }

    if (osfc->ws_res) {
        osfc->ws_res->release(osfc->ws_res);
        osfc->ws_res = NULL;
    }

    if (osfc->pbuffer_tex || osfc->rb) {
        yagl_ensure_ctx();
        egl_onscreen->gles_driver->DeleteTextures(1, &osfc->pbuffer_tex);
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
    struct winsys_resource *ws_res = NULL;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_window,
                        "dpy = %p, cfg = %d, id = 0x%X",
                        dpy,
                        cfg->config_id,
                        id);

    ws_res = egl_onscreen->wsi->acquire_resource(egl_onscreen->wsi, id);

    if (!ws_res) {
        goto fail;
    }

    if (ws_res->type != winsys_res_type_window) {
        goto fail;
    }

    ws_sfc = (struct winsys_gl_surface*)ws_res->acquire_surface(ws_res);

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
    sfc->ws_res = ws_res;
    sfc->ws_sfc = ws_sfc;

    sfc->base.invalidate = &yagl_egl_onscreen_surface_invalidate;
    sfc->base.query = &yagl_egl_onscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_onscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_onscreen_surface_copy_buffers;
    sfc->base.destroy = &yagl_egl_onscreen_surface_destroy;

    sfc->cookie = ws_res->add_callback(ws_res,
                                       &yagl_egl_onscreen_surface_update,
                                       sfc);

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;

fail:
    if (ws_sfc) {
        ws_sfc->base.release(&ws_sfc->base);
    }

    if (ws_res) {
        ws_res->release(ws_res);
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
    struct winsys_resource *ws_res = NULL;
    struct winsys_gl_surface *ws_sfc = NULL;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_pixmap,
                        "dpy = %p, cfg = %d, id = 0x%X",
                        dpy,
                        cfg->config_id,
                        id);

    ws_res = egl_onscreen->wsi->acquire_resource(egl_onscreen->wsi, id);

    if (!ws_res) {
        goto fail;
    }

    if (ws_res->type != winsys_res_type_pixmap) {
        goto fail;
    }

    ws_sfc = (struct winsys_gl_surface*)ws_res->acquire_surface(ws_res);

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
    sfc->ws_res = ws_res;
    sfc->ws_sfc = ws_sfc;

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

    if (ws_res) {
        ws_res->release(ws_res);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pbuffer(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pbuffer_attribs *attribs,
                                             uint32_t width,
                                             uint32_t height)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct yagl_egl_onscreen_surface *sfc = NULL;
    EGLSurface dummy_native_sfc = EGL_NO_SURFACE;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_surface_create_pbuffer,
                        "dpy = %p, cfg = %d, width = %u, height = %u",
                        dpy,
                        cfg->config_id,
                        width,
                        height);

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
    sfc->pbuffer_width = width;
    sfc->pbuffer_height = height;

    sfc->base.invalidate = &yagl_egl_onscreen_surface_invalidate;
    sfc->base.query = &yagl_egl_onscreen_surface_query;
    sfc->base.swap_buffers = &yagl_egl_onscreen_surface_swap_buffers;
    sfc->base.copy_buffers = &yagl_egl_onscreen_surface_copy_buffers;
    sfc->base.destroy = &yagl_egl_onscreen_surface_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return sfc;

fail:
    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void yagl_egl_onscreen_surface_setup(struct yagl_egl_onscreen_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->base.dpy->backend;
    GLuint cur_rb = 0;
    GLuint cur_tex = 0;

    YAGL_LOG_FUNC_SET(yagl_egl_onscreen_surface_setup);

    if (sfc->needs_update) {
        struct winsys_gl_surface *ws_sfc = NULL;

        ws_sfc = (struct winsys_gl_surface*)sfc->ws_res->acquire_surface(sfc->ws_res);

        if (ws_sfc) {
            if (ws_sfc != sfc->ws_sfc) {
                YAGL_LOG_DEBUG("winsys resource was changed for 0x%X", sfc->ws_res->id);

                if (sfc->rb) {
                    egl_onscreen->gles_driver->DeleteRenderbuffers(1, &sfc->rb);
                    sfc->rb = 0;
                }

                sfc->ws_sfc->base.release(&sfc->ws_sfc->base);
                ws_sfc->base.acquire(&ws_sfc->base);
                sfc->ws_sfc = ws_sfc;
            }

            ws_sfc->base.release(&ws_sfc->base);
        } else {
            YAGL_LOG_WARN("winsys resource was destroyed for 0x%X", sfc->ws_res->id);
        }

        sfc->needs_update = false;
    }

    switch (sfc->base.type) {
    case EGL_PBUFFER_BIT:
        if (!sfc->rb) {
            egl_onscreen->gles_driver->GenRenderbuffers(1, &sfc->rb);

            egl_onscreen->gles_driver->GetIntegerv(GL_RENDERBUFFER_BINDING,
                                                   (GLint*)&cur_rb);

            egl_onscreen->gles_driver->BindRenderbuffer(GL_RENDERBUFFER, sfc->rb);

            egl_onscreen->gles_driver->RenderbufferStorage(GL_RENDERBUFFER,
                                                           GL_DEPTH24_STENCIL8,
                                                           sfc->pbuffer_width,
                                                           sfc->pbuffer_height);

            egl_onscreen->gles_driver->BindRenderbuffer(GL_RENDERBUFFER, cur_rb);
        }
        if (!sfc->pbuffer_tex) {
            egl_onscreen->gles_driver->GenTextures(1, &sfc->pbuffer_tex);

            egl_onscreen->gles_driver->GetIntegerv(GL_TEXTURE_BINDING_2D,
                                                   (GLint*)&cur_tex);

            egl_onscreen->gles_driver->BindTexture(GL_TEXTURE_2D, sfc->pbuffer_tex);

            /*
             * TODO: Use pbuffer attribs to setup texture format
             * correctly.
             */

            egl_onscreen->gles_driver->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                                                  sfc->pbuffer_width,
                                                  sfc->pbuffer_height, 0,
                                                  GL_BGRA, GL_UNSIGNED_INT,
                                                  NULL);

            egl_onscreen->gles_driver->BindTexture(GL_TEXTURE_2D, cur_tex);
        }
        break;
    case EGL_PIXMAP_BIT:
    case EGL_WINDOW_BIT:
        if (!sfc->rb) {
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
        break;
    default:
        assert(false);
        break;
    }
}

void yagl_egl_onscreen_surface_attach_to_framebuffer(struct yagl_egl_onscreen_surface *sfc)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)sfc->base.dpy->backend;

    switch (sfc->base.type) {
    case EGL_PBUFFER_BIT:
        egl_onscreen->gles_driver->FramebufferTexture2D(GL_FRAMEBUFFER,
                                                        GL_COLOR_ATTACHMENT0,
                                                        GL_TEXTURE_2D,
                                                        sfc->pbuffer_tex,
                                                        0);
        break;
    case EGL_PIXMAP_BIT:
        egl_onscreen->gles_driver->FramebufferTexture2D(GL_FRAMEBUFFER,
                                                        GL_COLOR_ATTACHMENT0,
                                                        GL_TEXTURE_2D,
                                                        sfc->ws_sfc->get_front_texture(sfc->ws_sfc),
                                                        0);
        break;
    case EGL_WINDOW_BIT:
        egl_onscreen->gles_driver->FramebufferTexture2D(GL_FRAMEBUFFER,
                                                        GL_COLOR_ATTACHMENT0,
                                                        GL_TEXTURE_2D,
                                                        sfc->ws_sfc->get_back_texture(sfc->ws_sfc),
                                                        0);
        break;
    default:
        assert(false);
        break;
    }
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
    switch (sfc->base.type) {
    case EGL_PBUFFER_BIT:
        return sfc->pbuffer_width;
    case EGL_PIXMAP_BIT:
    case EGL_WINDOW_BIT:
        return sfc->ws_sfc->base.width;
    default:
        assert(false);
        return 0;
    }
}

uint32_t yagl_egl_onscreen_surface_height(struct yagl_egl_onscreen_surface *sfc)
{
    switch (sfc->base.type) {
    case EGL_PBUFFER_BIT:
        return sfc->pbuffer_height;
    case EGL_PIXMAP_BIT:
    case EGL_WINDOW_BIT:
        return sfc->ws_sfc->base.height;
    default:
        assert(false);
        return 0;
    }
}
