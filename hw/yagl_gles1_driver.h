#ifndef _QEMU_YAGL_GLES1_DRIVER_H
#define _QEMU_YAGL_GLES1_DRIVER_H

#include "yagl_types.h"
#include "yagl_gles_driver.h"

/*
 * YaGL GLES1 driver per-process state.
 * @{
 */

struct yagl_gles1_driver_ps
{
    struct yagl_gles1_driver *driver;

    struct yagl_gles_driver_ps *common;

    void (*thread_init)(struct yagl_gles1_driver_ps */*driver_ps*/,
                        struct yagl_thread_state */*ts*/);

    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, AlphaFunc, GLenum, GLclampf, func, ref)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, Color4f, GLfloat, GLfloat, GLfloat, GLfloat, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, DepthRangef, GLclampf, GLclampf, zNear, zFar)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, Fogf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, Fogfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, GetLightfv, GLenum, GLenum, GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, GetMaterialfv, GLenum, GLenum, GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, GetTexEnvfv, GLenum, GLenum, GLfloat*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, LightModelf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, LightModelfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Lightf, GLenum, GLenum, GLfloat, light, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Lightfv, GLenum, GLenum, const GLfloat*, light, pname, params)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, LoadMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Materialf, GLenum, GLenum, GLfloat, face, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Materialfv, GLenum, GLenum, const GLfloat*, face, pname, params)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, MultMatrixf, const GLfloat*, m)
    YAGL_GLES_DRIVER_FUNC5(struct yagl_gles1_driver_ps *driver_ps, MultiTexCoord4f, GLenum, GLfloat, GLfloat, GLfloat, GLfloat, target, s, t, r, q)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Normal3f, GLfloat, GLfloat, GLfloat, nx, ny, nz)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, PointParameterf, GLenum, GLfloat, pname, param)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, PointParameterfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, PointSize, GLfloat, size)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, Rotatef, GLfloat, GLfloat, GLfloat, GLfloat, angle, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Scalef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, TexEnvf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, TexEnvfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, Translatef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, ClientActiveTexture, GLenum, texture)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, Color4ub, GLubyte, GLubyte, GLubyte, GLubyte, red, green, blue, alpha)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, ColorPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, DisableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, EnableClientState, GLenum, array)
    YAGL_GLES_DRIVER_FUNC2(struct yagl_gles1_driver_ps *driver_ps, GetPointerv, GLenum, GLvoid**, pname, params)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, GetTexEnviv, GLenum, GLenum, GLint*, env, pname, params)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles1_driver_ps *driver_ps, LoadIdentity)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, LogicOp, GLenum, opcode)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, MatrixMode, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, NormalPointer, GLenum, GLsizei, const GLvoid*, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles1_driver_ps *driver_ps, PopMatrix)
    YAGL_GLES_DRIVER_FUNC0(struct yagl_gles1_driver_ps *driver_ps, PushMatrix)
    YAGL_GLES_DRIVER_FUNC1(struct yagl_gles1_driver_ps *driver_ps, ShadeModel, GLenum, mode)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, TexCoordPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, TexEnvi, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_DRIVER_FUNC3(struct yagl_gles1_driver_ps *driver_ps, TexEnviv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_DRIVER_FUNC4(struct yagl_gles1_driver_ps *driver_ps, VertexPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)

    void (*thread_fini)(struct yagl_gles1_driver_ps */*driver_ps*/);

    void (*destroy)(struct yagl_gles1_driver_ps */*driver_ps*/);
};

void yagl_gles1_driver_ps_init(struct yagl_gles1_driver_ps *driver_ps,
                               struct yagl_gles1_driver *driver,
                               struct yagl_gles_driver_ps *common);
void yagl_gles1_driver_ps_cleanup(struct yagl_gles1_driver_ps *driver_ps);

/*
 * @}
 */

/*
 * YaGL GLES1 driver.
 * @{
 */

struct yagl_gles1_driver
{
    struct yagl_gles1_driver_ps *(*process_init)(struct yagl_gles1_driver */*driver*/,
                                                 struct yagl_process_state */*ps*/);

    void (*destroy)(struct yagl_gles1_driver */*driver*/);
};

void yagl_gles1_driver_init(struct yagl_gles1_driver *driver);
void yagl_gles1_driver_cleanup(struct yagl_gles1_driver *driver);

/*
 * @}
 */

#endif
