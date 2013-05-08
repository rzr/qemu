#ifndef _QEMU_YAGL_EGL_SURFACE_H
#define _QEMU_YAGL_EGL_SURFACE_H

#include "yagl_types.h"
#include "yagl_resource.h"

struct yagl_egl_display;
struct yagl_egl_config;
struct yagl_eglb_surface;

struct yagl_egl_surface
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_config *cfg;

    struct yagl_eglb_surface *backend_sfc;
};

/*
 * Takes ownership of 'backend_sfc'.
 */
struct yagl_egl_surface
    *yagl_egl_surface_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_eglb_surface *backend_sfc);

/*
 * Helper functions that simply acquire/release yagl_egl_surface::res
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_surface_acquire(struct yagl_egl_surface *sfc);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_surface_release(struct yagl_egl_surface *sfc);

/*
 * @}
 */

#endif
