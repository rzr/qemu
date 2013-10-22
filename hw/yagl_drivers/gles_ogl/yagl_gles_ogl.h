#ifndef _QEMU_YAGL_GLES_OGL_H
#define _QEMU_YAGL_GLES_OGL_H

#include "yagl_types.h"

struct yagl_gles_driver;
struct yagl_dyn_lib;

struct yagl_gles_driver *yagl_gles_ogl_create(struct yagl_dyn_lib *dyn_lib,
                                              yagl_gl_version gl_version);

#endif
