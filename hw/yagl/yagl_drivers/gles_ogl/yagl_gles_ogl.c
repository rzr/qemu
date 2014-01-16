/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include <GL/gl.h>
#include "yagl_gles_driver.h"
#include "yagl_gles_ogl.h"
#include "yagl_gles_ogl_macros.h"
#include "yagl_log.h"
#include "yagl_dyn_lib.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static void yagl_gles_ogl_destroy(struct yagl_gles_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_gles_ogl_destroy, NULL);

    yagl_gles_driver_cleanup(driver);
    g_free(driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_gles_driver *yagl_gles_ogl_create(struct yagl_dyn_lib *dyn_lib,
                                              yagl_gl_version gl_version)
{
    struct yagl_gles_driver *driver = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_gles_ogl_create, NULL);

    driver = g_malloc0(sizeof(*driver));

    yagl_gles_driver_init(driver, gl_version);

    YAGL_GLES_OGL_GET_PROC(driver, DrawArrays, glDrawArrays);
    YAGL_GLES_OGL_GET_PROC(driver, DrawElements, glDrawElements);
    YAGL_GLES_OGL_GET_PROC(driver, ReadPixels, glReadPixels);
    YAGL_GLES_OGL_GET_PROC(driver, DisableVertexAttribArray, glDisableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(driver, EnableVertexAttribArray, glEnableVertexAttribArray);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttribPointer, glVertexAttribPointer);
    YAGL_GLES_OGL_GET_PROC(driver, VertexPointer, glVertexPointer);
    YAGL_GLES_OGL_GET_PROC(driver, NormalPointer, glNormalPointer);
    YAGL_GLES_OGL_GET_PROC(driver, ColorPointer, glColorPointer);
    YAGL_GLES_OGL_GET_PROC(driver, TexCoordPointer, glTexCoordPointer);
    YAGL_GLES_OGL_GET_PROC(driver, DisableClientState, glDisableClientState);
    YAGL_GLES_OGL_GET_PROC(driver, EnableClientState, glEnableClientState);
    YAGL_GLES_OGL_GET_PROC(driver, GenBuffers, glGenBuffers);
    YAGL_GLES_OGL_GET_PROC(driver, BindBuffer, glBindBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, BufferData, glBufferData);
    YAGL_GLES_OGL_GET_PROC(driver, BufferSubData, glBufferSubData);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteBuffers, glDeleteBuffers);
    YAGL_GLES_OGL_GET_PROC(driver, GenTextures, glGenTextures);
    YAGL_GLES_OGL_GET_PROC(driver, BindTexture, glBindTexture);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteTextures, glDeleteTextures);
    YAGL_GLES_OGL_GET_PROC(driver, ActiveTexture, glActiveTexture);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexImage2D, glCompressedTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexSubImage2D, glCompressedTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CopyTexImage2D, glCopyTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, CopyTexSubImage2D, glCopyTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexParameterfv, glGetTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexParameteriv, glGetTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(driver, TexImage2D, glTexImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameterf, glTexParameterf);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameterfv, glTexParameterfv);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameteri, glTexParameteri);
    YAGL_GLES_OGL_GET_PROC(driver, TexParameteriv, glTexParameteriv);
    YAGL_GLES_OGL_GET_PROC(driver, TexSubImage2D, glTexSubImage2D);
    YAGL_GLES_OGL_GET_PROC(driver, ClientActiveTexture, glClientActiveTexture);
    YAGL_GLES_OGL_GET_PROC(driver, TexEnvi, glTexEnvi);
    YAGL_GLES_OGL_GET_PROC(driver, TexEnvf, glTexEnvf);
    YAGL_GLES_OGL_GET_PROC(driver, MultiTexCoord4f, glMultiTexCoord4f);
    YAGL_GLES_OGL_GET_PROC(driver, TexEnviv, glTexEnviv);
    YAGL_GLES_OGL_GET_PROC(driver, TexEnvfv, glTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexEnviv, glGetTexEnviv);
    YAGL_GLES_OGL_GET_PROC(driver, GetTexEnvfv, glGetTexEnvfv);
    YAGL_GLES_OGL_GET_PROC(driver, TexImage3D, glTexImage3D);
    YAGL_GLES_OGL_GET_PROC(driver, TexSubImage3D, glTexSubImage3D);
    YAGL_GLES_OGL_GET_PROC(driver, CopyTexSubImage3D, glCopyTexSubImage3D);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexImage3D, glCompressedTexImage3D);
    YAGL_GLES_OGL_GET_PROC(driver, CompressedTexSubImage3D, glCompressedTexSubImage3D);
    YAGL_GLES_OGL_GET_PROC(driver, GenFramebuffers, glGenFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BindFramebuffer, glBindFramebufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, FramebufferTexture2D, glFramebufferTexture2DEXT);
    YAGL_GLES_OGL_GET_PROC(driver, FramebufferRenderbuffer, glFramebufferRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteFramebuffers, glDeleteFramebuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BlitFramebuffer, glBlitFramebufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, DrawBuffers, glDrawBuffers);
    YAGL_GLES_OGL_GET_PROC(driver, FramebufferTexture3D, glFramebufferTexture3DEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GenRenderbuffers, glGenRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, BindRenderbuffer, glBindRenderbufferEXT);
    YAGL_GLES_OGL_GET_PROC(driver, RenderbufferStorage, glRenderbufferStorageEXT);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteRenderbuffers, glDeleteRenderbuffersEXT);
    YAGL_GLES_OGL_GET_PROC(driver, GetRenderbufferParameteriv, glGetRenderbufferParameterivEXT);
    YAGL_GLES_OGL_GET_PROC(driver, CreateProgram, glCreateProgram);
    YAGL_GLES_OGL_GET_PROC(driver, CreateShader, glCreateShader);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteProgram, glDeleteProgram);
    YAGL_GLES_OGL_GET_PROC(driver, DeleteShader, glDeleteShader);
    YAGL_GLES_OGL_GET_PROC(driver, ShaderSource, glShaderSource);
    YAGL_GLES_OGL_GET_PROC(driver, AttachShader, glAttachShader);
    YAGL_GLES_OGL_GET_PROC(driver, DetachShader, glDetachShader);
    YAGL_GLES_OGL_GET_PROC(driver, CompileShader, glCompileShader);
    YAGL_GLES_OGL_GET_PROC(driver, BindAttribLocation, glBindAttribLocation);
    YAGL_GLES_OGL_GET_PROC(driver, GetActiveAttrib, glGetActiveAttrib);
    YAGL_GLES_OGL_GET_PROC(driver, GetActiveUniform, glGetActiveUniform);
    YAGL_GLES_OGL_GET_PROC(driver, GetAttribLocation, glGetAttribLocation);
    YAGL_GLES_OGL_GET_PROC(driver, GetProgramiv, glGetProgramiv);
    YAGL_GLES_OGL_GET_PROC(driver, GetProgramInfoLog, glGetProgramInfoLog);
    YAGL_GLES_OGL_GET_PROC(driver, GetShaderiv, glGetShaderiv);
    YAGL_GLES_OGL_GET_PROC(driver, GetShaderInfoLog, glGetShaderInfoLog);
    YAGL_GLES_OGL_GET_PROC(driver, GetUniformfv, glGetUniformfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetUniformiv, glGetUniformiv);
    YAGL_GLES_OGL_GET_PROC(driver, GetUniformLocation, glGetUniformLocation);
    YAGL_GLES_OGL_GET_PROC(driver, GetVertexAttribfv, glGetVertexAttribfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetVertexAttribiv, glGetVertexAttribiv);
    YAGL_GLES_OGL_GET_PROC(driver, LinkProgram, glLinkProgram);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform1f, glUniform1f);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform1fv, glUniform1fv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform1i, glUniform1i);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform1iv, glUniform1iv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform2f, glUniform2f);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform2fv, glUniform2fv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform2i, glUniform2i);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform2iv, glUniform2iv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform3f, glUniform3f);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform3fv, glUniform3fv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform3i, glUniform3i);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform3iv, glUniform3iv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform4f, glUniform4f);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform4fv, glUniform4fv);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform4i, glUniform4i);
    YAGL_GLES_OGL_GET_PROC(driver, Uniform4iv, glUniform4iv);
    YAGL_GLES_OGL_GET_PROC(driver, UniformMatrix2fv, glUniformMatrix2fv);
    YAGL_GLES_OGL_GET_PROC(driver, UniformMatrix3fv, glUniformMatrix3fv);
    YAGL_GLES_OGL_GET_PROC(driver, UniformMatrix4fv, glUniformMatrix4fv);
    YAGL_GLES_OGL_GET_PROC(driver, UseProgram, glUseProgram);
    YAGL_GLES_OGL_GET_PROC(driver, ValidateProgram, glValidateProgram);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib1f, glVertexAttrib1f);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib1fv, glVertexAttrib1fv);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib2f, glVertexAttrib2f);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib2fv, glVertexAttrib2fv);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib3f, glVertexAttrib3f);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib3fv, glVertexAttrib3fv);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib4f, glVertexAttrib4f);
    YAGL_GLES_OGL_GET_PROC(driver, VertexAttrib4fv, glVertexAttrib4fv);
    YAGL_GLES_OGL_GET_PROC(driver, GetIntegerv, glGetIntegerv);
    YAGL_GLES_OGL_GET_PROC(driver, GetFloatv, glGetFloatv);
    YAGL_GLES_OGL_GET_PROC(driver, GetString, glGetString);
    YAGL_GLES_OGL_GET_PROC(driver, IsEnabled, glIsEnabled);
    YAGL_GLES_OGL_GET_PROC(driver, BlendEquation, glBlendEquation);
    YAGL_GLES_OGL_GET_PROC(driver, BlendEquationSeparate, glBlendEquationSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, BlendFunc, glBlendFunc);
    YAGL_GLES_OGL_GET_PROC(driver, BlendFuncSeparate, glBlendFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, BlendColor, glBlendColor);
    YAGL_GLES_OGL_GET_PROC(driver, Clear, glClear);
    YAGL_GLES_OGL_GET_PROC(driver, ClearColor, glClearColor);
    YAGL_GLES_OGL_GET_PROC(driver, ClearDepth, glClearDepth);
    YAGL_GLES_OGL_GET_PROC(driver, ClearStencil, glClearStencil);
    YAGL_GLES_OGL_GET_PROC(driver, ColorMask, glColorMask);
    YAGL_GLES_OGL_GET_PROC(driver, CullFace, glCullFace);
    YAGL_GLES_OGL_GET_PROC(driver, DepthFunc, glDepthFunc);
    YAGL_GLES_OGL_GET_PROC(driver, DepthMask, glDepthMask);
    YAGL_GLES_OGL_GET_PROC(driver, DepthRange, glDepthRange);
    YAGL_GLES_OGL_GET_PROC(driver, Enable, glEnable);
    YAGL_GLES_OGL_GET_PROC(driver, Disable, glDisable);
    YAGL_GLES_OGL_GET_PROC(driver, Flush, glFlush);
    YAGL_GLES_OGL_GET_PROC(driver, FrontFace, glFrontFace);
    YAGL_GLES_OGL_GET_PROC(driver, GenerateMipmap, glGenerateMipmapEXT);
    YAGL_GLES_OGL_GET_PROC(driver, Hint, glHint);
    YAGL_GLES_OGL_GET_PROC(driver, LineWidth, glLineWidth);
    YAGL_GLES_OGL_GET_PROC(driver, PixelStorei, glPixelStorei);
    YAGL_GLES_OGL_GET_PROC(driver, PolygonOffset, glPolygonOffset);
    YAGL_GLES_OGL_GET_PROC(driver, Scissor, glScissor);
    YAGL_GLES_OGL_GET_PROC(driver, StencilFunc, glStencilFunc);
    YAGL_GLES_OGL_GET_PROC(driver, StencilMask, glStencilMask);
    YAGL_GLES_OGL_GET_PROC(driver, StencilOp, glStencilOp);
    YAGL_GLES_OGL_GET_PROC(driver, SampleCoverage, glSampleCoverage);
    YAGL_GLES_OGL_GET_PROC(driver, Viewport, glViewport);
    YAGL_GLES_OGL_GET_PROC(driver, StencilFuncSeparate, glStencilFuncSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, StencilMaskSeparate, glStencilMaskSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, StencilOpSeparate, glStencilOpSeparate);
    YAGL_GLES_OGL_GET_PROC(driver, PointSize, glPointSize);
    YAGL_GLES_OGL_GET_PROC(driver, AlphaFunc, glAlphaFunc);
    YAGL_GLES_OGL_GET_PROC(driver, MatrixMode, glMatrixMode);
    YAGL_GLES_OGL_GET_PROC(driver, LoadIdentity, glLoadIdentity);
    YAGL_GLES_OGL_GET_PROC(driver, PopMatrix, glPopMatrix);
    YAGL_GLES_OGL_GET_PROC(driver, PushMatrix, glPushMatrix);
    YAGL_GLES_OGL_GET_PROC(driver, Rotatef, glRotatef);
    YAGL_GLES_OGL_GET_PROC(driver, Translatef, glTranslatef);
    YAGL_GLES_OGL_GET_PROC(driver, Scalef, glScalef);
    YAGL_GLES_OGL_GET_PROC(driver, Ortho, glOrtho);
    YAGL_GLES_OGL_GET_PROC(driver, Color4f, glColor4f);
    YAGL_GLES_OGL_GET_PROC(driver, Color4ub, glColor4ub);
    YAGL_GLES_OGL_GET_PROC(driver, Normal3f, glNormal3f);
    YAGL_GLES_OGL_GET_PROC(driver, PointParameterf, glPointParameterf);
    YAGL_GLES_OGL_GET_PROC(driver, PointParameterfv, glPointParameterfv);
    YAGL_GLES_OGL_GET_PROC(driver, Fogf, glFogf);
    YAGL_GLES_OGL_GET_PROC(driver, Fogfv, glFogfv);
    YAGL_GLES_OGL_GET_PROC(driver, Frustum, glFrustum);
    YAGL_GLES_OGL_GET_PROC(driver, Lightf, glLightf);
    YAGL_GLES_OGL_GET_PROC(driver, Lightfv, glLightfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetLightfv, glGetLightfv);
    YAGL_GLES_OGL_GET_PROC(driver, LightModelf, glLightModelf);
    YAGL_GLES_OGL_GET_PROC(driver, LightModelfv, glLightModelfv);
    YAGL_GLES_OGL_GET_PROC(driver, Materialf, glMaterialf);
    YAGL_GLES_OGL_GET_PROC(driver, Materialfv, glMaterialfv);
    YAGL_GLES_OGL_GET_PROC(driver, GetMaterialfv, glGetMaterialfv);
    YAGL_GLES_OGL_GET_PROC(driver, ShadeModel, glShadeModel);
    YAGL_GLES_OGL_GET_PROC(driver, LogicOp, glLogicOp);
    YAGL_GLES_OGL_GET_PROC(driver, MultMatrixf, glMultMatrixf);
    YAGL_GLES_OGL_GET_PROC(driver, LoadMatrixf, glLoadMatrixf);
    YAGL_GLES_OGL_GET_PROC(driver, ClipPlane, glClipPlane);
    YAGL_GLES_OGL_GET_PROC(driver, GetClipPlane, glGetClipPlane);
    YAGL_GLES_OGL_GET_PROC(driver, MapBuffer, glMapBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, UnmapBuffer, glUnmapBuffer);
    YAGL_GLES_OGL_GET_PROC(driver, Finish, glFinish);

    if (gl_version > yagl_gl_2) {
        YAGL_GLES_OGL_GET_PROC(driver, GetStringi, glGetStringi);
        YAGL_GLES_OGL_GET_PROC(driver, GenVertexArrays, glGenVertexArrays);
        YAGL_GLES_OGL_GET_PROC(driver, BindVertexArray, glBindVertexArray);
        YAGL_GLES_OGL_GET_PROC(driver, DeleteVertexArrays, glDeleteVertexArrays);
        YAGL_GLES_OGL_GET_PROC(driver, GetActiveUniformsiv, glGetActiveUniformsiv);
        YAGL_GLES_OGL_GET_PROC(driver, GetUniformIndices, glGetUniformIndices);
        YAGL_GLES_OGL_GET_PROC(driver, GetUniformBlockIndex, glGetUniformBlockIndex);
        YAGL_GLES_OGL_GET_PROC(driver, UniformBlockBinding, glUniformBlockBinding);
        YAGL_GLES_OGL_GET_PROC(driver, BindBufferBase, glBindBufferBase);
        YAGL_GLES_OGL_GET_PROC(driver, BindBufferRange, glBindBufferRange);
        YAGL_GLES_OGL_GET_PROC(driver, GetActiveUniformBlockName, glGetActiveUniformBlockName);
        YAGL_GLES_OGL_GET_PROC(driver, GetActiveUniformBlockiv, glGetActiveUniformBlockiv);
        YAGL_GLES_OGL_GET_PROC(driver, GenTransformFeedbacks, glGenTransformFeedbacks);
        YAGL_GLES_OGL_GET_PROC(driver, BindTransformFeedback, glBindTransformFeedback);
        YAGL_GLES_OGL_GET_PROC(driver, BeginTransformFeedback, glBeginTransformFeedback);
        YAGL_GLES_OGL_GET_PROC(driver, EndTransformFeedback, glEndTransformFeedback);
        YAGL_GLES_OGL_GET_PROC(driver, PauseTransformFeedback, glPauseTransformFeedback);
        YAGL_GLES_OGL_GET_PROC(driver, ResumeTransformFeedback, glResumeTransformFeedback);
        YAGL_GLES_OGL_GET_PROC(driver, DeleteTransformFeedbacks, glDeleteTransformFeedbacks);
        YAGL_GLES_OGL_GET_PROC(driver, TransformFeedbackVaryings, glTransformFeedbackVaryings);
        YAGL_GLES_OGL_GET_PROC(driver, GetTransformFeedbackVarying, glGetTransformFeedbackVarying);
        YAGL_GLES_OGL_GET_PROC(driver, GenQueries, glGenQueries);
        YAGL_GLES_OGL_GET_PROC(driver, BeginQuery, glBeginQuery);
        YAGL_GLES_OGL_GET_PROC(driver, EndQuery, glEndQuery);
        YAGL_GLES_OGL_GET_PROC(driver, GetQueryObjectuiv, glGetQueryObjectuiv);
        YAGL_GLES_OGL_GET_PROC(driver, DeleteQueries, glDeleteQueries);
        YAGL_GLES_OGL_GET_PROC(driver, DrawArraysInstanced, glDrawArraysInstanced);
        YAGL_GLES_OGL_GET_PROC(driver, DrawElementsInstanced, glDrawElementsInstanced);
        YAGL_GLES_OGL_GET_PROC(driver, VertexAttribDivisor, glVertexAttribDivisor);
    }

    driver->destroy = &yagl_gles_ogl_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return driver;

fail:
    yagl_gles_driver_cleanup(driver);
    g_free(driver);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
