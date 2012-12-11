#ifndef _QEMU_YAGL_EGL_OFFSCREEN_H
#define _QEMU_YAGL_EGL_OFFSCREEN_H

#include "yagl_egl_backend.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_native_config.h"

struct yagl_egl_offscreen
{
    struct yagl_egl_backend base;

    struct yagl_egl_driver *driver;

    /*
     * Display, config, context and surface which'll be used
     * when we need to ensure that at least some context
     * is current.
     */
    EGLNativeDisplayType ensure_dpy;
    struct yagl_egl_native_config ensure_config;
    EGLContext ensure_ctx;
    EGLSurface ensure_sfc;

    /*
     * Global context, all created contexts share with it. This
     * context is never current to any thread (And never make it
     * current because on windows it's impossible to share with
     * a context that's current).
     */
    EGLContext global_ctx;
};

/*
 * Takes ownership of 'driver'
 */
struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *driver);

#endif
