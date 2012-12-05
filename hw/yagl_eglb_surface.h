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

    void (*invalidate)(struct yagl_eglb_surface */*sfc*/);

    void (*replace)(struct yagl_eglb_surface */*sfc*/,
                    struct yagl_eglb_surface */*with*/);

    bool (*query)(struct yagl_eglb_surface */*sfc*/,
                  EGLint /*attribute*/,
                  EGLint */*value*/);

    bool (*swap_buffers)(struct yagl_eglb_surface */*sfc*/);

    bool (*copy_buffers)(struct yagl_eglb_surface */*sfc*/);

    void (*destroy)(struct yagl_eglb_surface */*sfc*/);
};

void yagl_eglb_surface_init(struct yagl_eglb_surface *sfc,
                            struct yagl_eglb_display *dpy,
                            EGLenum type,
                            const void *attribs);
void yagl_eglb_surface_cleanup(struct yagl_eglb_surface *sfc);

#endif
