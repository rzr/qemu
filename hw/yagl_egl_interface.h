#ifndef _QEMU_YAGL_EGL_INTERFACE_H
#define _QEMU_YAGL_EGL_INTERFACE_H

#include "yagl_types.h"

struct yagl_client_context;

struct yagl_egl_interface
{
    struct yagl_client_context *(*get_ctx)(struct yagl_egl_interface */*iface*/);
};

void yagl_egl_interface_init(struct yagl_egl_interface *iface);

void yagl_egl_interface_cleanup(struct yagl_egl_interface *iface);

#endif
