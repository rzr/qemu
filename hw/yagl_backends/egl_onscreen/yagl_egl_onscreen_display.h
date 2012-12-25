#ifndef _QEMU_YAGL_EGL_ONSCREEN_DISPLAY_H
#define _QEMU_YAGL_EGL_ONSCREEN_DISPLAY_H

#include "yagl_eglb_display.h"

struct yagl_egl_onscreen;

struct yagl_egl_onscreen_display
{
    struct yagl_eglb_display base;

    EGLNativeDisplayType native_dpy;
};

struct yagl_egl_onscreen_display
    *yagl_egl_onscreen_display_create(struct yagl_egl_onscreen *egl_onscreen);

#endif
