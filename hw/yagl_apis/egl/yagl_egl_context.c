#include "yagl_egl_context.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_egl_driver.h"
#include "yagl_sharegroup.h"
#include "yagl_client_context.h"

static void yagl_egl_context_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_context *ctx = (struct yagl_egl_context*)ref;
    struct yagl_client_context *client_ctx = ctx->client_ctx;

    yagl_egl_context_update_surfaces(ctx, NULL, NULL);

    yagl_egl_config_release(ctx->cfg);

    ctx->dpy->driver_ps->context_destroy(ctx->dpy->driver_ps,
                                         ctx->dpy->native_dpy,
                                         ctx->native_ctx);

    yagl_resource_cleanup(&ctx->res);

    g_free(ctx);

    client_ctx->destroy(client_ctx);
}

struct yagl_egl_context
    *yagl_egl_context_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_client_context *client_ctx,
                             EGLContext native_share_ctx)
{
    EGLContext native_ctx;
    struct yagl_egl_context *ctx;

    native_ctx = dpy->driver_ps->context_create(dpy->driver_ps,
                                                dpy->native_dpy,
                                                &cfg->native,
                                                client_ctx->client_api,
                                                native_share_ctx);

    if (!native_ctx) {
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_resource_init(&ctx->res, &yagl_egl_context_destroy);

    ctx->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    ctx->cfg = cfg;
    ctx->client_ctx = client_ctx;
    ctx->native_ctx = native_ctx;
    ctx->read = NULL;
    ctx->draw = NULL;

    return ctx;
}

void yagl_egl_context_update_surfaces(struct yagl_egl_context *ctx,
                                      struct yagl_egl_surface *draw,
                                      struct yagl_egl_surface *read)
{
    struct yagl_egl_surface *tmp_draw, *tmp_read;

    yagl_egl_surface_acquire(draw);
    yagl_egl_surface_acquire(read);

    tmp_draw = ctx->draw;
    tmp_read = ctx->read;

    ctx->draw = draw;
    ctx->read = read;

    yagl_egl_surface_release(tmp_draw);
    yagl_egl_surface_release(tmp_read);
}

bool yagl_egl_context_uses_surface(struct yagl_egl_context *ctx,
                                   struct yagl_egl_surface *sfc)
{
    return sfc && ((ctx->read == sfc) || (ctx->draw == sfc));
}

void yagl_egl_context_acquire(struct yagl_egl_context *ctx)
{
    if (ctx) {
        yagl_resource_acquire(&ctx->res);
    }
}

void yagl_egl_context_release(struct yagl_egl_context *ctx)
{
    if (ctx) {
        yagl_resource_release(&ctx->res);
    }
}
