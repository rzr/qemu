#ifndef _QEMU_YAGL_EGL_SURFACE_H
#define _QEMU_YAGL_EGL_SURFACE_H

#include "yagl_types.h"
#include "yagl_resource.h"
#include "qemu-thread.h"

struct yagl_egl_display;
struct yagl_egl_config;
struct yagl_eglb_surface;

struct yagl_egl_surface
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_config *cfg;

    /*
     * Backend surfaces are the only ones that
     * can be accessed from multiple threads
     * simultaneously. When we call
     * eglDestroySurface we want to destroy backend surface
     * immediately. The surface however
     * may be accessed from another thread where it's current
     * (from eglSwapBuffers for example), in that case the result of the call
     * must be EGL_BAD_SURFACE.
     */
    QemuMutex backend_sfc_mtx;

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
 * Lock/unlock surface for 'backend_sfc' access, all 'backend_sfc' operations
 * must be carried out while surface is locked.
 * @{
 */

void yagl_egl_surface_lock(struct yagl_egl_surface *sfc);

void yagl_egl_surface_unlock(struct yagl_egl_surface *sfc);

/*
 * @}
 */

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
