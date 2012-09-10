#ifndef _QEMU_YAGL_GLES_OGL_H
#define _QEMU_YAGL_GLES_OGL_H

#include "yagl_gles_driver.h"

struct yagl_dyn_lib;
struct yagl_gles_ogl;

struct yagl_gles_ogl_ps
{
    struct yagl_gles_driver_ps base;

    struct yagl_gles_ogl *driver;
};

struct yagl_gles_ogl_ps
    *yagl_gles_ogl_ps_create(struct yagl_gles_ogl *gles_ogl,
                             struct yagl_process_state *ps);

void yagl_gles_ogl_ps_destroy(struct yagl_gles_ogl_ps *gles_ogl_ps);

struct yagl_gles_ogl
    *yagl_gles_ogl_create(struct yagl_dyn_lib *dyn_lib);

void yagl_gles_ogl_destroy(struct yagl_gles_ogl *gles_ogl);

#endif
