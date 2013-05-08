#include <GLES2/gl2.h>
#include "yagl_gles2_program.h"
#include "yagl_gles2_shader.h"
#include "yagl_gles2_driver.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_gles2_program_destroy(struct yagl_ref *ref)
{
    struct yagl_gles2_program *program = (struct yagl_gles2_program*)ref;

    yagl_ensure_ctx();
    program->driver->DeleteProgram(program->global_name);
    yagl_unensure_ctx();

    yagl_object_cleanup(&program->base);

    g_free(program);
}

struct yagl_gles2_program
    *yagl_gles2_program_create(struct yagl_gles2_driver *driver)
{
    GLuint global_name;
    struct yagl_gles2_program *program;

    global_name = driver->CreateProgram();

    program = g_malloc0(sizeof(*program));

    yagl_object_init(&program->base, &yagl_gles2_program_destroy);

    program->is_shader = false;
    program->driver = driver;
    program->global_name = global_name;

    return program;
}

bool yagl_gles2_program_attach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name)
{
    switch (shader->type) {
    case GL_VERTEX_SHADER:
        if (program->vertex_shader_local_name) {
            return false;
        }
        program->vertex_shader_local_name = shader_local_name;
        break;
    case GL_FRAGMENT_SHADER:
        if (program->fragment_shader_local_name) {
            return false;
        }
        program->fragment_shader_local_name = shader_local_name;
        break;
    default:
        return false;
    }

    program->driver->AttachShader(program->global_name,
                                  shader->global_name);

    return true;
}

bool yagl_gles2_program_detach_shader(struct yagl_gles2_program *program,
                                      struct yagl_gles2_shader *shader,
                                      yagl_object_name shader_local_name)
{
    if (program->vertex_shader_local_name == shader_local_name) {
        program->vertex_shader_local_name = 0;
    } else if (program->fragment_shader_local_name == shader_local_name) {
        program->fragment_shader_local_name = 0;
    } else {
        return false;
    }

    program->driver->DetachShader(program->global_name,
                                  shader->global_name);

    return true;
}

void yagl_gles2_program_link(struct yagl_gles2_program *program)
{
    program->driver->LinkProgram(program->global_name);
}

int yagl_gles2_program_get_attrib_location(struct yagl_gles2_program *program,
                                           const GLchar *name)
{
    return program->driver->GetAttribLocation(program->global_name,
                                              name);
}

int yagl_gles2_program_get_uniform_location(struct yagl_gles2_program *program,
                                            const GLchar *name)
{
    return program->driver->GetUniformLocation(program->global_name,
                                               name);
}

void yagl_gles2_program_bind_attrib_location(struct yagl_gles2_program *program,
                                             GLuint index,
                                             const GLchar *name)
{
    program->driver->BindAttribLocation(program->global_name,
                                        index,
                                        name);
}

void yagl_gles2_program_get_param(struct yagl_gles2_program *program,
                                  GLenum pname,
                                  GLint *param)
{
    program->driver->GetProgramiv(program->global_name,
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
    program->driver->GetActiveAttrib(program->global_name,
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
    program->driver->GetActiveUniform(program->global_name,
                                      index,
                                      bufsize,
                                      length,
                                      size,
                                      type,
                                      name);
}

void yagl_gles2_program_get_info_log(struct yagl_gles2_program *program,
                                     GLsizei bufsize,
                                     GLsizei *length,
                                     GLchar *infolog)
{
    program->driver->GetProgramInfoLog(program->global_name,
                                       bufsize,
                                       length,
                                       infolog);
}

void yagl_gles2_program_validate(struct yagl_gles2_program *program)
{
    program->driver->ValidateProgram(program->global_name);
}

bool yagl_gles2_program_get_uniform_type(struct yagl_gles2_program *program,
                                         GLint location,
                                         GLenum *type)
{
    GLint link_status = GL_FALSE;
    GLint i = 0, num_active_uniforms = 0;
    GLint uniform_name_max_length = 0;
    GLchar *uniform_name = NULL;
    bool res = false;

    YAGL_LOG_FUNC_SET(yagl_gles2_program_get_uniform_type);

    if (location < 0) {
        return false;
    }

    program->driver->GetProgramiv(program->global_name,
                                  GL_LINK_STATUS,
                                  &link_status);

    if (link_status == GL_FALSE) {
        return false;
    }

    program->driver->GetProgramiv(program->global_name,
                                  GL_ACTIVE_UNIFORMS,
                                  &num_active_uniforms);

    program->driver->GetProgramiv(program->global_name,
                                  GL_ACTIVE_UNIFORM_MAX_LENGTH,
                                  &uniform_name_max_length);

    uniform_name = g_malloc(uniform_name_max_length + 1);

    for (i = 0; i < num_active_uniforms; ++i) {
        GLsizei length = 0;
        GLint size = 0;
        GLenum tmp_type = 0;

        program->driver->GetActiveUniform(program->global_name,
                                          i,
                                          uniform_name_max_length,
                                          &length,
                                          &size,
                                          &tmp_type,
                                          uniform_name);

        if (length == 0) {
            YAGL_LOG_ERROR("Cannot get active uniform %d for program %d", i, program->global_name);
            continue;
        }

        if (program->driver->GetUniformLocation(program->global_name,
                                                uniform_name) == location) {
            *type = tmp_type;
            res = true;
            break;
        }
    }

    g_free(uniform_name);

    return res;
}

void yagl_gles2_program_get_uniform_float(struct yagl_gles2_program *program,
                                          GLint location,
                                          GLfloat *params)
{
    program->driver->GetUniformfv(program->global_name,
                                  location,
                                  params);
}

void yagl_gles2_program_get_uniform_int(struct yagl_gles2_program *program,
                                        GLint location,
                                        GLint *params)
{
    program->driver->GetUniformiv(program->global_name,
                                  location,
                                  params);
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
