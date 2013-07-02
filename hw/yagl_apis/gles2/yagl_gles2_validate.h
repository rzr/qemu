#ifndef _QEMU_YAGL_GLES2_VALIDATE_H
#define _QEMU_YAGL_GLES2_VALIDATE_H

#include "yagl_types.h"

bool yagl_gles2_get_array_param_count(GLenum pname, int *count);

bool yagl_gles2_get_uniform_type_count(GLenum uniform_type, int *count);

#endif
