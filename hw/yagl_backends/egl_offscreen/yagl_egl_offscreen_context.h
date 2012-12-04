#ifndef _QEMU_YAGL_EGL_OFFSCREEN_CONTEXT_H
#define _QEMU_YAGL_EGL_OFFSCREEN_CONTEXT_H

#include "yagl_eglb_context.h"
#include "yagl_egl_driver.h"

struct yagl_egl_offscreen_display;
struct yagl_egl_native_config;

struct yagl_egl_offscreen_context
{
    struct yagl_eglb_context base;

    EGLContext native_ctx;
};

struct yagl_egl_offscreen_context
    *yagl_egl_offscreen_context_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       struct yagl_client_context *client_ctx,
                                       struct yagl_egl_offscreen_context *share_context);

#endif
