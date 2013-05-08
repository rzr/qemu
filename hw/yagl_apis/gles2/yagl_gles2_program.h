#ifndef _QEMU_YAGL_GLES2_PROGRAM_H
#define _QEMU_YAGL_GLES2_PROGRAM_H

#include "yagl_types.h"
#include "yagl_object.h"

struct yagl_gles2_driver;
struct yagl_gles2_shader;

struct yagl_gles2_program
{
    /*
     * These members must be exactly as in yagl_gles2_shader
     * @{
     */
    struct yagl_object base;

    bool is_shader;
    /*
     * @}
     */

    struct yagl_gles2_driver *driver;

    yagl_object_name global_name;

    yagl_object_name vertex_shader_local_name;

    yagl_object_name fragment_shader_local_name;
};

struct yagl_gles2_program
    *yagl_gles2_program_create(struct yagl_gles2_driver *driver);

bool yagl_gles2_program_attach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name);

bool yagl_gles2_program_detach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name);

void yagl_gles2_program_link(struct yagl_gles2_program *program);

int yagl_gles2_program_get_attrib_location(struct yagl_gles2_program *program,
                                           const GLchar *name);

int yagl_gles2_program_get_uniform_location(struct yagl_gles2_program *program,
                                            const GLchar *name);

void yagl_gles2_program_bind_attrib_location(struct yagl_gles2_program *program,
                                             GLuint index,
                                             const GLchar *name);

void yagl_gles2_program_get_param(struct yagl_gles2_program *program,
                                  GLenum pname,
                                  GLint *param);

void yagl_gles2_program_get_active_attrib(struct yagl_gles2_program *program,
                                          GLuint index,
                                          GLsizei bufsize,
                                          GLsizei *length,
                                          GLint *size,
                                          GLenum *type,
                                          GLchar *name);

void yagl_gles2_program_get_active_uniform(struct yagl_gles2_program *program,
                                           GLuint index,
                                           GLsizei bufsize,
                                           GLsizei *length,
                                           GLint *size,
                                           GLenum *type,
                                           GLchar *name);

void yagl_gles2_program_get_info_log(struct yagl_gles2_program *program,
                                     GLsizei bufsize,
                                     GLsizei *length,
                                     GLchar *infolog);

void yagl_gles2_program_validate(struct yagl_gles2_program *program);

bool yagl_gles2_program_get_uniform_type(struct yagl_gles2_program *program,
                                         GLint location,
                                         GLenum *type);

void yagl_gles2_program_get_uniform_float(struct yagl_gles2_program *program,
                                          GLint location,
                                          GLfloat *params);

void yagl_gles2_program_get_uniform_int(struct yagl_gles2_program *program,
                                        GLint location,
                                        GLint *params);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles2_program_acquire(struct yagl_gles2_program *program);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_gles2_program_release(struct yagl_gles2_program *program);

#endif
