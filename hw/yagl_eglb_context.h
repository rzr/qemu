#ifndef _QEMU_YAGL_EGLB_CONTEXT_H
#define _QEMU_YAGL_EGLB_CONTEXT_H

#include "yagl_types.h"

struct yagl_eglb_display;
struct yagl_client_context;

struct yagl_eglb_context
{
    struct yagl_eglb_display *dpy;

    struct yagl_client_context *client_ctx;

    void (*destroy)(struct yagl_eglb_context */*ctx*/);
};

void yagl_eglb_context_init(struct yagl_eglb_context *ctx,
                            struct yagl_eglb_display *dpy,
                            struct yagl_client_context *client_ctx);
void yagl_eglb_context_cleanup(struct yagl_eglb_context *ctx);

#endif
