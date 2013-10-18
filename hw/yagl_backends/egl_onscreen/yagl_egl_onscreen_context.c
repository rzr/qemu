#include <GL/gl.h>
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_display.h"
#include "yagl_egl_onscreen.h"
#include "yagl_egl_surface_attribs.h"
#include "yagl_gles_driver.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_egl_onscreen_context_destroy(struct yagl_eglb_context *ctx)
{
    struct yagl_egl_onscreen_context *egl_onscreen_ctx =
        (struct yagl_egl_onscreen_context*)ctx;
    struct yagl_egl_onscreen_display *dpy =
        (struct yagl_egl_onscreen_display*)ctx->dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)ctx->dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_context_destroy, NULL);

    if (egl_onscreen_ctx->fb) {
        yagl_ensure_ctx();
        egl_onscreen->gles_driver->DeleteFramebuffers(1, &egl_onscreen_ctx->fb);
        yagl_unensure_ctx();
    }

    if (egl_onscreen_ctx->null_sfc != EGL_NO_SURFACE) {
        egl_onscreen->egl_driver->pbuffer_surface_destroy(
            egl_onscreen->egl_driver,
            dpy->native_dpy,
            egl_onscreen_ctx->null_sfc);
        egl_onscreen_ctx->null_sfc = EGL_NO_SURFACE;
    }

    egl_onscreen->egl_driver->context_destroy(egl_onscreen->egl_driver,
                                              dpy->native_dpy,
                                              egl_onscreen_ctx->native_ctx);

    yagl_eglb_context_cleanup(ctx);

    g_free(egl_onscreen_ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_onscreen_context
    *yagl_egl_onscreen_context_create(struct yagl_egl_onscreen_display *dpy,
                                      const struct yagl_egl_native_config *cfg,
                                      struct yagl_egl_onscreen_context *share_context)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->base.backend;
    struct yagl_egl_onscreen_context *ctx;
    EGLContext native_ctx;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_context_create,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    native_ctx = egl_onscreen->egl_driver->context_create(
        egl_onscreen->egl_driver,
        dpy->native_dpy,
        cfg,
        egl_onscreen->global_ctx);

    if (!native_ctx) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_eglb_context_init(&ctx->base, &dpy->base);

    ctx->base.destroy = &yagl_egl_onscreen_context_destroy;

    ctx->native_ctx = native_ctx;

    memcpy(&ctx->cfg, cfg, sizeof(*cfg));

    ctx->null_sfc = EGL_NO_SURFACE;

    YAGL_LOG_FUNC_EXIT(NULL);

    return ctx;
}

void yagl_egl_onscreen_context_setup(struct yagl_egl_onscreen_context *ctx)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)ctx->base.dpy->backend;

    if (ctx->fb) {
        return;
    }

    egl_onscreen->gles_driver->GenFramebuffers(1, &ctx->fb);
}

bool yagl_egl_onscreen_context_setup_surfaceless(struct yagl_egl_onscreen_context *ctx)
{
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)ctx->base.dpy->backend;
    struct yagl_egl_onscreen_display *dpy =
        (struct yagl_egl_onscreen_display*)ctx->base.dpy;
    struct yagl_egl_pbuffer_attribs pbuffer_attribs;

    if (ctx->null_sfc != EGL_NO_SURFACE) {
        return true;
    }

    yagl_egl_pbuffer_attribs_init(&pbuffer_attribs);

    ctx->null_sfc = egl_onscreen->egl_driver->pbuffer_surface_create(egl_onscreen->egl_driver,
                                                                     dpy->native_dpy,
                                                                     &ctx->cfg,
                                                                     1,
                                                                     1,
                                                                     &pbuffer_attribs);

    return ctx->null_sfc != EGL_NO_SURFACE;
}
