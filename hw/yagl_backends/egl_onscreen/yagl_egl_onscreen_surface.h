#ifndef _QEMU_YAGL_EGL_ONSCREEN_SURFACE_H
#define _QEMU_YAGL_EGL_ONSCREEN_SURFACE_H

#include "yagl_eglb_surface.h"
#include "yagl_egl_surface_attribs.h"

struct yagl_egl_onscreen_display;
struct yagl_egl_native_config;

struct winsys_resource;
struct winsys_gl_surface;

struct yagl_egl_onscreen_surface
{
    struct yagl_eglb_surface base;

    /*
     * 1x1 native pbuffer surface, used just to be able to make
     * this surface current.
     */
    EGLSurface dummy_native_sfc;

    /*
     * winsys resource in case if this is a window or pixmap surface.
     */
    struct winsys_resource *ws_res;

    /*
     * winsys surface in case if this is a window or pixmap surface.
     */
    struct winsys_gl_surface *ws_sfc;

    /*
     * Callback cookie from ws_res::add_callback.
     */
    void *cookie;

    /*
     * pbuffer texture in case if this is a pbuffer surface. Allocated
     * when this surface is made current for the first time.
     */
    GLuint pbuffer_tex;
    uint32_t pbuffer_width;
    uint32_t pbuffer_height;

    /*
     * Depth and stencil renderbuffer for 'ws_sfc'/'pbuffer_tex'. Allocated
     * when this surface is made current for the first time.
     */
    GLuint rb;

    /*
     * This surface has been changed on target and
     * needs update on host.
     */
    bool needs_update;
};

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_window(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_window_attribs *attribs,
                                             yagl_winsys_id id);

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pixmap(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pixmap_attribs *attribs,
                                             yagl_winsys_id id);

struct yagl_egl_onscreen_surface
    *yagl_egl_onscreen_surface_create_pbuffer(struct yagl_egl_onscreen_display *dpy,
                                             const struct yagl_egl_native_config *cfg,
                                             const struct yagl_egl_pbuffer_attribs *attribs,
                                             uint32_t width,
                                             uint32_t height);

void yagl_egl_onscreen_surface_setup(struct yagl_egl_onscreen_surface *sfc);

void yagl_egl_onscreen_surface_attach_to_framebuffer(struct yagl_egl_onscreen_surface *sfc);

uint32_t yagl_egl_onscreen_surface_width(struct yagl_egl_onscreen_surface *sfc);

uint32_t yagl_egl_onscreen_surface_height(struct yagl_egl_onscreen_surface *sfc);

#endif
