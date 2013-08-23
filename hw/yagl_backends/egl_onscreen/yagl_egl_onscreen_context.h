#ifndef _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H
#define _QEMU_YAGL_EGL_ONSCREEN_CONTEXT_H

#include "yagl_eglb_context.h"
#include "yagl_egl_native_config.h"
#include <EGL/egl.h>

struct yagl_egl_onscreen_display;
struct yagl_egl_onscreen_surface;

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

    /*
     * Config with which this context was created, used
     * for 'null_sfc' creation.
     *
     * TODO: It's not very nice to copy 'cfg' into this. It works
     * only because all configs are display resources and context
     * is also a display resource and contexts always outlive configs.
     * But it could change someday...
     */
    struct yagl_egl_native_config cfg;

    /*
     * 1x1 native pbuffer surface, used to implement
     * EGL_KHR_surfaceless_context. Allocated on first
     * surfaceless eglMakeCurrent.
     */
    EGLSurface null_sfc;
};

struct yagl_egl_onscreen_context
    *yagl_egl_onscreen_context_create(struct yagl_egl_onscreen_display *dpy,
                                      const struct yagl_egl_native_config *cfg,
                                      struct yagl_client_context *client_ctx,
                                      struct yagl_egl_onscreen_context *share_context);

void yagl_egl_onscreen_context_setup(struct yagl_egl_onscreen_context *ctx);

bool yagl_egl_onscreen_context_setup_surfaceless(struct yagl_egl_onscreen_context *ctx);

#endif
