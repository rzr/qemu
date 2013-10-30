#ifndef _QEMU_YAGL_EGL_INTERFACE_H
#define _QEMU_YAGL_EGL_INTERFACE_H

#include "yagl_types.h"

struct yagl_egl_interface
{
    uint32_t (*get_ctx_id)(struct yagl_egl_interface */*iface*/);

    void (*ensure_ctx)(struct yagl_egl_interface */*iface*/, uint32_t /*ctx_id*/);

    void (*unensure_ctx)(struct yagl_egl_interface */*iface*/, uint32_t /*ctx_id*/);
};

#endif
