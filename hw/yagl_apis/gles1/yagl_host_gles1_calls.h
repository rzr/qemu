#ifndef _QEMU_YAGL_HOST_GLES1_CALLS_H
#define _QEMU_YAGL_HOST_GLES1_CALLS_H

#include "yagl_api.h"
#include <GLES/gl.h>
#include "yagl_apis/gles/yagl_host_gles_calls.h"

void yagl_host_glAlphaFunc(GLenum func,
    GLclampf ref);
void yagl_host_glClipPlanef(GLenum plane,
    target_ulong /* const GLfloat* */ equation);
void yagl_host_glColor4f(GLfloat red,
    GLfloat green,
    GLfloat blue,
    GLfloat alpha);
void yagl_host_glFogf(GLenum pname,
    GLfloat param);
void yagl_host_glFogfv(GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glFrustumf(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar);
void yagl_host_glGetClipPlanef(GLenum pname,
    target_ulong /* GLfloat* */ eqn);
void yagl_host_glGetLightfv(GLenum light,
    GLenum pname,
    target_ulong /* GLfloat* */ params);
void yagl_host_glGetMaterialfv(GLenum face,
    GLenum pname,
    target_ulong /* GLfloat* */ params);
void yagl_host_glGetTexEnvfv(GLenum env,
    GLenum pname,
    target_ulong /* GLfloat* */ params);
void yagl_host_glLightModelf(GLenum pname,
    GLfloat param);
void yagl_host_glLightModelfv(GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glLightf(GLenum light,
    GLenum pname,
    GLfloat param);
void yagl_host_glLightfv(GLenum light,
    GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glLoadMatrixf(target_ulong /* const GLfloat* */ m);
void yagl_host_glMaterialf(GLenum face,
    GLenum pname,
    GLfloat param);
void yagl_host_glMaterialfv(GLenum face,
    GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glMultMatrixf(target_ulong /* const GLfloat* */ m);
void yagl_host_glMultiTexCoord4f(GLenum target,
    GLfloat s,
    GLfloat t,
    GLfloat r,
    GLfloat q);
void yagl_host_glNormal3f(GLfloat nx,
    GLfloat ny,
    GLfloat nz);
void yagl_host_glOrthof(GLfloat left,
    GLfloat right,
    GLfloat bottom,
    GLfloat top,
    GLfloat zNear,
    GLfloat zFar);
void yagl_host_glPointParameterf(GLenum pname,
    GLfloat param);
void yagl_host_glPointParameterfv(GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glPointSize(GLfloat size);
void yagl_host_glRotatef(GLfloat angle,
    GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glScalef(GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glTexEnvf(GLenum target,
    GLenum pname,
    GLfloat param);
void yagl_host_glTexEnvfv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfloat* */ params);
void yagl_host_glTranslatef(GLfloat x,
    GLfloat y,
    GLfloat z);
void yagl_host_glAlphaFuncx(GLenum func,
    GLclampx ref);
void yagl_host_glClearColorx(GLclampx red,
    GLclampx green,
    GLclampx blue,
    GLclampx alpha);
void yagl_host_glClearDepthx(GLclampx depth);
void yagl_host_glClientActiveTexture(GLenum texture);
void yagl_host_glClipPlanex(GLenum plane,
    target_ulong /* const GLfixed* */ equation);
void yagl_host_glColor4ub(GLubyte red,
    GLubyte green,
    GLubyte blue,
    GLubyte alpha);
void yagl_host_glColor4x(GLfixed red,
    GLfixed green,
    GLfixed blue,
    GLfixed alpha);
void yagl_host_glColorPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer);
void yagl_host_glDepthRangex(GLclampx zNear,
    GLclampx zFar);
void yagl_host_glDisableClientState(GLenum array);
void yagl_host_glEnableClientState(GLenum array);
void yagl_host_glFogx(GLenum pname,
    GLfixed param);
void yagl_host_glFogxv(GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glFrustumx(GLfixed left,
    GLfixed right,
    GLfixed bottom,
    GLfixed top,
    GLfixed zNear,
    GLfixed zFar);
void yagl_host_glGetClipPlanex(GLenum pname,
    target_ulong /* GLfixed* */ eqn);
void yagl_host_glGetFixedv(GLenum pname,
    target_ulong /* GLfixed* */ params);
void yagl_host_glGetLightxv(GLenum light,
    GLenum pname,
    target_ulong /* GLfixed* */ params);
void yagl_host_glGetMaterialxv(GLenum face,
    GLenum pname,
    target_ulong /* GLfixed* */ params);
void yagl_host_glGetPointerv(GLenum pname,
    target_ulong /* GLvoid** */ params);
void yagl_host_glGetTexEnviv(GLenum env,
    GLenum pname,
    target_ulong /* GLint* */ params);
void yagl_host_glGetTexEnvxv(GLenum env,
    GLenum pname,
    target_ulong /* GLfixed* */ params);
void yagl_host_glGetTexParameterxv(GLenum target,
    GLenum pname,
    target_ulong /* GLfixed* */ params);
void yagl_host_glLightModelx(GLenum pname,
    GLfixed param);
void yagl_host_glLightModelxv(GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glLightx(GLenum light,
    GLenum pname,
    GLfixed param);
void yagl_host_glLightxv(GLenum light,
    GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glLineWidthx(GLfixed width);
void yagl_host_glLoadIdentity(void);
void yagl_host_glLoadMatrixx(target_ulong /* const GLfixed* */ m);
void yagl_host_glLogicOp(GLenum opcode);
void yagl_host_glMaterialx(GLenum face,
    GLenum pname,
    GLfixed param);
void yagl_host_glMaterialxv(GLenum face,
    GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glMatrixMode(GLenum mode);
void yagl_host_glMultMatrixx(target_ulong /* const GLfixed* */ m);
void yagl_host_glMultiTexCoord4x(GLenum target,
    GLfixed s,
    GLfixed t,
    GLfixed r,
    GLfixed q);
void yagl_host_glNormal3x(GLfixed nx,
    GLfixed ny,
    GLfixed nz);
void yagl_host_glNormalPointer(GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer);
void yagl_host_glOrthox(GLfixed left,
    GLfixed right,
    GLfixed bottom,
    GLfixed top,
    GLfixed zNear,
    GLfixed zFar);
void yagl_host_glPointParameterx(GLenum pname,
    GLfixed param);
void yagl_host_glPointParameterxv(GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glPointSizex(GLfixed size);
void yagl_host_glPolygonOffsetx(GLfixed factor,
    GLfixed units);
void yagl_host_glPopMatrix(void);
void yagl_host_glPushMatrix(void);
void yagl_host_glRotatex(GLfixed angle,
    GLfixed x,
    GLfixed y,
    GLfixed z);
void yagl_host_glSampleCoveragex(GLclampx value,
    GLboolean invert);
void yagl_host_glScalex(GLfixed x,
    GLfixed y,
    GLfixed z);
void yagl_host_glShadeModel(GLenum mode);
void yagl_host_glTexCoordPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer);
void yagl_host_glTexEnvi(GLenum target,
    GLenum pname,
    GLint param);
void yagl_host_glTexEnvx(GLenum target,
    GLenum pname,
    GLfixed param);
void yagl_host_glTexEnviv(GLenum target,
    GLenum pname,
    target_ulong /* const GLint* */ params);
void yagl_host_glTexEnvxv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glTexParameterx(GLenum target,
    GLenum pname,
    GLfixed param);
void yagl_host_glTexParameterxv(GLenum target,
    GLenum pname,
    target_ulong /* const GLfixed* */ params);
void yagl_host_glTranslatex(GLfixed x,
    GLfixed y,
    GLfixed z);
void yagl_host_glVertexPointer(GLint size,
    GLenum type,
    GLsizei stride,
    target_ulong /* const GLvoid* */ pointer);

#endif
