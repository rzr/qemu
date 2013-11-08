#ifndef _QEMU_YAGL_HOST_GLES_CALLS_H
#define _QEMU_YAGL_HOST_GLES_CALLS_H

#include "yagl_api.h"
#include <GL/gl.h>

struct yagl_api_ps *yagl_host_gles_process_init(struct yagl_api *api);

void yagl_host_glDrawArrays(GLenum mode,
    GLint first,
    GLsizei count);
void yagl_host_glDrawElements(GLenum mode,
    GLsizei count,
    GLenum type,
    const GLvoid *indices, int32_t indices_count);
void yagl_host_glReadPixels(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLvoid *pixels, int32_t pixels_maxcount, int32_t *pixels_count);
void yagl_host_glGenVertexArrays(const GLuint *arrays, int32_t arrays_count);
void yagl_host_glBindVertexArray(GLuint array);
void yagl_host_glDisableVertexAttribArray(GLuint index);
void yagl_host_glEnableVertexAttribArray(GLuint index);
void yagl_host_glVertexAttribPointerData(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glVertexAttribPointerOffset(GLuint indx,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    GLsizei offset);
void yagl_host_glVertexPointerData(GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glVertexPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset);
void yagl_host_glNormalPointerData(GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glNormalPointerOffset(GLenum type,
    GLsizei stride,
    GLsizei offset);
void yagl_host_glColorPointerData(GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glColorPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset);
void yagl_host_glTexCoordPointerData(GLint tex_id,
    GLint size,
    GLenum type,
    GLsizei stride,
    GLint first,
    const GLvoid *data, int32_t data_count);
void yagl_host_glTexCoordPointerOffset(GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei offset);
void yagl_host_glDisableClientState(GLenum array);
void yagl_host_glEnableClientState(GLenum array);
void yagl_host_glGenBuffers(const GLuint *buffers, int32_t buffers_count);
void yagl_host_glBindBuffer(GLenum target,
    GLuint buffer);
void yagl_host_glBufferData(GLenum target,
    const GLvoid *data, int32_t data_count,
    GLenum usage);
void yagl_host_glBufferSubData(GLenum target,
    GLsizei offset,
    const GLvoid *data, int32_t data_count);
void yagl_host_glGenTextures(const GLuint *textures, int32_t textures_count);
void yagl_host_glBindTexture(GLenum target,
    GLuint texture);
void yagl_host_glActiveTexture(GLenum texture);
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
void yagl_host_glGetTexParameterfv(GLenum target,
    GLenum pname,
    GLfloat *param);
void yagl_host_glGetTexParameteriv(GLenum target,
    GLenum pname,
    GLint *param);
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
void yagl_host_glClientActiveTexture(GLenum texture);
void yagl_host_glTexEnvi(GLenum target,
    GLenum pname,
    GLint param);
void yagl_host_glTexEnvf(GLenum target,
    GLenum pname,
    GLfloat param);
void yagl_host_glMultiTexCoord4f(GLenum target,
    GLfloat s,
    GLfloat tt,
    GLfloat r,
    GLfloat q);
void yagl_host_glTexEnviv(GLenum target,
    GLenum pname,
    const GLint *params, int32_t params_count);
void yagl_host_glTexEnvfv(GLenum target,
    GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glGetTexEnviv(GLenum env,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetTexEnvfv(GLenum env,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGenFramebuffers(const GLuint *framebuffers, int32_t framebuffers_count);
void yagl_host_glBindFramebuffer(GLenum target,
    GLuint framebuffer);
void yagl_host_glFramebufferTexture2D(GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level);
void yagl_host_glFramebufferRenderbuffer(GLenum target,
    GLenum attachment,
    GLenum renderbuffertarget,
    GLuint renderbuffer);
void yagl_host_glBlitFramebuffer(GLint srcX0,
    GLint srcY0,
    GLint srcX1,
    GLint srcY1,
    GLint dstX0,
    GLint dstY0,
    GLint dstX1,
    GLint dstY1,
    GLbitfield mask,
    GLenum filter);
void yagl_host_glDrawBuffers(const GLenum *bufs, int32_t bufs_count);
void yagl_host_glGenRenderbuffers(const GLuint *renderbuffers, int32_t renderbuffers_count);
void yagl_host_glBindRenderbuffer(GLenum target,
    GLuint renderbuffer);
void yagl_host_glRenderbufferStorage(GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height);
void yagl_host_glGetRenderbufferParameteriv(GLenum target,
    GLenum pname,
    GLint *param);
void yagl_host_glCreateProgram(GLuint program);
void yagl_host_glCreateShader(GLuint shader,
    GLenum type);
void yagl_host_glShaderSource(GLuint shader,
    const GLchar *string, int32_t string_count);
void yagl_host_glAttachShader(GLuint program,
    GLuint shader);
void yagl_host_glDetachShader(GLuint program,
    GLuint shader);
void yagl_host_glCompileShader(GLuint shader);
void yagl_host_glBindAttribLocation(GLuint program,
    GLuint index,
    const GLchar *name, int32_t name_count);
void yagl_host_glGetActiveAttrib(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count);
void yagl_host_glGetActiveUniform(GLuint program,
    GLuint index,
    GLint *size,
    GLenum *type,
    GLchar *name, int32_t name_maxcount, int32_t *name_count);
int yagl_host_glGetAttribLocation(GLuint program,
    const GLchar *name, int32_t name_count);
void yagl_host_glGetProgramiv(GLuint program,
    GLenum pname,
    GLint *param);
GLboolean yagl_host_glGetProgramInfoLog(GLuint program,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count);
void yagl_host_glGetShaderiv(GLuint shader,
    GLenum pname,
    GLint *param);
GLboolean yagl_host_glGetShaderInfoLog(GLuint shader,
    GLchar *infolog, int32_t infolog_maxcount, int32_t *infolog_count);
void yagl_host_glGetUniformfv(GLboolean tl,
    GLuint program,
    uint32_t location,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetUniformiv(GLboolean tl,
    GLuint program,
    uint32_t location,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
int yagl_host_glGetUniformLocation(GLuint program,
    const GLchar *name, int32_t name_count);
void yagl_host_glGetVertexAttribfv(GLuint index,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetVertexAttribiv(GLuint index,
    GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glLinkProgram(GLuint program,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glUniform1f(GLboolean tl,
    uint32_t location,
    GLfloat x);
void yagl_host_glUniform1fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform1i(GLboolean tl,
    uint32_t location,
    GLint x);
void yagl_host_glUniform1iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform2f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y);
void yagl_host_glUniform2fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform2i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y);
void yagl_host_glUniform2iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform3f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glUniform3fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform3i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y,
    GLint z);
void yagl_host_glUniform3iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniform4f(GLboolean tl,
    uint32_t location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glUniform4fv(GLboolean tl,
    uint32_t location,
    const GLfloat *v, int32_t v_count);
void yagl_host_glUniform4i(GLboolean tl,
    uint32_t location,
    GLint x,
    GLint y,
    GLint z,
    GLint w);
void yagl_host_glUniform4iv(GLboolean tl,
    uint32_t location,
    const GLint *v, int32_t v_count);
void yagl_host_glUniformMatrix2fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUniformMatrix3fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUniformMatrix4fv(GLboolean tl,
    uint32_t location,
    GLboolean transpose,
    const GLfloat *value, int32_t value_count);
void yagl_host_glUseProgram(GLuint program);
void yagl_host_glValidateProgram(GLuint program);
void yagl_host_glVertexAttrib1f(GLuint indx,
    GLfloat x);
void yagl_host_glVertexAttrib1fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib2f(GLuint indx,
    GLfloat x,
    GLfloat y);
void yagl_host_glVertexAttrib2fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib3f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glVertexAttrib3fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glVertexAttrib4f(GLuint indx,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w);
void yagl_host_glVertexAttrib4fv(GLuint indx,
    const GLfloat *values, int32_t values_count);
void yagl_host_glGetActiveUniformsiv(GLuint program,
    const GLuint *uniformIndices, int32_t uniformIndices_count,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetIntegerv(GLenum pname,
    GLint *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetFloatv(GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glGetString(GLenum name,
    GLchar *str, int32_t str_maxcount, int32_t *str_count);
GLboolean yagl_host_glIsEnabled(GLenum cap);
void yagl_host_glDeleteObjects(const GLuint *objects, int32_t objects_count);
void yagl_host_glBlendEquation(GLenum mode);
void yagl_host_glBlendEquationSeparate(GLenum modeRGB,
    GLenum modeAlpha);
void yagl_host_glBlendFunc(GLenum sfactor,
    GLenum dfactor);
void yagl_host_glBlendFuncSeparate(GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha);
void yagl_host_glBlendColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha);
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
void yagl_host_glCullFace(GLenum mode);
void yagl_host_glDepthFunc(GLenum func);
void yagl_host_glDepthMask(GLboolean flag);
void yagl_host_glDepthRangef(GLclampf zNear,
    GLclampf zFar);
void yagl_host_glEnable(GLenum cap);
void yagl_host_glDisable(GLenum cap);
void yagl_host_glFlush(void);
void yagl_host_glFrontFace(GLenum mode);
void yagl_host_glGenerateMipmap(GLenum target);
void yagl_host_glHint(GLenum target,
    GLenum mode);
void yagl_host_glLineWidth(GLfloat width);
void yagl_host_glPixelStorei(GLenum pname,
    GLint param);
void yagl_host_glPolygonOffset(GLfloat factor,
    GLfloat units);
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
void yagl_host_glSampleCoverage(GLclampf value,
    GLboolean invert);
void yagl_host_glViewport(GLint x,
    GLint y,
    GLsizei width,
    GLsizei height);
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
void yagl_host_glPointSize(GLfloat size);
void yagl_host_glAlphaFunc(GLenum func,
    GLclampf ref);
void yagl_host_glMatrixMode(GLenum mode);
void yagl_host_glLoadIdentity(void);
void yagl_host_glPopMatrix(void);
void yagl_host_glPushMatrix(void);
void yagl_host_glRotatef(GLfloat angle,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glTranslatef(GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glScalef(GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glOrthof(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar);
void yagl_host_glColor4f(GLfloat red,
    GLfloat green,
    GLfloat blue,
    GLfloat alpha);
void yagl_host_glColor4ub(GLubyte red,
    GLubyte green,
    GLubyte blue,
    GLubyte alpha);
void yagl_host_glNormal3f(GLfloat nx,
    GLfloat ny,
    GLfloat nz);
void yagl_host_glPointParameterf(GLenum pname,
    GLfloat param);
void yagl_host_glPointParameterfv(GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glFogf(GLenum pname,
    GLfloat param);
void yagl_host_glFogfv(GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glFrustumf(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar);
void yagl_host_glLightf(GLenum light,
    GLenum pname,
    GLfloat param);
void yagl_host_glLightfv(GLenum light,
    GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glGetLightfv(GLenum light,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glLightModelf(GLenum pname,
    GLfloat param);
void yagl_host_glLightModelfv(GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glMaterialf(GLenum face,
    GLenum pname,
    GLfloat param);
void yagl_host_glMaterialfv(GLenum face,
    GLenum pname,
    const GLfloat *params, int32_t params_count);
void yagl_host_glGetMaterialfv(GLenum face,
    GLenum pname,
    GLfloat *params, int32_t params_maxcount, int32_t *params_count);
void yagl_host_glShadeModel(GLenum mode);
void yagl_host_glLogicOp(GLenum opcode);
void yagl_host_glMultMatrixf(const GLfloat *m, int32_t m_count);
void yagl_host_glLoadMatrixf(const GLfloat *m, int32_t m_count);
void yagl_host_glClipPlanef(GLenum plane,
    const GLfloat *equation, int32_t equation_count);
void yagl_host_glGetClipPlanef(GLenum pname,
    GLfloat *eqn, int32_t eqn_maxcount, int32_t *eqn_count);
void yagl_host_glUpdateOffscreenImageYAGL(GLuint texture,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    const void *pixels, int32_t pixels_count);
void yagl_host_glGenUniformLocationYAGL(uint32_t location,
    GLuint program,
    const GLchar *name, int32_t name_count);
void yagl_host_glDeleteUniformLocationsYAGL(const uint32_t *locations, int32_t locations_count);

#endif
