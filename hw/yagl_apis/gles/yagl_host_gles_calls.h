#ifndef _QEMU_YAGL_HOST_GLES_CALLS_H
#define _QEMU_YAGL_HOST_GLES_CALLS_H

#include "yagl_api.h"

void yagl_host_glActiveTexture(GLenum texture);
void yagl_host_glBindBuffer(GLenum target,
    GLuint buffer);
void yagl_host_glBindTexture(GLenum target,
    GLuint texture);
void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor);
void yagl_host_glBufferData(GLenum target,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_,
    GLenum usage);
void yagl_host_glBufferSubData(GLenum target,
    GLintptr offset,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_);
void yagl_host_glClear(GLbitfield mask);
void yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
void yagl_host_glClearDepthf(GLclampf depth);
void yagl_host_glClearStencil(GLint s);
void yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha);
void yagl_host_glCompressedTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_);
void yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_);
void yagl_host_glCopyTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border);
void yagl_host_glCopyTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
void yagl_host_glCullFace(GLenum mode);
void yagl_host_glDeleteBuffers(GLsizei n,
    target_ulong /* const GLuint* */ buffers_);
void yagl_host_glDeleteTextures(GLsizei n,
    target_ulong /* const GLuint* */ textures_);
void yagl_host_glDepthFunc(GLenum func);
void yagl_host_glDepthMask(GLboolean flag);
void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar);
void yagl_host_glDisable(GLenum cap);
void yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count);
void yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_);
void yagl_host_glEnable(GLenum cap);
void yagl_host_glFinish(void);
void yagl_host_glFlush(void);
void yagl_host_glFrontFace(GLenum mode);
void yagl_host_glGenBuffers(GLsizei n,
    target_ulong /* GLuint* */ buffers_);
void yagl_host_glGenTextures(GLsizei n,
    target_ulong /* GLuint* */ textures_);
void yagl_host_glGetBooleanv(GLenum pname,
    target_ulong /* GLboolean* */ params_);
void yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params);
GLenum yagl_host_glGetError(void);
void yagl_host_glGetFloatv(GLenum pname,
    target_ulong /* GLfloat* */ params_);
void yagl_host_glGetIntegerv(GLenum pname,
    target_ulong /* GLint* */ params_);
void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* GLfloat* */ params);
void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params);
void yagl_host_glHint(GLenum target,
    GLenum mode);
GLboolean yagl_host_glIsBuffer(GLuint buffer);
GLboolean yagl_host_glIsEnabled(GLenum cap);
GLboolean yagl_host_glIsTexture(GLuint texture);
void yagl_host_glLineWidth(GLfloat width);
void yagl_host_glPixelStorei(GLenum pname,
    GLint param);
void yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units);
void yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* GLvoid* */ pixels);
void yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert);
void yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
void yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask);
void yagl_host_glStencilMask(GLuint mask);
void yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass);
void yagl_host_glTexImage2D(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_);
void yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param);
void yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param);
void yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params);
void yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_);
void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);

#endif
