#include <GL/gl.h>
#include "yagl_gles1_ogl.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl_macros.h"
#include "yagl_drivers/gles_ogl/yagl_gles_ogl.h"
#include "yagl_gles1_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_gles1_ogl_destroy(struct yagl_gles1_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_gles1_ogl_destroy, NULL);

    yagl_gles1_driver_cleanup(driver);
    yagl_gles_ogl_cleanup(&driver->base);
    g_free(driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles1_driver *yagl_gles1_ogl_create(struct yagl_dyn_lib *dyn_lib)
{
    struct yagl_gles1_driver *gles1_ogl = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles1_ogl_create, NULL);

    gles1_ogl = g_malloc0(sizeof(*gles1_ogl));

    if (!yagl_gles_ogl_init(&gles1_ogl->base, dyn_lib)) {
        goto fail_free;
    }

    yagl_gles1_driver_init(gles1_ogl);

    YAGL_GLES_OGL_GET_PROC(gles1_ogl, AlphaFunc, glAlphaFunc);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Color4f, glColor4f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, DepthRangef, glDepthRangef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Frustum, glFrustum);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Fogf, glFogf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Fogfv, glFogfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetLightfv, glGetLightfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetMaterialfv, glGetMaterialfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetTexEnvfv, glGetTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, LightModelf, glLightModelf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, LightModelfv, glLightModelfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Lightf, glLightf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Lightfv, glLightfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, LoadMatrixf, glLoadMatrixf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Materialf, glMaterialf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Materialfv, glMaterialfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, MultMatrixf, glMultMatrixf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, MultiTexCoord4f, glMultiTexCoord4f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Normal3f, glNormal3f);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Ortho, glOrtho);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, PointParameterf, glPointParameterf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, PointParameterfv, glPointParameterfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, PointSize, glPointSize);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Rotatef, glRotatef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Scalef, glScalef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, TexEnvf, glTexEnvf);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, TexEnvfv, glTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Translatef, glTranslatef);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, ClientActiveTexture, glClientActiveTexture);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, ClipPlane, glClipPlane);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, Color4ub, glColor4ub);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, ColorPointer, glColorPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, DisableClientState, glDisableClientState);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, EnableClientState, glEnableClientState);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetClipPlane, glGetClipPlane);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetPointerv, glGetPointerv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, GetTexEnviv, glGetTexEnviv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, LoadIdentity, glLoadIdentity);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, LogicOp, glLogicOp);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, MatrixMode, glMatrixMode);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, NormalPointer, glNormalPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, PopMatrix, glPopMatrix);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, PushMatrix, glPushMatrix);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, ShadeModel, glShadeModel);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, TexCoordPointer, glTexCoordPointer);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, TexEnvi, glTexEnvi);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, TexEnviv, glTexEnviv);
    YAGL_GLES_OGL_GET_PROC(gles1_ogl, VertexPointer, glVertexPointer);

    gles1_ogl->destroy = &yagl_gles1_ogl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return gles1_ogl;

fail:
    yagl_gles1_driver_cleanup(gles1_ogl);
    yagl_gles_ogl_cleanup(&gles1_ogl->base);
fail_free:
    g_free(gles1_ogl);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
