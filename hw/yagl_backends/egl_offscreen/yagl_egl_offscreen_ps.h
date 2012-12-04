#ifndef _QEMU_YAGL_EGL_OFFSCREEN_PS_H
#define _QEMU_YAGL_EGL_OFFSCREEN_PS_H

#include "yagl_egl_backend.h"

struct yagl_egl_offscreen;
struct yagl_egl_driver_ps;

struct yagl_egl_offscreen_ps
{
    struct yagl_egl_backend_ps base;

    struct yagl_egl_driver_ps *driver_ps;
};

struct yagl_egl_offscreen_ps
    *yagl_egl_offscreen_ps_create(struct yagl_egl_offscreen *egl_offscreen,
                                  struct yagl_process_state *ps);


#endif
