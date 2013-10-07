#ifndef _QEMU_YAGL_EGL_INTERFACE_H
#define _QEMU_YAGL_EGL_INTERFACE_H

#include "yagl_types.h"

struct yagl_egl_interface
{
    void (*ensure_ctx)(struct yagl_egl_interface */*iface*/);

    void (*unensure_ctx)(struct yagl_egl_interface */*iface*/);
};

#endif
