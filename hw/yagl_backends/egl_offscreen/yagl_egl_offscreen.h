#ifndef _QEMU_YAGL_EGL_OFFSCREEN_H
#define _QEMU_YAGL_EGL_OFFSCREEN_H

#include "yagl_egl_backend.h"

struct yagl_egl_driver;

struct yagl_egl_offscreen
{
    struct yagl_egl_backend base;

    struct yagl_egl_driver *driver;
};

/*
 * Takes ownership of 'driver'
 */
struct yagl_egl_backend *yagl_egl_offscreen_create(struct yagl_egl_driver *driver);

#endif
