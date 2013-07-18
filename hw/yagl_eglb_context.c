#include "yagl_eglb_context.h"

void yagl_eglb_context_init(struct yagl_eglb_context *ctx,
                            struct yagl_eglb_display *dpy,
                            struct yagl_client_context *client_ctx)
{
    ctx->dpy = dpy;
    ctx->client_ctx = client_ctx;
}

void yagl_eglb_context_cleanup(struct yagl_eglb_context *ctx)
{
}
