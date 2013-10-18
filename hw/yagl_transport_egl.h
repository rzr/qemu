#ifndef _QEMU_YAGL_TRANSPORT_EGL_H
#define _QEMU_YAGL_TRANSPORT_EGL_H

#include "yagl_types.h"
#include "yagl_transport.h"
#include <EGL/egl.h>

static __inline EGLint yagl_transport_get_out_EGLint(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

static __inline EGLenum yagl_transport_get_out_EGLenum(struct yagl_transport *t)
{
    return yagl_transport_get_out_uint32_t(t);
}

#endif
