#ifndef _QEMU_YAGL_HOST_GLES2_CALLS_H
#define _QEMU_YAGL_HOST_GLES2_CALLS_H

#include "yagl_api.h"
#include <GLES2/gl2.h>
#include "yagl_apis/gles/yagl_host_gles_calls.h"

struct yagl_api_ps
    *yagl_host_gles2_process_init(struct yagl_api *api,
                                  struct yagl_process_state *ps);

void yagl_host_glAttachShader(GLuint program,
    GLuint shader);
void yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    target_ulong /* const GLchar* */ name_);
void yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer);
void yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer);
void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
void yagl_host_glBlendEquation(GLenum mode);
void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha);
void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha);
GLenum yagl_host_glCheckFramebufferStatus(GLenum target);
void yagl_host_glCompileShader(GLuint shader);
GLuint yagl_host_glCreateProgram(void);
GLuint yagl_host_glCreateShader(GLenum type);
void yagl_host_glDeleteFramebuffers(GLsizei n,
    target_ulong /* const GLuint* */ framebuffers_);
void yagl_host_glDeleteProgram(GLuint program);
void yagl_host_glDeleteRenderbuffers(GLsizei n,
    target_ulong /* const GLuint* */ renderbuffers_);
void yagl_host_glDeleteShader(GLuint shader);
void yagl_host_glDetachShader(GLuint program,
    GLuint shader);
void yagl_host_glDisableVertexAttribArray(GLuint index);
void yagl_host_glEnableVertexAttribArray(GLuint index);
void yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer);
void yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level);
void yagl_host_glGenerateMipmap(GLenum target);
void yagl_host_glGenFramebuffers(GLsizei n,
    target_ulong /* GLuint* */ framebuffers_);
void yagl_host_glGenRenderbuffers(GLsizei n,
    target_ulong /* GLuint* */ renderbuffers_);
void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_);
void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length_,
    target_ulong /* GLint* */ size_,
    target_ulong /* GLenum* */ type_,
    target_ulong /* GLchar* */ name_);
void yagl_host_glGetAttachedShaders(GLuint program,
    GLsizei maxcount,
    target_ulong /* GLsizei* */ count,
    target_ulong /* GLuint* */ shaders);
int yagl_host_glGetAttribLocation(GLuint program,
    target_ulong /* const GLchar* */ name_);
void yagl_host_glGetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment,
    GLenum pname,
    target_ulong /* GLint* */ params);
void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    target_ulong /* GLint* */ params_);
void yagl_host_glGetProgramInfoLog(GLuint program,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ infolog);
void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params);
void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    target_ulong /* GLint* */ params_);
void yagl_host_glGetShaderInfoLog(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ infolog);
void yagl_host_glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype,
    target_ulong /* GLint* */ range,
    target_ulong /* GLint* */ precision);
void yagl_host_glGetShaderSource(GLuint shader,
    GLsizei bufsize,
    target_ulong /* GLsizei* */ length,
    target_ulong /* GLchar* */ source);
void yagl_host_glGetUniformfv(GLuint program,
    GLint location,
    target_ulong /* GLfloat* */ params);
void yagl_host_glGetUniformiv(GLuint program,
    GLint location,
    target_ulong /* GLint* */ params);
int yagl_host_glGetUniformLocation(GLuint program,
    target_ulong /* const GLchar* */ name_);
void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    target_ulong /* GLfloat* */ params);
void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    target_ulong /* GLint* */ params);
void yagl_host_glGetVertexAttribPointerv(GLuint index,
    GLenum pname,
    target_ulong /* GLvoid** */ pointer);
GLboolean yagl_host_glIsFramebuffer(GLuint framebuffer);
GLboolean yagl_host_glIsProgram(GLuint program);
GLboolean yagl_host_glIsRenderbuffer(GLuint renderbuffer);
GLboolean yagl_host_glIsShader(GLuint shader);
void yagl_host_glLinkProgram(GLuint program);
void yagl_host_glReleaseShaderCompiler(void);
void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height);
void yagl_host_glShaderBinary(GLsizei n,
    target_ulong /* const GLuint* */ shaders,
    GLenum binaryformat,
    target_ulong /* const GLvoid* */ binary,
    GLsizei length);
void yagl_host_glShaderSource(GLuint shader,
    GLsizei count,
    target_ulong /* const GLchar** */ string_,
    target_ulong /* const GLint* */ length_);
void yagl_host_glStencilFuncSeparate(GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask);
void yagl_host_glStencilMaskSeparate(GLenum face,
    GLuint mask);
void yagl_host_glStencilOpSeparate(GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass);
void yagl_host_glUniform1f(GLint location,
    GLfloat x);
void yagl_host_glUniform1fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
void yagl_host_glUniform1i(GLint location,
    GLint x);
void yagl_host_glUniform1iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
void yagl_host_glUniform2f(GLint location,
    GLfloat x,
    GLfloat y);
void yagl_host_glUniform2fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
void yagl_host_glUniform2i(GLint location,
    GLint x,
    GLint y);
void yagl_host_glUniform2iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
void yagl_host_glUniform3f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glUniform3fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
void yagl_host_glUniform3i(GLint location,
    GLint x,
    GLint y,
    GLint z);
void yagl_host_glUniform3iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
void yagl_host_glUniform4f(GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glUniform4fv(GLint location,
    GLsizei count,
    target_ulong /* const GLfloat* */ v_);
void yagl_host_glUniform4i(GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w);
void yagl_host_glUniform4iv(GLint location,
    GLsizei count,
    target_ulong /* const GLint* */ v_);
void yagl_host_glUniformMatrix2fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
void yagl_host_glUniformMatrix3fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
void yagl_host_glUniformMatrix4fv(GLint location,
    GLsizei count,
    GLboolean transpose,
    target_ulong /* const GLfloat* */ value_);
void yagl_host_glUseProgram(GLuint program);
void yagl_host_glValidateProgram(GLuint program);
void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x);
void yagl_host_glVertexAttrib1fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y);
void yagl_host_glVertexAttrib2fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glVertexAttrib3fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glVertexAttrib4fv(GLuint indx,
    target_ulong /* const GLfloat* */ values_);
void yagl_host_glVertexAttribPointer(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    target_ulong /* const GLvoid* */ ptr);

#endif
