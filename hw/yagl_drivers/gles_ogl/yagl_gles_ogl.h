#ifndef _QEMU_YAGL_GLES_OGL_H
#define _QEMU_YAGL_GLES_OGL_H

#include "yagl_types.h"

struct yagl_gles_driver;
struct yagl_dyn_lib;

bool yagl_gles_ogl_init(struct yagl_gles_driver *driver,
                        struct yagl_dyn_lib *dyn_lib);

void yagl_gles_ogl_cleanup(struct yagl_gles_driver *driver);

#endif
