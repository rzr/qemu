#include "yagl_egl_context.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_eglb_context.h"
#include "yagl_eglb_display.h"
#include "yagl_sharegroup.h"
#include "yagl_client_context.h"

static void yagl_egl_context_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_context *ctx = (struct yagl_egl_context*)ref;
    struct yagl_client_context *client_ctx = ctx->backend_ctx->client_ctx;

    assert(!ctx->draw);
    assert(!ctx->read);

    ctx->backend_ctx->destroy(ctx->backend_ctx);

    yagl_egl_config_release(ctx->cfg);

    yagl_resource_cleanup(&ctx->res);

    g_free(ctx);

    client_ctx->destroy(client_ctx);
}

struct yagl_egl_context
    *yagl_egl_context_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_client_context *client_ctx,
                             struct yagl_eglb_context *backend_share_ctx)
{
    struct yagl_eglb_context *backend_ctx;
    struct yagl_egl_context *ctx;

    backend_ctx = dpy->backend_dpy->create_context(dpy->backend_dpy,
                                                   &cfg->native,
                                                   client_ctx,
                                                   backend_share_ctx);

    if (!backend_ctx) {
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_resource_init(&ctx->res, &yagl_egl_context_destroy);

    ctx->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    ctx->cfg = cfg;
    ctx->backend_ctx = backend_ctx;
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
