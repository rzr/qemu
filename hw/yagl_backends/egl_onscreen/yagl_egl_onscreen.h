#ifndef _QEMU_YAGL_EGL_ONSCREEN_H
#define _QEMU_YAGL_EGL_ONSCREEN_H

#include "yagl_egl_backend.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_native_config.h"

struct winsys_interface;
struct yagl_gles_driver;

struct yagl_egl_onscreen
{
    struct yagl_egl_backend base;

    struct winsys_interface *wsi;

    struct yagl_egl_driver *egl_driver;

    struct yagl_gles_driver *gles_driver;

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
 * Takes ownership of 'egl_driver'.
 */
struct yagl_egl_backend *yagl_egl_onscreen_create(struct winsys_interface *wsi,
                                                  struct yagl_egl_driver *egl_driver,
                                                  struct yagl_gles_driver *gles_driver);

#endif
