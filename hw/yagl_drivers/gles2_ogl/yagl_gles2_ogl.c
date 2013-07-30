#include <GL/gl.h>
#include "yagl_gles2_ogl.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl_macros.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl.h"
#include "yagl_gles2_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_tls.h"
#include <GL/glx.h>

static YAGL_DEFINE_TLS(struct yagl_thread_state*, gles2_ogl_ts);

struct yagl_gles2_ogl_ps
{
    struct yagl_gles2_driver_ps base;
};

struct yagl_gles2_ogl
{
    struct yagl_gles2_driver base;

    struct yagl_dyn_lib *dyn_lib;

    struct yagl_gles_ogl *gles_ogl;

    YAGL_GLES_OGL_PROC2(glAttachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_OGL_PROC3(glBindAttribLocation, GLuint, GLuint, const GLchar*, program, index, name)
    YAGL_GLES_OGL_PROC4(glBlendColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
    YAGL_GLES_OGL_PROC1(glBlendEquation, GLenum, mode)
    YAGL_GLES_OGL_PROC2(glBlendEquationSeparate, GLenum, GLenum, modeRGB, modeAlpha)
    YAGL_GLES_OGL_PROC4(glBlendFuncSeparate, GLenum, GLenum, GLenum, GLenum, srcRGB, dstRGB, srcAlpha, dstAlpha)
    YAGL_GLES_OGL_PROC1(glCompileShader, GLuint, shader)
    YAGL_GLES_OGL_PROC_RET0(GLuint, glCreateProgram)
    YAGL_GLES_OGL_PROC_RET1(GLuint, glCreateShader, GLenum, type)
    YAGL_GLES_OGL_PROC1(glDeleteProgram, GLuint, program)
    YAGL_GLES_OGL_PROC1(glDeleteShader, GLuint, shader)
    YAGL_GLES_OGL_PROC2(glDetachShader, GLuint, GLuint, program, shader)
    YAGL_GLES_OGL_PROC1(glDisableVertexAttribArray, GLuint, index)
    YAGL_GLES_OGL_PROC1(glEnableVertexAttribArray, GLuint, index)
    YAGL_GLES_OGL_PROC7(glGetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_OGL_PROC7(glGetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
    YAGL_GLES_OGL_PROC4(glGetAttachedShaders, GLuint, GLsizei, GLsizei*, GLuint*, program, maxcount, count, shaders)
    YAGL_GLES_OGL_PROC_RET2(int, glGetAttribLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_OGL_PROC3(glGetProgramiv, GLuint, GLenum, GLint*, program, pname, params)
    YAGL_GLES_OGL_PROC4(glGetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, program, bufsize, length, infolog)
    YAGL_GLES_OGL_PROC3(glGetShaderiv, GLuint, GLenum, GLint*, shader, pname, params)
    YAGL_GLES_OGL_PROC4(glGetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, infolog)
    YAGL_GLES_OGL_PROC4(glGetShaderPrecisionFormat, GLenum, GLenum, GLint*, GLint*, shadertype, precisiontype, range, precision)
    YAGL_GLES_OGL_PROC4(glGetShaderSource, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, source)
    YAGL_GLES_OGL_PROC3(glGetUniformfv, GLuint, GLint, GLfloat*, program, location, params)
    YAGL_GLES_OGL_PROC3(glGetUniformiv, GLuint, GLint, GLint*, program, location, params)
    YAGL_GLES_OGL_PROC_RET2(int, glGetUniformLocation, GLuint, const GLchar*, program, name)
    YAGL_GLES_OGL_PROC3(glGetVertexAttribfv, GLuint, GLenum, GLfloat*, index, pname, params)
    YAGL_GLES_OGL_PROC3(glGetVertexAttribiv, GLuint, GLenum, GLint*, index, pname, params)
    YAGL_GLES_OGL_PROC3(glGetVertexAttribPointerv, GLuint, GLenum, GLvoid**, index, pname, pointer)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glIsProgram, GLuint, program)
    YAGL_GLES_OGL_PROC_RET1(GLboolean, glIsShader, GLuint, shader)
    YAGL_GLES_OGL_PROC1(glLinkProgram, GLuint, program)
    YAGL_GLES_OGL_PROC0(glReleaseShaderCompiler)
    YAGL_GLES_OGL_PROC5(glShaderBinary, GLsizei, const GLuint*, GLenum, const GLvoid*, GLsizei, n, shaders, binaryformat, binary, length)
    YAGL_GLES_OGL_PROC4(glShaderSource, GLuint, GLsizei, const GLchar**, const GLint*, shader, count, string, length)
    YAGL_GLES_OGL_PROC4(glStencilFuncSeparate, GLenum, GLenum, GLint, GLuint, face, func, ref, mask)
    YAGL_GLES_OGL_PROC2(glStencilMaskSeparate, GLenum, GLuint, face, mask)
    YAGL_GLES_OGL_PROC4(glStencilOpSeparate, GLenum, GLenum, GLenum, GLenum, face, fail, zfail, zpass)
    YAGL_GLES_OGL_PROC2(glUniform1f, GLint, GLfloat, location, x)
    YAGL_GLES_OGL_PROC3(glUniform1fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_OGL_PROC2(glUniform1i, GLint, GLint, location, x)
    YAGL_GLES_OGL_PROC3(glUniform1iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_OGL_PROC3(glUniform2f, GLint, GLfloat, GLfloat, location, x, y)
    YAGL_GLES_OGL_PROC3(glUniform2fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_OGL_PROC3(glUniform2i, GLint, GLint, GLint, location, x, y)
    YAGL_GLES_OGL_PROC3(glUniform2iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_OGL_PROC4(glUniform3f, GLint, GLfloat, GLfloat, GLfloat, location, x, y, z)
    YAGL_GLES_OGL_PROC3(glUniform3fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_OGL_PROC4(glUniform3i, GLint, GLint, GLint, GLint, location, x, y, z)
    YAGL_GLES_OGL_PROC3(glUniform3iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_OGL_PROC5(glUniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat, location, x, y, z, w)
    YAGL_GLES_OGL_PROC3(glUniform4fv, GLint, GLsizei, const GLfloat*, location, count, v)
    YAGL_GLES_OGL_PROC5(glUniform4i, GLint, GLint, GLint, GLint, GLint, location, x, y, z, w)
    YAGL_GLES_OGL_PROC3(glUniform4iv, GLint, GLsizei, const GLint*, location, count, v)
    YAGL_GLES_OGL_PROC4(glUniformMatrix2fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_OGL_PROC4(glUniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_OGL_PROC4(glUniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
    YAGL_GLES_OGL_PROC1(glUseProgram, GLuint, program)
    YAGL_GLES_OGL_PROC1(glValidateProgram, GLuint, program)
    YAGL_GLES_OGL_PROC2(glVertexAttrib1f, GLuint, GLfloat, indx, x)
    YAGL_GLES_OGL_PROC2(glVertexAttrib1fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_OGL_PROC3(glVertexAttrib2f, GLuint, GLfloat, GLfloat, indx, x, y)
    YAGL_GLES_OGL_PROC2(glVertexAttrib2fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_OGL_PROC4(glVertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat, indx, x, y, z)
    YAGL_GLES_OGL_PROC2(glVertexAttrib3fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_OGL_PROC5(glVertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, indx, x, y, z, w)
    YAGL_GLES_OGL_PROC2(glVertexAttrib4fv, GLuint, const GLfloat*, indx, values)
    YAGL_GLES_OGL_PROC6(glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, indx, size, type, normalized, stride, ptr)
};

YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, AttachShader, GLuint, GLuint, program, shader)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, BindAttribLocation, GLuint, GLuint, const GLchar*, program, index, name)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, BlendColor, GLclampf, GLclampf, GLclampf, GLclampf, red, green, blue, alpha)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, BlendEquation, GLenum, mode)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, BlendEquationSeparate, GLenum, GLenum, modeRGB, modeAlpha)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, BlendFuncSeparate, GLenum, GLenum, GLenum, GLenum, srcRGB, dstRGB, srcAlpha, dstAlpha)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, CompileShader, GLuint, shader)
YAGL_GLES_OGL_PROC_IMPL_RET0(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GLuint, CreateProgram)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GLuint, CreateShader, GLenum, type)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, DeleteProgram, GLuint, program)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, DeleteShader, GLuint, shader)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, DetachShader, GLuint, GLuint, program, shader)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, DisableVertexAttribArray, GLuint, index)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, EnableVertexAttribArray, GLuint, index)
YAGL_GLES_OGL_PROC_IMPL7(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
YAGL_GLES_OGL_PROC_IMPL7(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*, program, index, bufsize, length, size, type, name)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetAttachedShaders, GLuint, GLsizei, GLsizei*, GLuint*, program, maxcount, count, shaders)
YAGL_GLES_OGL_PROC_IMPL_RET2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, int, GetAttribLocation, GLuint, const GLchar*, program, name)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetProgramiv, GLuint, GLenum, GLint*, program, pname, params)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, program, bufsize, length, infolog)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetShaderiv, GLuint, GLenum, GLint*, shader, pname, params)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, infolog)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetShaderPrecisionFormat, GLenum, GLenum, GLint*, GLint*, shadertype, precisiontype, range, precision)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetShaderSource, GLuint, GLsizei, GLsizei*, GLchar*, shader, bufsize, length, source)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetUniformfv, GLuint, GLint, GLfloat*, program, location, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetUniformiv, GLuint, GLint, GLint*, program, location, params)
YAGL_GLES_OGL_PROC_IMPL_RET2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, int, GetUniformLocation, GLuint, const GLchar*, program, name)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetVertexAttribfv, GLuint, GLenum, GLfloat*, index, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetVertexAttribiv, GLuint, GLenum, GLint*, index, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GetVertexAttribPointerv, GLuint, GLenum, GLvoid**, index, pname, pointer)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GLboolean, IsProgram, GLuint, program)
YAGL_GLES_OGL_PROC_IMPL_RET1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, GLboolean, IsShader, GLuint, shader)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, LinkProgram, GLuint, program)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, ReleaseShaderCompiler)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, ShaderBinary, GLsizei, const GLuint*, GLenum, const GLvoid*, GLsizei, n, shaders, binaryformat, binary, length)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, ShaderSource, GLuint, GLsizei, const GLchar**, const GLint*, shader, count, string, length)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, StencilFuncSeparate, GLenum, GLenum, GLint, GLuint, face, func, ref, mask)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, StencilMaskSeparate, GLenum, GLuint, face, mask)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, StencilOpSeparate, GLenum, GLenum, GLenum, GLenum, face, fail, zfail, zpass)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform1f, GLint, GLfloat, location, x)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform1fv, GLint, GLsizei, const GLfloat*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform1i, GLint, GLint, location, x)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform1iv, GLint, GLsizei, const GLint*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform2f, GLint, GLfloat, GLfloat, location, x, y)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform2fv, GLint, GLsizei, const GLfloat*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform2i, GLint, GLint, GLint, location, x, y)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform2iv, GLint, GLsizei, const GLint*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform3f, GLint, GLfloat, GLfloat, GLfloat, location, x, y, z)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform3fv, GLint, GLsizei, const GLfloat*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform3i, GLint, GLint, GLint, GLint, location, x, y, z)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform3iv, GLint, GLsizei, const GLint*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat, location, x, y, z, w)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform4fv, GLint, GLsizei, const GLfloat*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform4i, GLint, GLint, GLint, GLint, GLint, location, x, y, z, w)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, Uniform4iv, GLint, GLsizei, const GLint*, location, count, v)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, UniformMatrix2fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, UniformMatrix3fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*, location, count, transpose, value)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, UseProgram, GLuint, program)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, ValidateProgram, GLuint, program)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib1f, GLuint, GLfloat, indx, x)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib1fv, GLuint, const GLfloat*, indx, values)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib2f, GLuint, GLfloat, GLfloat, indx, x, y)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib2fv, GLuint, const GLfloat*, indx, values)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat, indx, x, y, z)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib3fv, GLuint, const GLfloat*, indx, values)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, indx, x, y, z, w)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttrib4fv, GLuint, const GLfloat*, indx, values)
YAGL_GLES_OGL_PROC_IMPL6(yagl_gles2_driver_ps, yagl_gles2_driver_ps, yagl_gles2_ogl, VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, indx, size, type, normalized, stride, ptr)

static void yagl_gles2_ogl_thread_init(struct yagl_gles2_driver_ps *driver_ps,
                                       struct yagl_thread_state *ts)
{
    gles2_ogl_ts = ts;

    YAGL_LOG_FUNC_ENTER_TS(gles2_ogl_ts, yagl_gles2_ogl_thread_init, NULL);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles2_ogl_thread_fini(struct yagl_gles2_driver_ps *driver_ps)
{
    YAGL_LOG_FUNC_ENTER_TS(gles2_ogl_ts, yagl_gles2_ogl_thread_fini, NULL);

    gles2_ogl_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles2_ogl_process_destroy(struct yagl_gles2_driver_ps *driver_ps)
{
    struct yagl_gles2_ogl_ps *gles2_ogl_ps = (struct yagl_gles2_ogl_ps*)driver_ps;
    struct yagl_gles_ogl_ps *gles_ogl_ps = NULL;

    YAGL_LOG_FUNC_ENTER(driver_ps->common->ps->id, 0, yagl_gles2_ogl_process_destroy, NULL);

    gles_ogl_ps = (struct yagl_gles_ogl_ps*)gles2_ogl_ps->base.common;

    yagl_gles2_driver_ps_cleanup(&gles2_ogl_ps->base);

    yagl_gles_ogl_ps_destroy(gles_ogl_ps);

    g_free(gles2_ogl_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_gles2_driver_ps
    *yagl_gles2_ogl_process_init(struct yagl_gles2_driver *driver,
                                 struct yagl_process_state *ps)
{
    struct yagl_gles2_ogl *gles2_ogl = (struct yagl_gles2_ogl*)driver;
    struct yagl_gles2_ogl_ps *gles2_ogl_ps =
        g_malloc0(sizeof(struct yagl_gles2_ogl_ps));
    struct yagl_gles_ogl_ps *gles_ogl_ps = NULL;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_gles2_ogl_process_init, NULL);

    gles_ogl_ps = yagl_gles_ogl_ps_create(gles2_ogl->gles_ogl, ps);
    assert(gles_ogl_ps);

    yagl_gles2_driver_ps_init(&gles2_ogl_ps->base, driver, &gles_ogl_ps->base);

    gles2_ogl_ps->base.thread_init = &yagl_gles2_ogl_thread_init;
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, AttachShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, BindAttribLocation);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, BlendColor);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, BlendEquation);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, BlendEquationSeparate);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, BlendFuncSeparate);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, CompileShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, CreateProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, CreateShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, DeleteProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, DeleteShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, DetachShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, DisableVertexAttribArray);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, EnableVertexAttribArray);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetActiveAttrib);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetActiveUniform);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetAttachedShaders);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetAttribLocation);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetProgramiv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetProgramInfoLog);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetShaderiv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetShaderInfoLog);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetShaderPrecisionFormat);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetShaderSource);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetUniformfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetUniformiv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetUniformLocation);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetVertexAttribfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetVertexAttribiv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, GetVertexAttribPointerv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, IsProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, IsShader);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, LinkProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, ReleaseShaderCompiler);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, ShaderBinary);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, ShaderSource);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, StencilFuncSeparate);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, StencilMaskSeparate);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, StencilOpSeparate);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform1f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform1fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform1i);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform1iv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform2f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform2fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform2i);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform2iv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform3f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform3fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform3i);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform3iv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform4f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform4fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform4i);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, Uniform4iv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, UniformMatrix2fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, UniformMatrix3fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, UniformMatrix4fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, UseProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, ValidateProgram);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib1f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib1fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib2f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib2fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib3f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib3fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib4f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttrib4fv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles2_ogl, gles2_ogl_ps, VertexAttribPointer);
    gles2_ogl_ps->base.thread_fini = &yagl_gles2_ogl_thread_fini;
    gles2_ogl_ps->base.destroy = &yagl_gles2_ogl_process_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles2_ogl_ps->base;
}

static void yagl_gles2_ogl_destroy(struct yagl_gles2_driver *driver)
{
    struct yagl_gles2_ogl *gles2_ogl = (struct yagl_gles2_ogl*)driver;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles2_ogl_destroy, NULL);

    yagl_gles_ogl_destroy(gles2_ogl->gles_ogl);

    yagl_dyn_lib_destroy(gles2_ogl->dyn_lib);

    yagl_gles2_driver_cleanup(&gles2_ogl->base);

    g_free(gles2_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles2_driver *yagl_gles2_ogl_create(void)
{
    struct yagl_gles2_ogl *gles2_ogl = NULL;
    PFNGLXGETPROCADDRESSPROC get_address = NULL;
    struct yagl_dyn_lib *dyn_lib = NULL;
    struct yagl_gles_ogl *gles_ogl = NULL;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles2_ogl_create, NULL);

    gles2_ogl = g_malloc0(sizeof(*gles2_ogl));

    yagl_gles2_driver_init(&gles2_ogl->base);

    dyn_lib = yagl_dyn_lib_create();

    if (!dyn_lib) {
        goto fail;
    }

    if (!yagl_dyn_lib_load(dyn_lib, "libGL.so.1")) {
        YAGL_LOG_ERROR("Unable to load libGL.so.1: %s",
                       yagl_dyn_lib_get_error(dyn_lib));
        goto fail;
    }

    gles_ogl = yagl_gles_ogl_create(dyn_lib);

    if (!gles_ogl) {
        goto fail;
    }

    get_address = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddress");

    if (!get_address) {
        get_address = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddressARB");
    }

    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glAttachShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glBindAttribLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glBlendColor);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glBlendEquation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glBlendEquationSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glBlendFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glCompileShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glCreateProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glCreateShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glDeleteProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glDeleteShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glDetachShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glDisableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glEnableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetActiveAttrib);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetActiveUniform);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetAttachedShaders);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetAttribLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetProgramiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetProgramInfoLog);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetShaderiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetShaderInfoLog);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetShaderPrecisionFormat);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetShaderSource);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetUniformfv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetUniformiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetUniformLocation);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetVertexAttribfv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetVertexAttribiv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glGetVertexAttribPointerv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glIsProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glIsShader);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glLinkProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glReleaseShaderCompiler);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glShaderBinary);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glShaderSource);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glStencilFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glStencilMaskSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glStencilOpSeparate);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform1f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform1fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform1i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform1iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform2f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform2i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform2iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform3f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform3i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform3iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform4f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform4i);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniform4iv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniformMatrix2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniformMatrix3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUniformMatrix4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glUseProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glValidateProgram);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib1f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib1fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib2f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib2fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib3f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib3fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib4f);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttrib4fv);
    YAGL_GLES_OGL_GET_PROC(gles2_ogl, glVertexAttribPointer);

    gles2_ogl->dyn_lib = dyn_lib;
    gles2_ogl->gles_ogl = gles_ogl;
    gles2_ogl->base.process_init = &yagl_gles2_ogl_process_init;
    gles2_ogl->base.destroy = &yagl_gles2_ogl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles2_ogl->base;

fail:
    if (gles_ogl) {
        yagl_gles_ogl_destroy(gles_ogl);
    }
    if (dyn_lib) {
        yagl_dyn_lib_destroy(dyn_lib);
    }
    yagl_gles2_driver_cleanup(&gles2_ogl->base);
    g_free(gles2_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
