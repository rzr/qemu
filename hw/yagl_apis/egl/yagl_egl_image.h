#ifndef _QEMU_YAGL_EGL_IMAGE_H
#define _QEMU_YAGL_EGL_IMAGE_H

#include "yagl_types.h"
#include "yagl_resource.h"

struct yagl_egl_display;
struct yagl_eglb_image;

struct yagl_egl_image
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_eglb_image *backend_image;
};

struct yagl_egl_image *yagl_egl_image_create(struct yagl_egl_display *dpy,
                                             yagl_winsys_id buffer);

/*
 * Helper functions that simply acquire/release yagl_egl_image::res
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_image_acquire(struct yagl_egl_image *image);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_image_release(struct yagl_egl_image *image);

/*
 * @}
 */

#endif
