#ifndef _QEMU_YAGL_EGL_onscreen_IMAGE_H
#define _QEMU_YAGL_EGL_onscreen_IMAGE_H

#include "yagl_eglb_image.h"

struct yagl_egl_onscreen_display;

struct yagl_egl_onscreen_image
{
    struct yagl_eglb_image base;
};

struct yagl_egl_onscreen_image
    *yagl_egl_onscreen_image_create(struct yagl_egl_onscreen_display *dpy,
                                    yagl_winsys_id buffer);

#endif
