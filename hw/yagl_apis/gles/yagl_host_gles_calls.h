#ifndef _QEMU_YAGL_HOST_GLES_CALLS_H
#define _QEMU_YAGL_HOST_GLES_CALLS_H

#include "yagl_api.h"

bool yagl_host_glActiveTexture(GLenum texture);
bool yagl_host_glBindBuffer(GLenum target,
    GLuint buffer);
bool yagl_host_glBindTexture(GLenum target,
    GLuint texture);
bool yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor);
bool yagl_host_glBufferData(GLenum target,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_,
    GLenum usage);
bool yagl_host_glBufferSubData(GLenum target,
    GLintptr offset,
    GLsizeiptr size,
    target_ulong /* const GLvoid* */ data_);
bool yagl_host_glClear(GLbitfield mask);
bool yagl_host_glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
bool yagl_host_glClearDepthf(GLclampf depth);
bool yagl_host_glClearStencil(GLint s);
bool yagl_host_glColorMask(GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha);
bool yagl_host_glCompressedTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_);
bool yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLsizei imageSize,
    target_ulong /* const GLvoid* */ data_);
bool yagl_host_glCopyTexImage2D(GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border);
bool yagl_host_glCopyTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
bool yagl_host_glCullFace(GLenum mode);
bool yagl_host_glDeleteBuffers(GLsizei n,
    target_ulong /* const GLuint* */ buffers_);
bool yagl_host_glDeleteTextures(GLsizei n,
    target_ulong /* const GLuint* */ textures_);
bool yagl_host_glDepthFunc(GLenum func);
bool yagl_host_glDepthMask(GLboolean flag);
bool yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar);
bool yagl_host_glDisable(GLenum cap);
bool yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count);
bool yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_);
bool yagl_host_glEnable(GLenum cap);
bool yagl_host_glFinish(void);
bool yagl_host_glFlush(void);
bool yagl_host_glFrontFace(GLenum mode);
bool yagl_host_glGenBuffers(GLsizei n,
    target_ulong /* GLuint* */ buffers_);
bool yagl_host_glGenTextures(GLsizei n,
    target_ulong /* GLuint* */ textures_);
bool yagl_host_glGetBooleanv(GLenum pname,
    target_ulong /* GLboolean* */ params_);
bool yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetError(GLenum* retval);
bool yagl_host_glGetFloatv(GLenum pname,
    target_ulong /* GLfloat* */ params_);
bool yagl_host_glGetIntegerv(GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* GLfloat* */ params_);
bool yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* GLint* */ params_);
bool yagl_host_glHint(GLenum target,
    GLenum mode);
bool yagl_host_glIsBuffer(GLboolean* retval, GLuint buffer);
bool yagl_host_glIsEnabled(GLboolean* retval, GLenum cap);
bool yagl_host_glIsTexture(GLboolean* retval, GLuint texture);
bool yagl_host_glLineWidth(GLfloat width);
bool yagl_host_glPixelStorei(GLenum pname,
    GLint param);
bool yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units);
bool yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* GLvoid* */ pixels_);
bool yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert);
bool yagl_host_glScissor(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
bool yagl_host_glStencilFunc(GLenum func,
    GLint ref,
    GLuint mask);
bool yagl_host_glStencilMask(GLuint mask);
bool yagl_host_glStencilOp(GLenum fail,
    GLenum zfail,
    GLenum zpass);
bool yagl_host_glTexImage2D(GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_);
bool yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param);
bool yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params_);
bool yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param);
bool yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params_);
bool yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    target_ulong /* const GLvoid* */ pixels_);
bool yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
bool yagl_host_glGetExtensionStringYAGL(GLuint* retval, target_ulong /* GLchar* */ str_);
bool yagl_host_glEGLImageTargetTexture2DYAGL(GLenum target,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* const void* */ pixels_);
bool yagl_host_glGetVertexAttribRangeYAGL(GLsizei count,
    GLenum type,
    target_ulong /* const GLvoid* */ indices_,
    target_ulong /* GLint* */ range_first_,
    target_ulong /* GLsizei* */ range_count_);

#endif
