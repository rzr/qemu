#ifndef _QEMU_YAGL_GLES1_DRIVER_H
#define _QEMU_YAGL_GLES1_DRIVER_H

#include "yagl_types.h"
#include "yagl_gles_driver.h"

/*
 * YaGL GLES1 driver.
 * @{
 */

struct yagl_gles1_driver
{
    struct yagl_gles_driver base;

    YAGL_GLES_DRIVER_FUNC2(AlphaFunc, GLenum, GLclampf, func, ref)
    YAGL_GLES_DRIVER_FUNC4(Color4f, GLfloat, GLfloat, GLfloat, GLfloat, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC2(DepthRangef, GLclampf, GLclampf, zNear, zFar)
    YAGL_GLES_DRIVER_FUNC2(Fogf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(Fogfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC6(Frustum, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, left, right, bottom, top, znear, zfar)
    YAGL_GLES_DRIVER_FUNC3(GetLightfv, GLenum, GLenum, GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetMaterialfv, GLenum, GLenum, GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexEnvfv, GLenum, GLenum, GLfloat*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC2(LightModelf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(LightModelfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(Lightf, GLenum, GLenum, GLfloat, light, pname, param)
    YAGL_GLES_DRIVER_FUNC3(Lightfv, GLenum, GLenum, const GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC1(LoadMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC3(Materialf, GLenum, GLenum, GLfloat, face, pname, param)
    YAGL_GLES_DRIVER_FUNC3(Materialfv, GLenum, GLenum, const GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC1(MultMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC5(MultiTexCoord4f, GLenum, GLfloat, GLfloat, GLfloat, GLfloat, target, s, t, r, q)
    YAGL_GLES_DRIVER_FUNC3(Normal3f, GLfloat, GLfloat, GLfloat, nx, ny, nz)
    YAGL_GLES_DRIVER_FUNC6(Ortho, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, yagl_GLdouble, left, right, bottom, top, znear, zfar)
    YAGL_GLES_DRIVER_FUNC2(PointParameterf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(PointParameterfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC1(PointSize, GLfloat, size)
    YAGL_GLES_DRIVER_FUNC4(Rotatef, GLfloat, GLfloat, GLfloat, GLfloat, angle, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(Scalef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(TexEnvf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexEnvfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(Translatef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC1(ClientActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC2(ClipPlane, GLenum, const yagl_GLdouble *, plane, equation)
    YAGL_GLES_DRIVER_FUNC4(Color4ub, GLubyte, GLubyte, GLubyte, GLubyte, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC4(ColorPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC1(DisableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC1(EnableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC2(GetClipPlane, GLenum, const yagl_GLdouble *, plane, equation)
    YAGL_GLES_DRIVER_FUNC2(GetPointerv, GLenum, GLvoid**, pname, params)
    YAGL_GLES_DRIVER_FUNC3(GetTexEnviv, GLenum, GLenum, GLint*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC0(LoadIdentity)
    YAGL_GLES_DRIVER_FUNC1(LogicOp, GLenum, opcode)
    YAGL_GLES_DRIVER_FUNC1(MatrixMode, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC3(NormalPointer, GLenum, GLsizei, const GLvoid*, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC0(PopMatrix)
    YAGL_GLES_DRIVER_FUNC0(PushMatrix)
    YAGL_GLES_DRIVER_FUNC1(ShadeModel, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC4(TexCoordPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC3(TexEnvi, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(TexEnviv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC4(VertexPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)

    void (*destroy)(struct yagl_gles1_driver */*driver*/);
};

void yagl_gles1_driver_init(struct yagl_gles1_driver *driver);
void yagl_gles1_driver_cleanup(struct yagl_gles1_driver *driver);

/*
 * @}
 */

#endif
