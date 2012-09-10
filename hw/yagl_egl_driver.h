#ifndef _QEMU_YAGL_EGL_DRIVER_H
#define _QEMU_YAGL_EGL_DRIVER_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_thread_state;
struct yagl_process_state;
struct yagl_egl_native_config;
struct yagl_egl_pbuffer_attribs;

/*
 * YaGL EGL driver per-process state.
 * @{
 */

struct yagl_egl_driver_ps
{
    struct yagl_egl_driver *driver;

    struct yagl_process_state *ps;

    void (*thread_init)(struct yagl_egl_driver_ps */*driver_ps*/,
                        struct yagl_thread_state */*ts*/);

    EGLNativeDisplayType (*display_create)(struct yagl_egl_driver_ps */*driver_ps*/);
    void (*display_destroy)(struct yagl_egl_driver_ps */*driver_ps*/,
                            EGLNativeDisplayType /*dpy*/);

    /*
     * Returns a list of supported configs, must be freed with 'g_free'
     * after use. Note that you also must call 'config_cleanup' on each
     * returned config before freeing to cleanup the driver data in config.
     *
     * Configs returned must:
     * + Support RGBA
     * + Support at least PBUFFER surface type
     */
    struct yagl_egl_native_config *(*config_enum)(struct yagl_egl_driver_ps */*driver_ps*/,
                                                  EGLNativeDisplayType /*dpy*/,
                                                  int */*num_configs*/);

    void (*config_cleanup)(struct yagl_egl_driver_ps */*driver_ps*/,
                           EGLNativeDisplayType /*dpy*/,
                           struct yagl_egl_native_config */*cfg*/);

    EGLSurface (*pbuffer_surface_create)(struct yagl_egl_driver_ps */*driver_ps*/,
                                         EGLNativeDisplayType /*dpy*/,
                                         const struct yagl_egl_native_config */*cfg*/,
                                         EGLint /*width*/,
                                         EGLint /*height*/,
                                         const struct yagl_egl_pbuffer_attribs */*attribs*/);

    void (*pbuffer_surface_destroy)(struct yagl_egl_driver_ps */*driver_ps*/,
                                    EGLNativeDisplayType /*dpy*/,
                                    EGLSurface /*sfc*/);

    EGLContext (*context_create)(struct yagl_egl_driver_ps */*driver_ps*/,
                                 EGLNativeDisplayType /*dpy*/,
                                 const struct yagl_egl_native_config */*cfg*/,
                                 yagl_client_api /*client_api*/,
                                 EGLContext /*share_context*/);

    void (*context_destroy)(struct yagl_egl_driver_ps */*driver_ps*/,
                            EGLNativeDisplayType /*dpy*/,
                            EGLContext /*ctx*/);

    bool (*make_current)(struct yagl_egl_driver_ps */*driver_ps*/,
                         EGLNativeDisplayType /*dpy*/,
                         EGLSurface /*draw*/,
                         EGLSurface /*read*/,
                         EGLContext /*ctx*/);

    void (*wait_native)(struct yagl_egl_driver_ps */*driver_ps*/);

    void (*thread_fini)(struct yagl_egl_driver_ps */*driver_ps*/);

    void (*destroy)(struct yagl_egl_driver_ps */*driver_ps*/);
};

void yagl_egl_driver_ps_init(struct yagl_egl_driver_ps *driver_ps,
                             struct yagl_egl_driver *driver,
                             struct yagl_process_state *ps);
void yagl_egl_driver_ps_cleanup(struct yagl_egl_driver_ps *driver_ps);

/*
 * @}
 */

/*
 * YaGL EGL driver.
 * @{
 */

struct yagl_egl_driver
{
    struct yagl_egl_driver_ps *(*process_init)(struct yagl_egl_driver */*driver*/,
                                               struct yagl_process_state */*ps*/);

    void (*destroy)(struct yagl_egl_driver */*driver*/);
};

void yagl_egl_driver_init(struct yagl_egl_driver *driver);
void yagl_egl_driver_cleanup(struct yagl_egl_driver *driver);

/*
 * @}
 */

#endif
