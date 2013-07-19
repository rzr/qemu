#ifndef _QEMU_YAGL_EGL_INTERFACE_H
#define _QEMU_YAGL_EGL_INTERFACE_H

#include "yagl_types.h"

struct yagl_client_context;
struct yagl_client_image;

struct yagl_egl_interface
{
    yagl_render_type render_type;

    struct yagl_client_context *(*get_ctx)(struct yagl_egl_interface */*iface*/);

    struct yagl_client_image *(*get_image)(struct yagl_egl_interface */*iface*/,
                                           yagl_host_handle /*image*/);

    void (*ensure_ctx)(struct yagl_egl_interface */*iface*/);

    void (*unensure_ctx)(struct yagl_egl_interface */*iface*/);
};

void yagl_egl_interface_init(struct yagl_egl_interface *iface,
                             yagl_render_type render_type);

void yagl_egl_interface_cleanup(struct yagl_egl_interface *iface);

#endif
