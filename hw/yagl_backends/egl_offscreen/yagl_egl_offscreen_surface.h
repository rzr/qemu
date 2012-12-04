#ifndef _QEMU_YAGL_EGL_OFFSCREEN_SURFACE_H
#define _QEMU_YAGL_EGL_OFFSCREEN_SURFACE_H

#include "yagl_eglb_surface.h"
#include "yagl_egl_driver.h"

struct yagl_egl_offscreen_display;
struct yagl_egl_native_config;

struct yagl_egl_offscreen_surface
{
    struct yagl_eglb_surface base;

    EGLSurface native_sfc;
};

struct yagl_egl_offscreen_surface
    *yagl_egl_offscreen_surface_create(struct yagl_egl_offscreen_display *dpy,
                                       const struct yagl_egl_native_config *cfg,
                                       EGLenum type,
                                       const void *attribs,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t bpp,
                                       target_ulong pixels);

#endif
