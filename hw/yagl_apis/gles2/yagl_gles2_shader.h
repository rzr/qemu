#ifndef _QEMU_YAGL_GLES2_SHADER_H
#define _QEMU_YAGL_GLES2_SHADER_H

#include "yagl_types.h"
#include "yagl_object.h"

#define YAGL_NS_SHADER 4

struct yagl_gles2_driver_ps;

struct yagl_gles2_shader
{
    struct yagl_object base;

    struct yagl_gles2_driver_ps *driver_ps;

    yagl_object_name global_name;

    GLenum type;
};

struct yagl_gles2_shader
    *yagl_gles2_shader_create(struct yagl_gles2_driver_ps *driver_ps,
                              GLenum type);

void yagl_gles2_shader_source(struct yagl_gles2_shader *shader,
                              GLchar **strings,
                              int count,
                              bool strip_precision);

void yagl_gles2_shader_compile(struct yagl_gles2_shader *shader);

void yagl_gles2_shader_get_param(struct yagl_gles2_shader *shader,
                                 GLenum pname,
                                 GLint *param);

void yagl_gles2_shader_get_source(struct yagl_gles2_shader *shader,
                                  GLsizei bufsize,
                                  GLsizei *length,
                                  GLchar *source);

void yagl_gles2_shader_get_info_log(struct yagl_gles2_shader *shader,
                                    GLsizei bufsize,
                                    GLsizei *length,
                                    GLchar *infolog);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles2_shader_acquire(struct yagl_gles2_shader *shader);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles2_shader_release(struct yagl_gles2_shader *shader);

#endif
