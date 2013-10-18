#include "yagl_eglb_context.h"

void yagl_eglb_context_init(struct yagl_eglb_context *ctx,
                            struct yagl_eglb_display *dpy)
{
    ctx->dpy = dpy;
}

void yagl_eglb_context_cleanup(struct yagl_eglb_context *ctx)
{
}
