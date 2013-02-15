#ifndef _QEMU_YAGL_GLES2_CONTEXT_H
#define _QEMU_YAGL_GLES2_CONTEXT_H

#include <GLES2/gl2.h>
#include "yagl_apis/gles/yagl_gles_context.h"

struct yagl_gles2_driver_ps;
struct yagl_sharegroup;

struct yagl_gles2_context
{
    struct yagl_gles_context base;

    struct yagl_gles2_driver_ps *driver_ps;

    bool prepared;

    /*
     * From 'base.base.sg' for speed.
     */
    struct yagl_sharegroup *sg;

    /*
     * Strip 'precision' keyword from shaders because
     * host implementation doesn't support them.
     */
    bool shader_strip_precision;

    int num_shader_binary_formats;

    bool texture_half_float;

    bool vertex_half_float;

    bool standard_derivatives;

    yagl_object_name program_local_name;
};

struct yagl_gles2_context
    *yagl_gles2_context_create(struct yagl_sharegroup *sg,
                               struct yagl_gles2_driver_ps *driver_ps);

void yagl_gles2_context_use_program(struct yagl_gles2_context *ctx,
                                    yagl_object_name program_local_name);

void yagl_gles2_context_unuse_program(struct yagl_gles2_context *ctx,
                                      yagl_object_name program_local_name);

#endif
