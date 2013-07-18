#ifndef _QEMU_YAGL_HOST_GLES2_CALLS_H
#define _QEMU_YAGL_HOST_GLES2_CALLS_H

#include "yagl_api.h"
#include <GLES2/gl2.h>
#include "yagl_apis/gles/yagl_host_gles_calls.h"

struct yagl_api_ps *yagl_host_gles2_process_init(struct yagl_api *api);

bool yagl_host_glAttachShader(GLuint program,
    GLuint shader);
bool yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    target_ulong /* const GLchar* */ name_);
bool yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
bool yagl_host_glCompileShader(GLuint shader);
bool yagl_host_glCreateProgram(GLuint* retval);
bool yagl_host_glCreateShader(GLuint* retval,
    GLenum type);
bool yagl_host_glDeleteProgram(GLuint program);
bool yagl_host_glDeleteShader(GLuint shader);
bool yagl_host_glDetachShader(GLuint program,
    GLuint shader);
bool yagl_host_glDisableVertexAttribArray(GLuint index);
bool yagl_host_glEnableVertexAttribArray(GLuint index);
bool yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_);
bool yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_);
bool yagl_host_glGetAttachedShaders(GLuint program,
    GLsizei maxcount,
    target_ulong /* GLsizei* */ count_,
    target_ulong /* GLuint* */ shaders_);
bool yagl_host_glGetAttribLocation(int* retval,
    GLuint program,
    target_ulong /* const GLchar* */ name_);
bool yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetProgramInfoLog(GLuint program,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLchar* */ infolog_);
bool yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetShaderInfoLog(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLchar* */ infolog_);
bool yagl_host_glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype,
    target_ulong /* GLint* */ range_,
    target_ulong /* GLint* */ precision_);
bool yagl_host_glGetShaderSource(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLchar* */ source_);
bool yagl_host_glGetUniformfv(GLuint program,
    GLint location,
    target_ulong /* GLfloat* */ params_);
bool yagl_host_glGetUniformiv(GLuint program,
    GLint location,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetUniformLocation(int* retval,
    GLuint program,
    target_ulong /* const GLchar* */ name_);
bool yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    target_ulong /* GLfloat* */ params_);
bool yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetVertexAttribPointerv(GLuint index,
    GLenum pname,
    target_ulong /* GLvoid** */ pointer_);
bool yagl_host_glIsProgram(GLboolean* retval,
    GLuint program);
bool yagl_host_glIsShader(GLboolean* retval,
    GLuint shader);
bool yagl_host_glLinkProgram(GLuint program);
bool yagl_host_glReleaseShaderCompiler(void);
bool yagl_host_glShaderBinary(GLsizei n,
    target_ulong /* const GLuint* */ shaders,
    GLenum binaryformat,
    target_ulong /* const GLvoid* */ binary,
    GLsizei length);
bool yagl_host_glShaderSource(GLuint shader,
    GLsizei count,
    target_ulong /* const GLchar** */ string_,
    target_ulong /* const GLint* */ length_);
bool yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask);
bool yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask);
bool yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass);
bool yagl_host_glUniform1f(GLint location,
    GLfloat x);
bool yagl_host_glUniform1fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
bool yagl_host_glUniform1i(GLint location,
    GLint x);
bool yagl_host_glUniform1iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
bool yagl_host_glUniform2f(GLint location,
    GLfloat x,
    GLfloat y);
bool yagl_host_glUniform2fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
bool yagl_host_glUniform2i(GLint location,
    GLint x,
    GLint y);
bool yagl_host_glUniform2iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
bool yagl_host_glUniform3f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z);
bool yagl_host_glUniform3fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
bool yagl_host_glUniform3i(GLint location,
    GLint x,
    GLint y,
    GLint z);
bool yagl_host_glUniform3iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
bool yagl_host_glUniform4f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
bool yagl_host_glUniform4fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
bool yagl_host_glUniform4i(GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w);
bool yagl_host_glUniform4iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
bool yagl_host_glUniformMatrix2fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
bool yagl_host_glUniformMatrix3fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
bool yagl_host_glUniformMatrix4fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
bool yagl_host_glUseProgram(GLuint program);
bool yagl_host_glValidateProgram(GLuint program);
bool yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x);
bool yagl_host_glVertexAttrib1fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
bool yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y);
bool yagl_host_glVertexAttrib2fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
bool yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z);
bool yagl_host_glVertexAttrib3fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
bool yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
bool yagl_host_glVertexAttrib4fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
bool yagl_host_glVertexAttribPointer(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    target_ulong /* const GLvoid* */ ptr);

#endif
