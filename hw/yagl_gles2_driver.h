#ifndef _QEMU_YAGL_GLES2_DRIVER_H
#define _QEMU_YAGL_GLES2_DRIVER_H

#include "yagl_types.h"
#include "yagl_gles_driver.h"

/*
 * YaGL GLES2 driver per-process state.
 * @{
 */

struct yagl_gles2_driver_ps
{
    struct yagl_gles2_driver *driver;

    struct yagl_gles_driver_ps *common;

    void (*thread_init)(struct yagl_gles2_driver_ps */*driver_ps*/,
                        struct yagl_thread_state */*ts*/);

    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, AttachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, BindAttribLocation, GLuint, GLuint, const GLchar*, program, index, name)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, BlendColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, BlendEquation, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, BlendEquationSeparate, GLenum, GLenum, modeRGB, modeAlpha)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, BlendFuncSeparate, GLenum, GLenum, GLenum, GLenum, srcRGB, dstRGB, srcAlpha, dstAlpha)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, CompileShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC_RET0(struct yagl_gles2_driver_ps *driver_ps, GLuint, CreateProgram)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles2_driver_ps *driver_ps, GLuint, CreateShader, GLenum, type)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, DeleteProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, DeleteShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, DetachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, DisableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, EnableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC7(struct yagl_gles2_driver_ps *driver_ps, GetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC7(struct yagl_gles2_driver_ps *driver_ps, GetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, GetAttachedShaders, GLuint, GLsizei, GLsizei*, GLuint*, program, maxcount, count, shaders)
    YAGL_GLES_DRIVER_FUNC_RET2(struct yagl_gles2_driver_ps *driver_ps, int, GetAttribLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetProgramiv, GLuint, GLenum, GLint*, program, pname, params)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, program, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetShaderiv, GLuint, GLenum, GLint*, shader, pname, params)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, GetShaderPrecisionFormat, GLenum, GLenum, GLint*, GLint*, shadertype, precisiontype, range, precision)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, GetShaderSource, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, source)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetUniformfv, GLuint, GLint, GLfloat*, program, location, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetUniformiv, GLuint, GLint, GLint*, program, location, params)
    YAGL_GLES_DRIVER_FUNC_RET2(struct yagl_gles2_driver_ps *driver_ps, int, GetUniformLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetVertexAttribfv, GLuint, GLenum, GLfloat*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetVertexAttribiv, GLuint, GLenum, GLint*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, GetVertexAttribPointerv, GLuint, GLenum, GLvoid**, index, pname, pointer)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles2_driver_ps *driver_ps, GLboolean, IsProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC_RET1(struct yagl_gles2_driver_ps *driver_ps, GLboolean, IsShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, LinkProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles2_driver_ps *driver_ps, ReleaseShaderCompiler)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles2_driver_ps *driver_ps, ShaderBinary, GLsizei, const GLuint*, GLenum, const GLvoid*, GLsizei, n, shaders, binaryformat, binary, length)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, ShaderSource, GLuint, GLsizei, const GLchar**, const GLint*, shader, count, string, length)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, StencilFuncSeparate, GLenum, GLenum, GLint, GLuint, face, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, StencilMaskSeparate, GLenum, GLuint, face, mask)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, StencilOpSeparate, GLenum, GLenum, GLenum, GLenum, face, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, Uniform1f, GLint, GLfloat, location, x)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform1fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, Uniform1i, GLint, GLint, location, x)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform1iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform2f, GLint, GLfloat, GLfloat, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform2fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform2i, GLint, GLint, GLint, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform2iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, Uniform3f, GLint, GLfloat, GLfloat, GLfloat, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform3fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, Uniform3i, GLint, GLint, GLint, GLint, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform3iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles2_driver_ps *driver_ps, Uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform4fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles2_driver_ps *driver_ps, Uniform4i, GLint, GLint, GLint, GLint, GLint, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, Uniform4iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, UniformMatrix2fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, UniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, UseProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles2_driver_ps *driver_ps, ValidateProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib1f, GLuint, GLfloat, indx, x)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib1fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib2f, GLuint, GLfloat, GLfloat, indx, x, y)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib2fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat, indx, x, y, z)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib3fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, indx, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles2_driver_ps *driver_ps, VertexAttrib4fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC6(struct yagl_gles2_driver_ps *driver_ps, VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, indx, size, type, normalized, stride, ptr)

    void (*thread_fini)(struct yagl_gles2_driver_ps */*driver_ps*/);

    void (*destroy)(struct yagl_gles2_driver_ps */*driver_ps*/);
};

/*
 * Does NOT take ownership of 'common', you must destroy it yourself when
 * you're done!
 */
void yagl_gles2_driver_ps_init(struct yagl_gles2_driver_ps *driver_ps,
                               struct yagl_gles2_driver *driver,
                               struct yagl_gles_driver_ps *common);
void yagl_gles2_driver_ps_cleanup(struct yagl_gles2_driver_ps *driver_ps);

/*
 * @}
 */

/*
 * YaGL GLES2 driver.
 * @{
 */

struct yagl_gles2_driver
{
    struct yagl_gles2_driver_ps *(*process_init)(struct yagl_gles2_driver */*driver*/,
                                                 struct yagl_process_state */*ps*/);

    void (*destroy)(struct yagl_gles2_driver */*driver*/);
};

void yagl_gles2_driver_init(struct yagl_gles2_driver *driver);
void yagl_gles2_driver_cleanup(struct yagl_gles2_driver *driver);

/*
 * @}
 */

#endif
