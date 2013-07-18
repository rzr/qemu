#ifndef _QEMU_YAGL_EGL_OFFSCREEN_TS_H
#define _QEMU_YAGL_EGL_OFFSCREEN_TS_H

#include "yagl_types.h"

struct yagl_egl_offscreen_display;
struct yagl_egl_offscreen_context;

struct yagl_egl_offscreen_ts
{
    struct yagl_egl_offscreen_display *dpy;
    struct yagl_egl_offscreen_context *ctx;
};

struct yagl_egl_offscreen_ts *yagl_egl_offscreen_ts_create(void);

void yagl_egl_offscreen_ts_destroy(struct yagl_egl_offscreen_ts *egl_offscreen_ts);

#endif
