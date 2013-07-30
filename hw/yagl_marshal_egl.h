#ifndef _QEMU_YAGL_MARSHAL_EGL_H
#define _QEMU_YAGL_MARSHAL_EGL_H

#include "yagl_marshal.h"
#include <EGL/egl.h>

#define yagl_marshal_put_EGLBoolean(buff, value) yagl_marshal_put_uint32((buff), (value))
#define yagl_marshal_get_EGLBoolean(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_put_EGLenum(buff, value) yagl_marshal_put_uint32((buff), (value))
#define yagl_marshal_get_EGLenum(buff) yagl_marshal_get_uint32(buff)
#define yagl_marshal_put_EGLint(buff, value) yagl_marshal_put_int32((buff), (value))
#define yagl_marshal_get_EGLint(buff) yagl_marshal_get_int32(buff)

#define yagl_marshal_get_EGLClientBuffer(buff) (EGLClientBuffer)yagl_marshal_skip(buff)
#define yagl_marshal_get_EGLNativePixmapType(buff) (EGLNativePixmapType)yagl_marshal_skip(buff)
#define yagl_marshal_get_EGLNativeWindowType(buff) (EGLNativeWindowType)yagl_marshal_skip(buff)

#endif
