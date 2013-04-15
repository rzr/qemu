#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_ps.h"
#include "yagl_client_context.h"

static void yagl_egl_offscreen_context_destroy(struct yagl_eglb_context *ctx)
{
}

struct yagl_egl_offscreen_context
    *yagl_egl_offscreen_context_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       struct yagl_client_context *client_ctx,
                                       struct yagl_egl_offscreen_context *share_context)
{
    struct yagl_egl_offscreen_ps *egl_offscreen_ps =
        (struct yagl_egl_offscreen_ps*)dpy->base.backend_ps;
    struct yagl_egl_offscreen_context *ctx;
    EGLContext native_ctx;

    native_ctx = egl_offscreen_ps->driver_ps->context_create(
        egl_offscreen_ps->driver_ps,
        dpy->native_dpy,
        cfg,
        client_ctx->client_api,
        (share_context ? share_context->native_ctx : EGL_NO_CONTEXT));

    if (!native_ctx) {
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_eglb_context_init(&ctx->base, &dpy->base, client_ctx);

    ctx->base.destroy = &yagl_egl_offscreen_context_destroy;

    ctx->native_ctx = native_ctx;

    return ctx;
}
