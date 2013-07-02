#ifndef _QEMU_YAGL_GLES1_OGL_H
#define _QEMU_YAGL_GLES1_OGL_H

#include "yagl_types.h"
#include "yagl_dyn_lib.h"

struct yagl_gles1_driver;

struct yagl_gles1_driver *yagl_gles1_ogl_create(struct yagl_dyn_lib *dyn_lib);

#endif
