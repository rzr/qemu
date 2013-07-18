#include <GL/gl.h>
#include "yagl_gles_onscreen.h"
#include "yagl_gles_driver.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_backends/egl_onscreen/yagl_egl_onscreen_ts.h"
#include "yagl_backends/egl_onscreen/yagl_egl_onscreen_context.h"

YAGL_DECLARE_TLS(struct yagl_egl_onscreen_ts*, egl_onscreen_ts);

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_GetBooleanv(GLenum pname, GLboolean* params)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    if ((pname == GL_FRAMEBUFFER_BINDING) && params) {
        egl_onscreen_ts->gles_driver->GetIntegerv(pname, (GLint*)&cur_fb);
        params[0] = (cur_fb == egl_onscreen_ts->ctx->fb) ? 0 : cur_fb;
    } else {
        egl_onscreen_ts->gles_driver->GetBooleanv(pname, params);
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_GetFloatv(GLenum pname, GLfloat* params)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    if ((pname == GL_FRAMEBUFFER_BINDING) && params) {
        egl_onscreen_ts->gles_driver->GetIntegerv(pname, (GLint*)&cur_fb);
        params[0] = (cur_fb == egl_onscreen_ts->ctx->fb) ? 0 : cur_fb;
    } else {
        egl_onscreen_ts->gles_driver->GetFloatv(pname, params);
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_GetIntegerv(GLenum pname, GLint* params)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    if ((pname == GL_FRAMEBUFFER_BINDING) && params) {
        egl_onscreen_ts->gles_driver->GetIntegerv(pname, (GLint*)&cur_fb);
        params[0] = (cur_fb == egl_onscreen_ts->ctx->fb) ? 0 : cur_fb;
    } else {
        egl_onscreen_ts->gles_driver->GetIntegerv(pname, params);
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_DeleteFramebuffers(GLsizei n,
    const GLuint* framebuffers)
{
    GLsizei i;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    for (i = 0; i < n; ++i) {
        if (framebuffers[i] != egl_onscreen_ts->ctx->fb) {
            egl_onscreen_ts->gles_driver->DeleteBuffers(1, &framebuffers[i]);
        }
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_BindFramebuffer(GLenum target,
    GLuint framebuffer)
{
    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    if (framebuffer == egl_onscreen_ts->ctx->fb) {
        /*
         * TODO: Generate GL_INVALID_OPERATION error.
         */

        return;
    }

    if (framebuffer == 0) {
        framebuffer = egl_onscreen_ts->ctx->fb;
    }

    egl_onscreen_ts->gles_driver->BindFramebuffer(target, framebuffer);
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_FramebufferTexture2D(GLenum target,
    GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen_ts->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                              (GLint*)&cur_fb);

    if (cur_fb != egl_onscreen_ts->ctx->fb) {
        egl_onscreen_ts->gles_driver->FramebufferTexture2D(target,
                                                           attachment,
                                                           textarget,
                                                           texture,
                                                           level);
    } else {
        /*
         * TODO: Generate GL_INVALID_OPERATION error.
         */
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_FramebufferRenderbuffer(GLenum target,
    GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen_ts->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                              (GLint*)&cur_fb);

    if (cur_fb != egl_onscreen_ts->ctx->fb) {
        egl_onscreen_ts->gles_driver->FramebufferRenderbuffer(target,
                                                              attachment,
                                                              renderbuffertarget,
                                                              renderbuffer);
    } else {
        /*
         * TODO: Generate GL_INVALID_OPERATION error.
         */
    }
}

static GLenum YAGL_GLES_APIENTRY yagl_gles_onscreen_CheckFramebufferStatus(GLenum target)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    }

    egl_onscreen_ts->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                              (GLint*)&cur_fb);

    if (cur_fb != egl_onscreen_ts->ctx->fb) {
        return egl_onscreen_ts->gles_driver->CheckFramebufferStatus(target);
    } else {
        return GL_FRAMEBUFFER_COMPLETE;
    }
}

static void YAGL_GLES_APIENTRY yagl_gles_onscreen_GetFramebufferAttachmentParameteriv(GLenum target,
    GLenum attachment, GLenum pname, GLint* params)
{
    GLuint cur_fb = 0;

    if (!egl_onscreen_ts || !egl_onscreen_ts->dpy) {
        return;
    }

    egl_onscreen_ts->gles_driver->GetIntegerv(GL_FRAMEBUFFER_BINDING,
                                              (GLint*)&cur_fb);

    if (cur_fb != egl_onscreen_ts->ctx->fb) {
        egl_onscreen_ts->gles_driver->GetFramebufferAttachmentParameteriv(target,
                                                                          attachment,
                                                                          pname,
                                                                          params);
    } else {
        /*
         * TODO: Generate GL_INVALID_OPERATION error.
         */
    }
}

void yagl_gles_onscreen_init(struct yagl_gles_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_gles_onscreen_init, NULL);

    yagl_gles_driver_init(driver);

    driver->GetBooleanv = &yagl_gles_onscreen_GetBooleanv;
    driver->GetFloatv = &yagl_gles_onscreen_GetFloatv;
    driver->GetIntegerv = &yagl_gles_onscreen_GetIntegerv;
    driver->DeleteFramebuffers = &yagl_gles_onscreen_DeleteFramebuffers;
    driver->BindFramebuffer = &yagl_gles_onscreen_BindFramebuffer;
    driver->FramebufferTexture2D = &yagl_gles_onscreen_FramebufferTexture2D;
    driver->FramebufferRenderbuffer = &yagl_gles_onscreen_FramebufferRenderbuffer;
    driver->CheckFramebufferStatus = &yagl_gles_onscreen_CheckFramebufferStatus;
    driver->GetFramebufferAttachmentParameteriv = &yagl_gles_onscreen_GetFramebufferAttachmentParameteriv;

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_gles_onscreen_cleanup(struct yagl_gles_driver *driver)
{
    yagl_gles_driver_cleanup(driver);
}
