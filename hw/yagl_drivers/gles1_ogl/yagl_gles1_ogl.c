#include <GL/gl.h>
#include "yagl_gles1_ogl.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl_macros.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl.h"
#include "yagl_gles1_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_tls.h"
#include <GL/glx.h>

static YAGL_DEFINE_TLS(struct yagl_thread_state*, gles1_ogl_ts);

struct yagl_gles1_ogl_ps
{
    struct yagl_gles1_driver_ps base;
};

struct yagl_gles1_ogl
{
    struct yagl_gles1_driver base;

    struct yagl_dyn_lib *dyn_lib;

    struct yagl_gles_ogl *gles_ogl;

    YAGL_GLES_OGL_PROC2(glAlphaFunc, GLenum, GLclampf, func, ref)
    YAGL_GLES_OGL_PROC4(glColor4f, GLfloat, GLfloat, GLfloat, GLfloat, red, green, blue, alpha)
    YAGL_GLES_OGL_PROC2(glDepthRangef, GLclampf, GLclampf, zNear, zFar)
    YAGL_GLES_OGL_PROC2(glFogf, GLenum, GLfloat, pname, param)
    YAGL_GLES_OGL_PROC2(glFogfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_OGL_PROC3(glGetLightfv, GLenum, GLenum, GLfloat*, light, pname, params)
    YAGL_GLES_OGL_PROC3(glGetMaterialfv, GLenum, GLenum, GLfloat*, face, pname, params)
    YAGL_GLES_OGL_PROC3(glGetTexEnvfv, GLenum, GLenum, GLfloat*, env, pname, params)
    YAGL_GLES_OGL_PROC2(glLightModelf, GLenum, GLfloat, pname, param)
    YAGL_GLES_OGL_PROC2(glLightModelfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_OGL_PROC3(glLightf, GLenum, GLenum, GLfloat, light, pname, param)
    YAGL_GLES_OGL_PROC3(glLightfv, GLenum, GLenum, const GLfloat*, light, pname, params)
    YAGL_GLES_OGL_PROC1(glLoadMatrixf, const GLfloat*, m)
    YAGL_GLES_OGL_PROC3(glMaterialf, GLenum, GLenum, GLfloat, face, pname, param)
    YAGL_GLES_OGL_PROC3(glMaterialfv, GLenum, GLenum, const GLfloat*, face, pname, params)
    YAGL_GLES_OGL_PROC1(glMultMatrixf, const GLfloat*, m)
    YAGL_GLES_OGL_PROC5(glMultiTexCoord4f, GLenum, GLfloat, GLfloat, GLfloat, GLfloat, target, s, t, r, q)
    YAGL_GLES_OGL_PROC3(glNormal3f, GLfloat, GLfloat, GLfloat, nx, ny, nz)
    YAGL_GLES_OGL_PROC2(glPointParameterf, GLenum, GLfloat, pname, param)
    YAGL_GLES_OGL_PROC2(glPointParameterfv, GLenum, const GLfloat*, pname, params)
    YAGL_GLES_OGL_PROC1(glPointSize, GLfloat, size)
    YAGL_GLES_OGL_PROC4(glRotatef, GLfloat, GLfloat, GLfloat, GLfloat, angle, x, y, z)
    YAGL_GLES_OGL_PROC3(glScalef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_OGL_PROC3(glTexEnvf, GLenum, GLenum, GLfloat, target, pname, param)
    YAGL_GLES_OGL_PROC3(glTexEnvfv, GLenum, GLenum, const GLfloat*, target, pname, params)
    YAGL_GLES_OGL_PROC3(glTranslatef, GLfloat, GLfloat, GLfloat, x, y, z)
    YAGL_GLES_OGL_PROC1(glClientActiveTexture, GLenum, texture)
    YAGL_GLES_OGL_PROC4(glColor4ub, GLubyte, GLubyte, GLubyte, GLubyte, red, green, blue, alpha)
    YAGL_GLES_OGL_PROC4(glColorPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_OGL_PROC1(glDisableClientState, GLenum, array)
    YAGL_GLES_OGL_PROC1(glEnableClientState, GLenum, array)
    YAGL_GLES_OGL_PROC2(glGetPointerv, GLenum, GLvoid**, pname, params)
    YAGL_GLES_OGL_PROC3(glGetTexEnviv, GLenum, GLenum, GLint*, env, pname, params)
    YAGL_GLES_OGL_PROC0(glLoadIdentity)
    YAGL_GLES_OGL_PROC1(glLogicOp, GLenum, opcode)
    YAGL_GLES_OGL_PROC1(glMatrixMode, GLenum, mode)
    YAGL_GLES_OGL_PROC3(glNormalPointer, GLenum, GLsizei, const GLvoid*, type, stride, pointer)
    YAGL_GLES_OGL_PROC0(glPopMatrix)
    YAGL_GLES_OGL_PROC0(glPushMatrix)
    YAGL_GLES_OGL_PROC1(glShadeModel, GLenum, mode)
    YAGL_GLES_OGL_PROC4(glTexCoordPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
    YAGL_GLES_OGL_PROC3(glTexEnvi, GLenum, GLenum, GLint, target, pname, param)
    YAGL_GLES_OGL_PROC3(glTexEnviv, GLenum, GLenum, const GLint*, target, pname, params)
    YAGL_GLES_OGL_PROC4(glVertexPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
};

YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, AlphaFunc, GLenum, GLclampf, func, ref)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Color4f, GLfloat, GLfloat, GLfloat, GLfloat, red, green, blue, alpha)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, DepthRangef, GLclampf, GLclampf, zNear, zFar)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Fogf, GLenum, GLfloat, pname, param)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Fogfv, GLenum, const GLfloat*, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, GetLightfv, GLenum, GLenum, GLfloat*, light, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, GetMaterialfv, GLenum, GLenum, GLfloat*, face, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, GetTexEnvfv, GLenum, GLenum, GLfloat*, env, pname, params)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, LightModelf, GLenum, GLfloat, pname, param)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, LightModelfv, GLenum, const GLfloat*, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Lightf, GLenum, GLenum, GLfloat, light, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Lightfv, GLenum, GLenum, const GLfloat*, light, pname, params)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, LoadMatrixf, const GLfloat*, m)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Materialf, GLenum, GLenum, GLfloat, face, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Materialfv, GLenum, GLenum, const GLfloat*, face, pname, params)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, MultMatrixf, const GLfloat*, m)
YAGL_GLES_OGL_PROC_IMPL5(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, MultiTexCoord4f, GLenum, GLfloat, GLfloat, GLfloat, GLfloat, target, s, t, r, q)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Normal3f, GLfloat, GLfloat, GLfloat, nx, ny, nz)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, PointParameterf, GLenum, GLfloat, pname, param)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, PointParameterfv, GLenum, const GLfloat*, pname, params)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, PointSize, GLfloat, size)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Rotatef, GLfloat, GLfloat, GLfloat, GLfloat, angle, x, y, z)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Scalef, GLfloat, GLfloat, GLfloat, x, y, z)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, TexEnvf, GLenum, GLenum, GLfloat, target, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, TexEnvfv, GLenum, GLenum, const GLfloat*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Translatef, GLfloat, GLfloat, GLfloat, x, y, z)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, ClientActiveTexture, GLenum, texture)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, Color4ub, GLubyte, GLubyte, GLubyte, GLubyte, red, green, blue, alpha)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, ColorPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, DisableClientState, GLenum, array)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, EnableClientState, GLenum, array)
YAGL_GLES_OGL_PROC_IMPL2(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, GetPointerv, GLenum, GLvoid**, pname, params)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, GetTexEnviv, GLenum, GLenum, GLint*, env, pname, params)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, LoadIdentity)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, LogicOp, GLenum, opcode)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, MatrixMode, GLenum, mode)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, NormalPointer, GLenum, GLsizei, const GLvoid*, type, stride, pointer)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, PopMatrix)
YAGL_GLES_OGL_PROC_IMPL0(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, PushMatrix)
YAGL_GLES_OGL_PROC_IMPL1(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, ShadeModel, GLenum, mode)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, TexCoordPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, TexEnvi, GLenum, GLenum, GLint, target, pname, param)
YAGL_GLES_OGL_PROC_IMPL3(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, TexEnviv, GLenum, GLenum, const GLint*, target, pname, params)
YAGL_GLES_OGL_PROC_IMPL4(yagl_gles1_driver_ps, yagl_gles1_driver_ps, yagl_gles1_ogl, VertexPointer, GLint, GLenum, GLsizei, const GLvoid*, size, type, stride, pointer)

static void yagl_gles1_ogl_thread_init(struct yagl_gles1_driver_ps *driver_ps,
                                       struct yagl_thread_state *ts)
{
    gles1_ogl_ts = ts;

    YAGL_LOG_FUNC_ENTER_TS(gles1_ogl_ts, yagl_gles1_ogl_thread_init, NULL);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles1_ogl_thread_fini(struct yagl_gles1_driver_ps *driver_ps)
{
    YAGL_LOG_FUNC_ENTER_TS(gles1_ogl_ts, yagl_gles1_ogl_thread_fini, NULL);

    gles1_ogl_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_gles1_ogl_process_destroy(struct yagl_gles1_driver_ps *driver_ps)
{
    struct yagl_gles1_ogl_ps *gles1_ogl_ps = (struct yagl_gles1_ogl_ps*)driver_ps;
    struct yagl_gles_ogl_ps *gles_ogl_ps = NULL;

    YAGL_LOG_FUNC_ENTER(driver_ps->common->ps->id, 0, yagl_gles1_ogl_process_destroy, NULL);

    gles_ogl_ps = (struct yagl_gles_ogl_ps*)gles1_ogl_ps->base.common;

    yagl_gles1_driver_ps_cleanup(&gles1_ogl_ps->base);

    yagl_gles_ogl_ps_destroy(gles_ogl_ps);

    g_free(gles1_ogl_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_gles1_driver_ps
    *yagl_gles1_ogl_process_init(struct yagl_gles1_driver *driver,
                                 struct yagl_process_state *ps)
{
    struct yagl_gles1_ogl *gles1_ogl = (struct yagl_gles1_ogl*)driver;
    struct yagl_gles1_ogl_ps *gles1_ogl_ps =
        g_malloc0(sizeof(struct yagl_gles1_ogl_ps));
    struct yagl_gles_ogl_ps *gles_ogl_ps = NULL;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_gles1_ogl_process_init, NULL);

    gles_ogl_ps = yagl_gles_ogl_ps_create(gles1_ogl->gles_ogl, ps);
    assert(gles_ogl_ps);

    yagl_gles1_driver_ps_init(&gles1_ogl_ps->base, driver, &gles_ogl_ps->base);

    gles1_ogl_ps->base.thread_init = &yagl_gles1_ogl_thread_init;
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, AlphaFunc);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Color4f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, DepthRangef);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Fogf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Fogfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, GetLightfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, GetMaterialfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, GetTexEnvfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, LightModelf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, LightModelfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Lightf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Lightfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, LoadMatrixf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Materialf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Materialfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, MultMatrixf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, MultiTexCoord4f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Normal3f);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, PointParameterf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, PointParameterfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, PointSize);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Rotatef);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Scalef);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, TexEnvf);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, TexEnvfv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Translatef);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, ClientActiveTexture);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, Color4ub);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, ColorPointer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, DisableClientState);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, EnableClientState);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, GetPointerv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, GetTexEnviv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, LoadIdentity);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, LogicOp);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, MatrixMode);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, NormalPointer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, PopMatrix);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, PushMatrix);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, ShadeModel);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, TexCoordPointer);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, TexEnvi);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, TexEnviv);
    YAGL_GLES_OGL_ASSIGN_PROC(yagl_gles1_ogl, gles1_ogl_ps, VertexPointer);
    gles1_ogl_ps->base.thread_fini = &yagl_gles1_ogl_thread_fini;
    gles1_ogl_ps->base.destroy = &yagl_gles1_ogl_process_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles1_ogl_ps->base;
}

static void yagl_gles1_ogl_destroy(struct yagl_gles1_driver *driver)
{
    struct yagl_gles1_ogl *gles1_ogl = (struct yagl_gles1_ogl*)driver;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles1_ogl_destroy, NULL);

    yagl_gles_ogl_destroy(gles1_ogl->gles_ogl);

    yagl_dyn_lib_destroy(gles1_ogl->dyn_lib);

    yagl_gles1_driver_cleanup(&gles1_ogl->base);

    g_free(gles1_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles1_driver *yagl_gles1_ogl_create(void)
{
    struct yagl_gles1_ogl *gles1_ogl = NULL;
    PFNGLXGETPROCADDRESSPROC get_address = NULL;
    struct yagl_dyn_lib *dyn_lib = NULL;
    struct yagl_gles_ogl *gles_ogl = NULL;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_gles1_ogl_create, NULL);

    gles1_ogl = g_malloc0(sizeof(*gles1_ogl));

    yagl_gles1_driver_init(&gles1_ogl->base);

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

    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glAlphaFunc);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glColor4f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glDepthRangef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glFogf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glFogfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glGetLightfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glGetMaterialfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glGetTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLightModelf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLightModelfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLightf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLightfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLoadMatrixf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glMaterialf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glMaterialfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glMultMatrixf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glMultiTexCoord4f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glNormal3f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glPointParameterf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glPointParameterfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glPointSize);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glRotatef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glScalef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTexEnvf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTranslatef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glClientActiveTexture);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glColor4ub);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glColorPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glDisableClientState);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glEnableClientState);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glGetPointerv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glGetTexEnviv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLoadIdentity);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glLogicOp);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glMatrixMode);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glNormalPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glPopMatrix);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glPushMatrix);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glShadeModel);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTexCoordPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTexEnvi);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glTexEnviv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, glVertexPointer);

    gles1_ogl->dyn_lib = dyn_lib;
    gles1_ogl->gles_ogl = gles_ogl;
    gles1_ogl->base.process_init = &yagl_gles1_ogl_process_init;
    gles1_ogl->base.destroy = &yagl_gles1_ogl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &gles1_ogl->base;

fail:
    if (gles_ogl) {
        yagl_gles_ogl_destroy(gles_ogl);
    }
    if (dyn_lib) {
        yagl_dyn_lib_destroy(dyn_lib);
    }
    yagl_gles1_driver_cleanup(&gles1_ogl->base);
    g_free(gles1_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
