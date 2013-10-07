#ifndef _QEMU_YAGL_EGL_DRIVER_H
#define _QEMU_YAGL_EGL_DRIVER_H

#include "yagl_types.h"
#include "yagl_dyn_lib.h"
#include <EGL/egl.h>

struct yagl_egl_native_config;
struct yagl_egl_pbuffer_attribs;

/*
 * YaGL EGL driver.
 * @{
 */

struct yagl_egl_driver
{
    /*
     * Open/close one and only display.
     * @{
     */

    EGLNativeDisplayType (*display_open)(struct yagl_egl_driver */*driver*/);
    void (*display_close)(struct yagl_egl_driver */*driver*/,
                          EGLNativeDisplayType /*dpy*/);

    /*
     * @}
     */

    /*
     * Returns a list of supported configs, must be freed with 'g_free'
     * after use. Note that you also must call 'config_cleanup' on each
     * returned config before freeing to cleanup the driver data in config.
     *
     * Configs returned must:
     * + Support RGBA
     * + Support at least PBUFFER surface type
     */
    struct yagl_egl_native_config *(*config_enum)(struct yagl_egl_driver */*driver*/,
                                                  EGLNativeDisplayType /*dpy*/,
                                                  int */*num_configs*/);

    void (*config_cleanup)(struct yagl_egl_driver */*driver*/,
                           EGLNativeDisplayType /*dpy*/,
                           const struct yagl_egl_native_config */*cfg*/);

    EGLSurface (*pbuffer_surface_create)(struct yagl_egl_driver */*driver*/,
                                         EGLNativeDisplayType /*dpy*/,
                                         const struct yagl_egl_native_config */*cfg*/,
                                         EGLint /*width*/,
                                         EGLint /*height*/,
                                         const struct yagl_egl_pbuffer_attribs */*attribs*/);

    void (*pbuffer_surface_destroy)(struct yagl_egl_driver */*driver*/,
                                    EGLNativeDisplayType /*dpy*/,
                                    EGLSurface /*sfc*/);

    EGLContext (*context_create)(struct yagl_egl_driver */*driver*/,
                                 EGLNativeDisplayType /*dpy*/,
                                 const struct yagl_egl_native_config */*cfg*/,
                                 EGLContext /*share_context*/);

    void (*context_destroy)(struct yagl_egl_driver */*driver*/,
                            EGLNativeDisplayType /*dpy*/,
                            EGLContext /*ctx*/);

    bool (*make_current)(struct yagl_egl_driver */*driver*/,
                         EGLNativeDisplayType /*dpy*/,
                         EGLSurface /*draw*/,
                         EGLSurface /*read*/,
                         EGLContext /*ctx*/);

    void (*destroy)(struct yagl_egl_driver */*driver*/);

    struct yagl_dyn_lib *dyn_lib;
};

/*
 * 'display' is Display* on linux and HWND on windows.
 */
struct yagl_egl_driver *yagl_egl_driver_create(void *display);

void yagl_egl_driver_init(struct yagl_egl_driver *driver);
void yagl_egl_driver_cleanup(struct yagl_egl_driver *driver);

/*
 * @}
 */

#endif
