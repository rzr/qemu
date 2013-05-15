#ifndef _QEMU_YAGL_EGLB_DISPLAY_H
#define _QEMU_YAGL_EGLB_DISPLAY_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_backend;
struct yagl_eglb_context;
struct yagl_eglb_surface;
struct yagl_eglb_image;
struct yagl_egl_native_config;
struct yagl_egl_window_attribs;
struct yagl_egl_pixmap_attribs;
struct yagl_egl_pbuffer_attribs;
struct yagl_client_context;

struct yagl_eglb_display
{
    struct yagl_egl_backend *backend;

    struct yagl_egl_native_config *(*config_enum)(struct yagl_eglb_display */*dpy*/,
                                                  int */*num_configs*/);

    void (*config_cleanup)(struct yagl_eglb_display */*dpy*/,
                           const struct yagl_egl_native_config */*cfg*/);

    struct yagl_eglb_context *(*create_context)(struct yagl_eglb_display */*dpy*/,
                                                const struct yagl_egl_native_config */*cfg*/,
                                                struct yagl_client_context */*client_ctx*/,
                                                struct yagl_eglb_context */*share_context*/);

    /*
     * 'pixels' are locked in target's memory, no page fault possible.
     */
    struct yagl_eglb_surface *(*create_offscreen_surface)(struct yagl_eglb_display */*dpy*/,
                                                          const struct yagl_egl_native_config */*cfg*/,
                                                          EGLenum /*type*/,
                                                          const void */*attribs*/,
                                                          uint32_t /*width*/,
                                                          uint32_t /*height*/,
                                                          uint32_t /*bpp*/,
                                                          target_ulong /*pixels*/);

    struct yagl_eglb_surface *(*create_onscreen_window_surface)(struct yagl_eglb_display */*dpy*/,
                                                                const struct yagl_egl_native_config */*cfg*/,
                                                                const struct yagl_egl_window_attribs */*attribs*/,
                                                                yagl_winsys_id /*id*/);

    struct yagl_eglb_surface *(*create_onscreen_pixmap_surface)(struct yagl_eglb_display */*dpy*/,
                                                                const struct yagl_egl_native_config */*cfg*/,
                                                                const struct yagl_egl_pixmap_attribs */*attribs*/,
                                                                yagl_winsys_id /*id*/);

    struct yagl_eglb_surface *(*create_onscreen_pbuffer_surface)(struct yagl_eglb_display */*dpy*/,
                                                                 const struct yagl_egl_native_config */*cfg*/,
                                                                 const struct yagl_egl_pbuffer_attribs */*attribs*/,
                                                                 yagl_winsys_id /*id*/);

    struct yagl_eglb_image *(*create_image)(struct yagl_eglb_display */*dpy*/,
                                            yagl_winsys_id /*buffer*/);

    void (*destroy)(struct yagl_eglb_display */*dpy*/);
};

void yagl_eglb_display_init(struct yagl_eglb_display *dpy,
                            struct yagl_egl_backend *backend);
void yagl_eglb_display_cleanup(struct yagl_eglb_display *dpy);

#endif
