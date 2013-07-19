#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_client_context.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_egl_offscreen_context_destroy(struct yagl_eglb_context *ctx)
{
    struct yagl_egl_offscreen_context *egl_offscreen_ctx =
        (struct yagl_egl_offscreen_context*)ctx;
    struct yagl_egl_offscreen_display *dpy =
        (struct yagl_egl_offscreen_display*)ctx->dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)ctx->dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_context_destroy, NULL);

    egl_offscreen->driver->context_destroy(egl_offscreen->driver,
                                           dpy->native_dpy,
                                           egl_offscreen_ctx->native_ctx);

    yagl_eglb_context_cleanup(ctx);

    g_free(egl_offscreen_ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_context
    *yagl_egl_offscreen_context_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       struct yagl_client_context *client_ctx,
                                       struct yagl_egl_offscreen_context *share_context)
{
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->base.backend;
    struct yagl_egl_offscreen_context *ctx;
    EGLContext native_ctx;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_context_create,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    native_ctx = egl_offscreen->driver->context_create(
        egl_offscreen->driver,
        dpy->native_dpy,
        cfg,
        client_ctx->client_api,
        egl_offscreen->global_ctx);

    if (!native_ctx) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_eglb_context_init(&ctx->base, &dpy->base, client_ctx);

    ctx->base.destroy = &yagl_egl_offscreen_context_destroy;

    ctx->native_ctx = native_ctx;

    YAGL_LOG_FUNC_EXIT(NULL);

    return ctx;
}
