#ifndef _QEMU_YAGL_EGL_ONSCREEN_TS_H
#define _QEMU_YAGL_EGL_ONSCREEN_TS_H

#include "yagl_types.h"

struct yagl_gles_driver;
struct yagl_egl_onscreen_display;
struct yagl_egl_onscreen_context;
struct yagl_egl_onscreen_surface;

struct yagl_egl_onscreen_ts
{
    struct yagl_gles_driver *gles_driver;

    struct yagl_egl_onscreen_display *dpy;
    struct yagl_egl_onscreen_context *ctx;
    struct yagl_egl_onscreen_surface *sfc_draw;
    struct yagl_egl_onscreen_surface *sfc_read;
};

struct yagl_egl_onscreen_ts *yagl_egl_onscreen_ts_create(struct yagl_gles_driver *gles_driver);

void yagl_egl_onscreen_ts_destroy(struct yagl_egl_onscreen_ts *egl_onscreen_ts);

#endif
