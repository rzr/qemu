#include <GLES2/gl2.h>
#include "yagl_gles2_program.h"
#include "yagl_gles2_shader.h"
#include "yagl_gles2_driver.h"

static void yagl_gles2_program_destroy(struct yagl_ref *ref)
{
    struct yagl_gles2_program *program = (struct yagl_gles2_program*)ref;

    if (!program->base.nodelete) {
        program->driver_ps->DeleteProgram(program->driver_ps, program->global_name);
    }

    qemu_mutex_destroy(&program->mutex);

    yagl_object_cleanup(&program->base);

    g_free(program);
}

struct yagl_gles2_program
    *yagl_gles2_program_create(struct yagl_gles2_driver_ps *driver_ps)
{
    GLuint global_name;
    struct yagl_gles2_program *program;

    global_name = driver_ps->CreateProgram(driver_ps);

    program = g_malloc0(sizeof(*program));

    yagl_object_init(&program->base, &yagl_gles2_program_destroy);

    program->driver_ps = driver_ps;
    program->global_name = global_name;

    qemu_mutex_init(&program->mutex);

    return program;
}

bool yagl_gles2_program_attach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name)
{
    qemu_mutex_lock(&program->mutex);

    switch (shader->type) {
    case GL_VERTEX_SHADER:
        if (program->vertex_shader_local_name) {
            qemu_mutex_unlock(&program->mutex);
            return false;
        }
        program->vertex_shader_local_name = shader_local_name;
        break;
    case GL_FRAGMENT_SHADER:
        if (program->fragment_shader_local_name) {
            qemu_mutex_unlock(&program->mutex);
            return false;
        }
        program->fragment_shader_local_name = shader_local_name;
        break;
    default:
        qemu_mutex_unlock(&program->mutex);
        return false;
    }

    program->driver_ps->AttachShader(program->driver_ps,
                                     program->global_name,
                                     shader->global_name);

    qemu_mutex_unlock(&program->mutex);

    return true;
}

bool yagl_gles2_program_detach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name)
{
    qemu_mutex_lock(&program->mutex);

    if (program->vertex_shader_local_name == shader_local_name) {
        program->vertex_shader_local_name = 0;
    } else if (program->fragment_shader_local_name == shader_local_name) {
        program->fragment_shader_local_name = 0;
    } else {
        qemu_mutex_unlock(&program->mutex);
        return false;
    }

    program->driver_ps->DetachShader(program->driver_ps,
                                     program->global_name,
                                     shader->global_name);

    qemu_mutex_unlock(&program->mutex);

    return true;
}

void yagl_gles2_program_link(struct yagl_gles2_program *program)
{
    program->driver_ps->LinkProgram(program->driver_ps,
                                    program->global_name);
}

int yagl_gles2_program_get_attrib_location(struct yagl_gles2_program *program,
                                           const GLchar *name)
{
    return program->driver_ps->GetAttribLocation(program->driver_ps,
                                                 program->global_name,
                                                 name);
}

int yagl_gles2_program_get_uniform_location(struct yagl_gles2_program *program,
                                            const GLchar *name)
{
    return program->driver_ps->GetUniformLocation(program->driver_ps,
                                                  program->global_name,
                                                  name);
}

void yagl_gles2_program_bind_attrib_location(struct yagl_gles2_program *program,
                                             GLuint index,
                                             const GLchar *name)
{
    program->driver_ps->BindAttribLocation(program->driver_ps,
                                           program->global_name,
                                           index,
                                           name);
}

void yagl_gles2_program_get_param(struct yagl_gles2_program *program,
                                  GLenum pname,
                                  GLint *param)
{
    program->driver_ps->GetProgramiv(program->driver_ps,
                                     program->global_name,
                                     pname,
                                     param);
}

void yagl_gles2_program_get_active_attrib(struct yagl_gles2_program *program,
                                          GLuint index,
                                          GLsizei bufsize,
                                          GLsizei *length,
                                          GLint *size,
                                          GLenum *type,
                                          GLchar *name)
{
    program->driver_ps->GetActiveAttrib(program->driver_ps,
                                        program->global_name,
                                        index,
                                        bufsize,
                                        length,
                                        size,
                                        type,
                                        name);
}

void yagl_gles2_program_get_active_uniform(struct yagl_gles2_program *program,
                                           GLuint index,
                                           GLsizei bufsize,
                                           GLsizei *length,
                                           GLint *size,
                                           GLenum *type,
                                           GLchar *name)
{
    program->driver_ps->GetActiveUniform(program->driver_ps,
                                         program->global_name,
                                         index,
                                         bufsize,
                                         length,
                                         size,
                                         type,
                                         name);
}

void yagl_gles2_program_acquire(struct yagl_gles2_program *program)
{
    if (program) {
        yagl_object_acquire(&program->base);
    }
}

void yagl_gles2_program_release(struct yagl_gles2_program *program)
{
    if (program) {
        yagl_object_release(&program->base);
    }
}
