#ifndef _QEMU_YAGL_EGLB_SURFACE_H
#define _QEMU_YAGL_EGLB_SURFACE_H

#include "yagl_types.h"
#include "yagl_egl_surface_attribs.h"
#include <EGL/egl.h>

struct yagl_eglb_display;

struct yagl_eglb_surface
{
    struct yagl_eglb_display *dpy;

    EGLenum type;

    union
    {
        struct yagl_egl_window_attribs window;
        struct yagl_egl_pixmap_attribs pixmap;
        struct yagl_egl_pbuffer_attribs pbuffer;
    } attribs;

    /*
     * Surface has been invalidated on target, update it
     * from 'id'.
     */
    void (*invalidate)(struct yagl_eglb_surface */*sfc*/,
                       yagl_winsys_id /*id*/);

    /*
     * Replaces 'sfc' with 'with', destroying 'with' afterwards.
     * 'sfc' and 'with' must be of same type, but can have
     * different formats.
     */
    void (*replace)(struct yagl_eglb_surface */*sfc*/,
                    struct yagl_eglb_surface */*with*/);

    /*
     * Can be called for surfaces that were reset.
     */
    bool (*query)(struct yagl_eglb_surface */*sfc*/,
                  EGLint /*attribute*/,
                  EGLint */*value*/);

    bool (*swap_buffers)(struct yagl_eglb_surface */*sfc*/);

    bool (*copy_buffers)(struct yagl_eglb_surface */*sfc*/);

    void (*wait_gl)(struct yagl_eglb_surface */*sfc*/);

    /*
     * Can be called even when this surface is not current!
     * @{
     */
    bool (*bind_tex_image)(struct yagl_eglb_surface */*sfc*/);

    bool (*release_tex_image)(struct yagl_eglb_surface */*sfc*/);
    /*
     * @}
     */

    void (*destroy)(struct yagl_eglb_surface */*sfc*/);
};

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs);
void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc);

#endif
