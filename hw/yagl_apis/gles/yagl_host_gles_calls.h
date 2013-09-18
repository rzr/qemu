#ifndef _QEMU_YAGL_HOST_GLES_CALLS_H
#define _QEMU_YAGL_HOST_GLES_CALLS_H

#include "yagl_api.h"

struct yagl_client_image;

void yagl_host_glActiveTexture(GLenum texture);
void yagl_host_glBindBuffer(GLenum target,
    GLuint buffer);
void yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer);
void yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer);
void yagl_host_glBindTexture(GLenum target,
    GLuint texture);
void yagl_host_glBlendEquation(GLenum mode);
void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha);
void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor);
void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha);
void yagl_host_glBufferData(GLenum target,
    const GLvoid *data, int32_t data_count,
    GLenum usage);
void yagl_host_glBufferSubData(GLenum target,
    GLsizei offset,
    const GLvoid *data, int32_t data_count);
GLenum yagl_host_glCheckFramebufferStatus(GLenum target);
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
    const GLvoid *data, int32_t data_count);
void yagl_host_glCompressedTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    const GLvoid *data, int32_t data_count);
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
void yagl_host_glDeleteBuffers(const GLuint *buffers, int32_t buffers_count);
void yagl_host_glDeleteFramebuffers(const GLuint *framebuffers, int32_t framebuffers_count);
void yagl_host_glDeleteRenderbuffers(const GLuint *renderbuffers, int32_t renderbuffers_count);
void yagl_host_glDeleteTextures(const GLuint *textures, int32_t textures_count);
void yagl_host_glDepthFunc(GLenum func);
void yagl_host_glDepthMask(GLboolean flag);
void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar);
void yagl_host_glDisable(GLenum cap);
void yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count);
void yagl_host_glEGLImageTargetTexture2DOES(GLenum target,
    yagl_host_handle image);
void yagl_host_glEnable(GLenum cap);
void yagl_host_glFlush(void);
void yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level);
void yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer);
void yagl_host_glFrontFace(GLenum mode);
void yagl_host_glGenBuffers(GLuint *buffers, int32_t buffers_maxcount, int32_t *buffers_count);
void yagl_host_glGenerateMipmap(GLenum target);
void yagl_host_glGenFramebuffers(GLuint *framebuffers, int32_t framebuffers_maxcount, int32_t *framebuffers_count);
void yagl_host_glGenRenderbuffers(GLuint *renderbuffers, int32_t renderbuffers_maxcount, int32_t *renderbuffers_count);
void yagl_host_glGenTextures(GLuint *textures, int32_t textures_maxcount, int32_t *textures_count);
void yagl_host_glGetBooleanv(GLenum pname,
    GLboolean *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetBufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param);
GLenum yagl_host_glGetError(void);
void yagl_host_glGetFloatv(GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment,
    GLenum pname,
    GLint *param);
void yagl_host_glGetIntegerv(GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param);
void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    GLfloat *param);
void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    GLint *param);
void yagl_host_glHint(GLenum target,
    GLenum mode);
GLboolean yagl_host_glIsBuffer(GLuint buffer);
GLboolean yagl_host_glIsEnabled(GLenum cap);
GLboolean yagl_host_glIsFramebuffer(GLuint framebuffer);
GLboolean yagl_host_glIsRenderbuffer(GLuint renderbuffer);
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
    GLvoid *pixels, int32_t pixels_maxcount, int32_t *pixels_count);
void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height);
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
    const GLvoid *pixels, int32_t pixels_count);
void yagl_host_glTexParameterf(GLenum target,
    GLenum pname,
    GLfloat param);
void yagl_host_glTexParameterfv(GLenum target,
    GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glTexParameteri(GLenum target,
    GLenum pname,
    GLint param);
void yagl_host_glTexParameteriv(GLenum target,
    GLenum pname,
    const GLint *params, int32_t params_count);
void yagl_host_glTexSubImage2D(GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    const GLvoid *pixels, int32_t pixels_count);
void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);

void yagl_host_glTransferArrayYAGL(GLuint indx,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glDrawElementsIndicesYAGL(GLenum mode,
    GLenum type,
    const GLvoid *indices, int32_t indices_count);
void yagl_host_glDrawElementsOffsetYAGL(GLenum mode,
    GLenum type,
    GLsizei offset,
    GLsizei count);
void yagl_host_glGetExtensionStringYAGL(GLchar *str, int32_t str_maxcount, int32_t *str_count);
void yagl_host_glGetVertexAttribRangeYAGL(GLenum type,
    GLsizei offset,
    GLsizei count,
    GLint *range_first,
    GLsizei *range_count);
void yagl_host_glEGLUpdateOffscreenImageYAGL(struct yagl_client_image *image,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    const void *pixels);

#endif
