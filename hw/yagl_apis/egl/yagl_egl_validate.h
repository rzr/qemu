#ifndef _QEMU_YAGL_EGL_VALIDATE_H
#define _QEMU_YAGL_EGL_VALIDATE_H

#include "yagl_types.h"
#include <EGL/egl.h>

bool yagl_egl_is_attrib_list_empty(const EGLint *attrib_list);

bool yagl_egl_is_api_valid(EGLenum api);

#endif
