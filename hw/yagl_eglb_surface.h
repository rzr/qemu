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

    bool invalid;

    union
    {
        struct yagl_egl_window_attribs window;
        struct yagl_egl_pixmap_attribs pixmap;
        struct yagl_egl_pbuffer_attribs pbuffer;
    } attribs;

    /*
     * The following function are guaranteed to
     * be serialized.
     * @{
     */

    /*
     * Indicates that user has called eglDestroySurface on this
     * surface. Surface cannot be destroyed immediately, since
     * it can be current to some thread, but its resources
     * can be released here. Implementation must set 'invalid'
     * to true if the surface should be considered invalid. If
     * a surface is invalid then calls to eglSwapBuffers,
     * eglCopyBuffers, etc will fail automatically.
     */
    void (*invalidate)(struct yagl_eglb_surface */*sfc*/);

    /*
     * Replaces 'sfc' with 'with', destroying 'with' afterwards.
     * 'sfc' and 'with' must be of same type, but can have
     * different formats.
     *
     * Only called for valid surfaces.
     */
    void (*replace)(struct yagl_eglb_surface */*sfc*/,
                    struct yagl_eglb_surface */*with*/);

    /*
     * Can be called for invalid surfaces.
     */
    bool (*query)(struct yagl_eglb_surface */*sfc*/,
                  EGLint /*attribute*/,
                  EGLint */*value*/);

    /*
     * Only called for valid surfaces.
     */
    bool (*swap_buffers)(struct yagl_eglb_surface */*sfc*/);

    /*
     * Only called for valid surfaces.
     */
    bool (*copy_buffers)(struct yagl_eglb_surface */*sfc*/,
                         yagl_winsys_id /*target*/);

    void (*destroy)(struct yagl_eglb_surface */*sfc*/);

    /*
     * @}
     */
};

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs);
void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc);

#endif
