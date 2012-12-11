#ifndef _QEMU_YAGL_EGL_OFFSCREEN_DISPLAY_H
#define _QEMU_YAGL_EGL_OFFSCREEN_DISPLAY_H

#include "yagl_eglb_display.h"
#include "yagl_egl_driver.h"

struct yagl_egl_offscreen;

struct yagl_egl_offscreen_display
{
    struct yagl_eglb_display base;

    EGLNativeDisplayType native_dpy;
};

struct yagl_egl_offscreen_display
    *yagl_egl_offscreen_display_create(struct yagl_egl_offscreen *egl_offscreen);

#endif
