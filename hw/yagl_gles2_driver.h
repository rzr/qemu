#ifndef _QEMU_YAGL_GLES2_DRIVER_H
#define _QEMU_YAGL_GLES2_DRIVER_H

#include "yagl_types.h"
#include "yagl_gles_driver.h"

/*
 * YaGL GLES2 driver.
 * @{
 */

struct yagl_gles2_driver
{
    struct yagl_gles_driver base;

    YAGL_GLES_DRIVER_FUNC2(AttachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC3(BindAttribLocation, GLuint, GLuint, const GLchar*, program, index, name)
    YAGL_GLES_DRIVER_FUNC4(BlendColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC1(CompileShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC_RET0(GLuint, CreateProgram)
    YAGL_GLES_DRIVER_FUNC_RET1(GLuint, CreateShader, GLenum, type)
    YAGL_GLES_DRIVER_FUNC1(DeleteProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(DeleteShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC2(DetachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_DRIVER_FUNC1(DisableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC1(EnableVertexAttribArray, GLuint, index)
    YAGL_GLES_DRIVER_FUNC7(GetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC7(GetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_DRIVER_FUNC4(GetAttachedShaders, GLuint, GLsizei, GLsizei*, GLuint*, program, maxcount, count, shaders)
    YAGL_GLES_DRIVER_FUNC_RET2(int, GetAttribLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(GetProgramiv, GLuint, GLenum, GLint*, program, pname, params)
    YAGL_GLES_DRIVER_FUNC4(GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, program, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC3(GetShaderiv, GLuint, GLenum, GLint*, shader, pname, params)
    YAGL_GLES_DRIVER_FUNC4(GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, infolog)
    YAGL_GLES_DRIVER_FUNC4(GetShaderPrecisionFormat, GLenum, GLenum, GLint*, GLint*, shadertype, precisiontype, range, precision)
    YAGL_GLES_DRIVER_FUNC4(GetShaderSource, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, source)
    YAGL_GLES_DRIVER_FUNC3(GetUniformfv, GLuint, GLint, GLfloat*, program, location, params)
    YAGL_GLES_DRIVER_FUNC3(GetUniformiv, GLuint, GLint, GLint*, program, location, params)
    YAGL_GLES_DRIVER_FUNC_RET2(int, GetUniformLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_DRIVER_FUNC3(GetVertexAttribfv, GLuint, GLenum, GLfloat*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetVertexAttribiv, GLuint, GLenum, GLint*, index, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetVertexAttribPointerv, GLuint, GLenum, GLvoid**, index, pname, pointer)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC_RET1(GLboolean, IsShader, GLuint, shader)
    YAGL_GLES_DRIVER_FUNC1(LinkProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC4(ShaderSource, GLuint, GLsizei, const GLchar**, const GLint*, shader, count, string, length)
    YAGL_GLES_DRIVER_FUNC4(StencilFuncSeparate, GLenum, GLenum, GLint, GLuint, face, func, ref, mask)
    YAGL_GLES_DRIVER_FUNC2(StencilMaskSeparate, GLenum, GLuint, face, mask)
    YAGL_GLES_DRIVER_FUNC4(StencilOpSeparate, GLenum, GLenum, GLenum, GLenum, face, fail, zfail, zpass)
    YAGL_GLES_DRIVER_FUNC2(Uniform1f, GLint, GLfloat, location, x)
    YAGL_GLES_DRIVER_FUNC3(Uniform1fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC2(Uniform1i, GLint, GLint, location, x)
    YAGL_GLES_DRIVER_FUNC3(Uniform1iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(Uniform2f, GLint, GLfloat, GLfloat, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(Uniform2fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC3(Uniform2i, GLint, GLint, GLint, location, x, y)
    YAGL_GLES_DRIVER_FUNC3(Uniform2iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(Uniform3f, GLint, GLfloat, GLfloat, GLfloat, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Uniform3fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(Uniform3i, GLint, GLint, GLint, GLint, location, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Uniform3iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(Uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(Uniform4fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_DRIVER_FUNC5(Uniform4i, GLint, GLint, GLint, GLint, GLint, location, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC3(Uniform4iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix2fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC4(UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_DRIVER_FUNC1(UseProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC1(ValidateProgram, GLuint, program)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib1f, GLuint, GLfloat, indx, x)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib1fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC3(VertexAttrib2f, GLuint, GLfloat, GLfloat, indx, x, y)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib2fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC4(VertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat, indx, x, y, z)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib3fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC5(VertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, indx, x, y, z, w)
    YAGL_GLES_DRIVER_FUNC2(VertexAttrib4fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_DRIVER_FUNC6(VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, indx, size, type, normalized, stride, ptr)

    void (*destroy)(struct yagl_gles2_driver */*driver*/);
};

/*
 * Does NOT take ownership of 'common', you must destroy it yourself when
 * you're done!
 */
void yagl_gles2_driver_init(struct yagl_gles2_driver *driver);
void yagl_gles2_driver_cleanup(struct yagl_gles2_driver *driver);

/*
 * @}
 */

#endif
