#ifndef _QEMU_YAGL_GLES2_SHADER_H
#define _QEMU_YAGL_GLES2_SHADER_H

#include "yagl_types.h"
#include "yagl_object.h"

/*
 * Programs and shaders share the same namespace,
 * pretty clumsy!
 */
#define YAGL_NS_SHADER_PROGRAM 4

struct yagl_gles2_driver;

struct yagl_gles2_shader
{
    /*
     * These members must be exactly as in yagl_gles2_program
     * @{
     */
    struct yagl_object base;

    bool is_shader;
    /*
     * @}
     */

    struct yagl_gles2_driver *driver;

    yagl_object_name global_name;

    GLenum type;
};

struct yagl_gles2_shader
    *yagl_gles2_shader_create(struct yagl_gles2_driver *driver,
                              GLenum type);

void yagl_gles2_shader_source(struct yagl_gles2_shader *shader,
                              GLchar **strings,
                              int count);

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
