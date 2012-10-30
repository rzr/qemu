#ifndef _QEMU_YAGL_EGL_SURFACE_H
#define _QEMU_YAGL_EGL_SURFACE_H

#include "yagl_types.h"
#include "yagl_resource.h"
#include "yagl_egl_surface_attribs.h"
#include "qemu-thread.h"
#include <EGL/egl.h>

struct yagl_egl_display;
struct yagl_egl_config;
struct yagl_compiled_transfer;

struct yagl_egl_surface
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_config *cfg;

    EGLenum type;

    union
    {
        struct yagl_egl_window_attribs window;
        struct yagl_egl_pixmap_attribs pixmap;
        struct yagl_egl_pbuffer_attribs pbuffer;
    } attribs;

    /*
     * Backing image compiled transfers are the only ones that
     * can be accessed from multiple threads
     * simultaneously. When we call
     * eglDestroySurface we want to destroy backing image
     * immediately, because target's pixmap will no longer be available
     * after host 'eglDestroySurface' call is complete. The surface however
     * may be accessed from another thread where it's current
     * (from eglSwapBuffers for example), in that case the result of the call
     * must be EGL_BAD_SURFACE.
     */
    QemuMutex bimage_mtx;

    /*
     * Compiled transfer for backing image. It'll become NULL
     * after eglDestroySurface is called.
     */
    struct yagl_compiled_transfer *bimage_ct;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;

    /*
     * Host pre-allocated storage for backing image.
     * This will go to compiled transfer above.
     */
    void *host_pixels;

    /*
     * Native pbuffer surface.
     */
    EGLSurface native_sfc;

    /*
     * Attribs with which this 'native_sfc' was created with.
     */
    struct yagl_egl_pbuffer_attribs native_sfc_attribs;
};

/*
 * Takes ownership of 'bimage_ct'. byte-copies 'attribs'.
 */
struct yagl_egl_surface
    *yagl_egl_surface_create_window(struct yagl_egl_display *dpy,
                                    struct yagl_egl_config *cfg,
                                    const struct yagl_egl_window_attribs *attribs,
                                    struct yagl_compiled_transfer *bimage_ct,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t bpp);

/*
 * Takes ownership of 'bimage_ct'. byte-copies 'attribs'.
 */
struct yagl_egl_surface
    *yagl_egl_surface_create_pixmap(struct yagl_egl_display *dpy,
                                    struct yagl_egl_config *cfg,
                                    const struct yagl_egl_pixmap_attribs *attribs,
                                    struct yagl_compiled_transfer *bimage_ct,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t bpp);

/*
 * Takes ownership of 'bimage_ct'. byte-copies 'attribs'.
 */
struct yagl_egl_surface
    *yagl_egl_surface_create_pbuffer(struct yagl_egl_display *dpy,
                                     struct yagl_egl_config *cfg,
                                     const struct yagl_egl_pbuffer_attribs *attribs,
                                     struct yagl_compiled_transfer *bimage_ct,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t bpp);

/*
 * Lock/unlock surface for 'bimage' access, all 'bimage' operations
 * must be carried out while surface is locked.
 * @{
 */

void yagl_egl_surface_lock(struct yagl_egl_surface *sfc);

void yagl_egl_surface_unlock(struct yagl_egl_surface *sfc);

/*
 * @}
 */

/*
 * Takes ownership of 'native_sfc' and 'bimage_ct'.
 *
 * This will destroy old surface data and update it with new one.
 * Must be called while surface is locked.
 */
void yagl_egl_surface_update(struct yagl_egl_surface *sfc,
                             EGLSurface native_sfc,
                             struct yagl_compiled_transfer *bimage_ct,
                             uint32_t width,
                             uint32_t height,
                             uint32_t bpp);

/*
 * This will destroy old surface data.
 * Must be called while surface is locked.
 */
void yagl_egl_surface_invalidate(struct yagl_egl_surface *sfc);

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
