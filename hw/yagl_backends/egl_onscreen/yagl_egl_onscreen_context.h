#ifndef _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H
#define _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H

#include "yagl_eglb_context.h"
#include <EGL/egl.h>

struct yagl_egl_onscreen_display;
struct yagl_egl_onscreen_surface;
struct yagl_egl_native_config;

struct yagl_egl_onscreen_context
{
    struct yagl_eglb_context base;

    EGLContext native_ctx;

    /*
     * Framebuffer that is used to render to window/pixmap/pbuffer texture.
     * Allocated when this context is made current for the first time.
     *
     * Onscreen GLES API redirects framebuffer zero to this framebuffer.
     */
    GLuint fb;
};

struct yagl_egl_onscreen_context
    *yagl_egl_onscreen_context_create(struct yagl_egl_onscreen_display *dpy,
                                      const struct yagl_egl_native_config *cfg,
                                      struct yagl_client_context *client_ctx,
                                      struct yagl_egl_onscreen_context *share_context);

void yagl_egl_onscreen_context_setup(struct yagl_egl_onscreen_context *ctx);

#endif
