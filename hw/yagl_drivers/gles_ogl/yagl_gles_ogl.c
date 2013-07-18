#include <GL/gl.h>
#include "yagl_gles_driver.h"
#include "yagl_gles_ogl.h"
#include "yagl_gles_ogl_macros.h"
#include "yagl_log.h"
#include "yagl_dyn_lib.h"
#include "yagl_process.h"
#include "yagl_thread.h"

bool yagl_gles_ogl_init(struct yagl_gles_driver *driver,
                        struct yagl_dyn_lib *dyn_lib)
{
    YAGL_LOG_FUNC_ENTER(yagl_gles_ogl_init, NULL);

    yagl_gles_driver_init(driver);

    YAGL_GLES_OGL_GET_PROC(driver, ActiveTexture, glActiveTexture);
    YAGL_GLES_OGL_GET_PROC(driver, BindBuffer, glBindBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, BindTexture, glBindTexture);
    YAGL_GLES_OGL_GET_PROC(driver, BlendEquation, glBlendEquation);
    YAGL_GLES_OGL_GET_PROC(driver, BlendEquationSeparate, glBlendEquationSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, BlendFunc, glBlendFunc);
    YAGL_GLES_OGL_GET_PROC(driver, BlendFuncSeparate, glBlendFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, BufferData, glBufferData);
    YAGL_GLES_OGL_GET_PROC(driver, BufferSubData, glBufferSubData);
    YAGL_GLES_OGL_GET_PROC(driver, Clear, glClear);
    YAGL_GLES_OGL_GET_PROC(driver, ClearColor, glClearColor);
    YAGL_GLES_OGL_GET_PROC(driver, ClearDepth, glClearDepth);
    YAGL_GLES_OGL_GET_PROC(driver, ClearStencil, glClearStencil);
    YAGL_GLES_OGL_GET_PROC(driver, ColorMask, glColorMask);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexImage2D, glCompressedTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexSubImage2D, glCompressedTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CopyTexImage2D, glCopyTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CopyTexSubImage2D, glCopyTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CullFace, glCullFace);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteBuffers, glDeleteBuffers);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteTextures, glDeleteTextures);
    YAGL_GLES_OGL_GET_PROC(driver, DepthFunc, glDepthFunc);
    YAGL_GLES_OGL_GET_PROC(driver, DepthMask, glDepthMask);
    YAGL_GLES_OGL_GET_PROC(driver, DepthRange, glDepthRange);
    YAGL_GLES_OGL_GET_PROC(driver, Disable, glDisable);
    YAGL_GLES_OGL_GET_PROC(driver, DrawArrays, glDrawArrays);
    YAGL_GLES_OGL_GET_PROC(driver, DrawElements, glDrawElements);
    YAGL_GLES_OGL_GET_PROC(driver, Enable, glEnable);
    YAGL_GLES_OGL_GET_PROC(driver, Finish, glFinish);
    YAGL_GLES_OGL_GET_PROC(driver, Flush, glFlush);
    YAGL_GLES_OGL_GET_PROC(driver, FrontFace, glFrontFace);
    YAGL_GLES_OGL_GET_PROC(driver, GenBuffers, glGenBuffers);
    YAGL_GLES_OGL_GET_PROC(driver, GenTextures, glGenTextures);
    YAGL_GLES_OGL_GET_PROC(driver, GetBooleanv, glGetBooleanv);
    YAGL_GLES_OGL_GET_PROC(driver, GetBufferParameteriv, glGetBufferParameteriv);
    YAGL_GLES_OGL_GET_PROC(driver, GetError, glGetError);
    YAGL_GLES_OGL_GET_PROC(driver, GetFloatv, glGetFloatv);
    YAGL_GLES_OGL_GET_PROC(driver, GetIntegerv, glGetIntegerv);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexParameterfv, glGetTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexParameteriv, glGetTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(driver, Hint, glHint);
    YAGL_GLES_OGL_GET_PROC(driver, IsBuffer, glIsBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, IsEnabled, glIsEnabled);
    YAGL_GLES_OGL_GET_PROC(driver, IsTexture, glIsTexture);
    YAGL_GLES_OGL_GET_PROC(driver, LineWidth, glLineWidth);
    YAGL_GLES_OGL_GET_PROC(driver, PixelStorei, glPixelStorei);
    YAGL_GLES_OGL_GET_PROC(driver, PolygonOffset, glPolygonOffset);
    YAGL_GLES_OGL_GET_PROC(driver, ReadPixels, glReadPixels);
    YAGL_GLES_OGL_GET_PROC(driver, SampleCoverage, glSampleCoverage);
    YAGL_GLES_OGL_GET_PROC(driver, Scissor, glScissor);
    YAGL_GLES_OGL_GET_PROC(driver, StencilFunc, glStencilFunc);
    YAGL_GLES_OGL_GET_PROC(driver, StencilMask, glStencilMask);
    YAGL_GLES_OGL_GET_PROC(driver, StencilOp, glStencilOp);
    YAGL_GLES_OGL_GET_PROC(driver, TexImage2D, glTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameterf, glTexParameterf);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameterfv, glTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameteri, glTexParameteri);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameteriv, glTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(driver, TexSubImage2D, glTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, Viewport, glViewport);
    YAGL_GLES_OGL_GET_PROC(driver, PushClientAttrib, glPushClientAttrib);
    YAGL_GLES_OGL_GET_PROC(driver, PopClientAttrib, glPopClientAttrib);
    YAGL_GLES_OGL_GET_PROC(driver, MapBuffer, glMapBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, UnmapBuffer, glUnmapBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, GenFramebuffers, glGenFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteFramebuffers, glDeleteFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GenRenderbuffers, glGenRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteRenderbuffers, glDeleteRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BindFramebuffer, glBindFramebufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BindRenderbuffer, glBindRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, FramebufferTexture2D, glFramebufferTexture2DEXT);
    YAGL_GLES_OGL_GET_PROC(driver, RenderbufferStorage, glRenderbufferStorageEXT);
    YAGL_GLES_OGL_GET_PROC(driver, FramebufferRenderbuffer, glFramebufferRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, CheckFramebufferStatus, glCheckFramebufferStatusEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GenerateMipmap, glGenerateMipmapEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GetString, glGetString);
    YAGL_GLES_OGL_GET_PROC(driver, GetFramebufferAttachmentParameteriv, glGetFramebufferAttachmentParameterivEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GetRenderbufferParameteriv, glGetRenderbufferParameterivEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BlitFramebuffer, glBlitFramebufferEXT);

    YAGL_LOG_FUNC_EXIT(NULL);

    return true;

fail:
    yagl_gles_driver_cleanup(driver);

    YAGL_LOG_FUNC_EXIT(NULL);

    return false;
}

void yagl_gles_ogl_cleanup(struct yagl_gles_driver *driver)
{
    yagl_gles_driver_cleanup(driver);
}
