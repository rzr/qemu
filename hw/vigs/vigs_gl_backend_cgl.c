/*
 * vigs
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

#include "vigs_gl_backend.h"
#include "vigs_log.h"
#include <OpenGL/OpenGL.h>
#include <dlfcn.h>

#define LIBGL_IMAGE_NAME \
"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#ifndef kCGLPFAOpenGLProfile
#   define kCGLPFAOpenGLProfile 99
#   define kCGLOGLPVersion_3_2_Core 0x3200
#   define kCGLOGLPVersion_Legacy 0x1000
#endif

#define VIGS_GL_GET_PROC(func, proc_name) \
    do { \
        *(void**)(&gl_backend_cgl->base.func) = dlsym(gl_backend_cgl->handle, #proc_name); \
        if (!gl_backend_cgl->base.func) { \
            VIGS_LOG_CRITICAL("Unable to load " #proc_name " symbol"); \
            goto fail2; \
        } \
    } while (0)

#define VIGS_GL_GET_PROC_OPTIONAL(func, proc_name) \
    *(void**)(&gl_backend_cgl->base.func) = dlsym(gl_backend_cgl->handle, #proc_name)

static const CGLPixelFormatAttribute pixel_format_legacy_attrs[] =
{
    kCGLPFAAccelerated,
    kCGLPFAMinimumPolicy,
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 8,
    kCGLPFANoRecovery,
    kCGLPFAPBuffer,
    0
};

static const CGLPixelFormatAttribute pixel_format_3_2_core_attrs[] =
{
    kCGLPFAAccelerated,
    kCGLPFAMinimumPolicy,
    kCGLPFAColorSize, 32,
    kCGLPFAAlphaSize, 8,
    kCGLPFADepthSize, 24,
    kCGLPFAStencilSize, 8,
    kCGLPFANoRecovery,
    kCGLPFAOpenGLProfile, kCGLOGLPVersion_3_2_Core,
    0
};

struct vigs_gl_backend_cgl
{
    struct vigs_gl_backend base;

    void *handle;
    CGLContextObj read_pixels_context;
    CGLContextObj context;
};

static bool vigs_gl_backend_cgl_check_gl_version(struct vigs_gl_backend_cgl *gl_backend_cgl,
                                                 bool *is_gl_2)
{
    CGLError error;
    bool res = false;
    const char *tmp;
    CGLPixelFormatObj pixel_format;
    int n;

    tmp = getenv("GL_VERSION");

    if (tmp) {
        if (strcmp(tmp, "2") == 0) {
            VIGS_LOG_INFO("GL_VERSION forces OpenGL version to 2.1");
            *is_gl_2 = true;
            res = true;
        } else if (strcmp(tmp, "3_2") == 0) {
            VIGS_LOG_INFO("GL_VERSION forces OpenGL version to 3.2");
            *is_gl_2 = false;
            res = true;
        } else {
            VIGS_LOG_CRITICAL("Bad GL_VERSION value = %s", tmp);
        }

        goto out;
    }

    error = CGLChoosePixelFormat(pixel_format_3_2_core_attrs,
                                 &pixel_format,
                                 &n);

    if (error) {
        VIGS_LOG_INFO("CGLChoosePixelFormat failed for 3_2_core attrs: %s, using OpenGL 2.1", CGLErrorString(error));
        *is_gl_2 = true;
        res = true;
        goto out;
    }

    if (!pixel_format) {
        VIGS_LOG_INFO("CGLChoosePixelFormat failed to find formats for 3_2_core attrs, using OpenGL 2.1");
        *is_gl_2 = true;
        res = true;
        goto out;
    }

    CGLDestroyPixelFormat(pixel_format);

    VIGS_LOG_INFO("Using OpenGL 3.2");
    *is_gl_2 = false;
    res = true;

out:
    return res;
}

static bool vigs_gl_backend_cgl_create_context(struct vigs_gl_backend_cgl *gl_backend_cgl,
                                               CGLPixelFormatObj pixel_format,
                                               CGLContextObj share_context,
                                               CGLContextObj *ctx)
{
    CGLError error;

    error = CGLCreateContext(pixel_format, share_context, ctx);

    if (error) {
        VIGS_LOG_CRITICAL("CGLCreateContext failed: %s", CGLErrorString(error));
        return false;
    }

    return true;
}

static bool vigs_gl_backend_cgl_has_current(struct vigs_gl_backend *gl_backend)
{
    return CGLGetCurrentContext() != NULL;
}

static bool vigs_gl_backend_cgl_make_current(struct vigs_gl_backend *gl_backend,
                                             bool enable)
{
    struct vigs_gl_backend_cgl *gl_backend_cgl =
        (struct vigs_gl_backend_cgl*)gl_backend;
    CGLError error;

    error = CGLSetCurrentContext(enable ? gl_backend_cgl->context : NULL);

    if (error) {
        VIGS_LOG_CRITICAL("CGLSetCurrentContext failed: %s", CGLErrorString(error));
        return false;
    }

    return true;
}

static bool vigs_gl_backend_cgl_read_pixels_make_current(struct vigs_gl_backend *gl_backend,
                                                         bool enable)
{
    struct vigs_gl_backend_cgl *gl_backend_cgl =
        (struct vigs_gl_backend_cgl*)gl_backend;
    CGLError error;

    error = CGLSetCurrentContext(enable ? gl_backend_cgl->read_pixels_context : NULL);

    if (error) {
        VIGS_LOG_CRITICAL("CGLSetCurrentContext failed: %s", CGLErrorString(error));
        return false;
    }

    return true;
}

static void vigs_gl_backend_cgl_destroy(struct vigs_backend *backend)
{
    struct vigs_gl_backend_cgl *gl_backend_cgl = (struct vigs_gl_backend_cgl*)backend;
    CGLError error;

    vigs_gl_backend_cleanup(&gl_backend_cgl->base);

    error = CGLDestroyContext(gl_backend_cgl->context);

    if (error) {
        VIGS_LOG_ERROR("CGLDestroyContext failed: %s", CGLErrorString(error));
    }

    error = CGLDestroyContext(gl_backend_cgl->read_pixels_context);

    if (error) {
        VIGS_LOG_ERROR("CGLDestroyContext failed: %s", CGLErrorString(error));
    }

    dlclose(gl_backend_cgl->handle);

    vigs_backend_cleanup(&gl_backend_cgl->base.base);

    g_free(gl_backend_cgl);

    VIGS_LOG_DEBUG("destroyed");
}

struct vigs_backend *vigs_gl_backend_create(void *display)
{
    struct vigs_gl_backend_cgl *gl_backend_cgl;
    CGLError error;
    CGLPixelFormatObj pixel_format;
    int n;

    gl_backend_cgl = g_malloc0(sizeof(*gl_backend_cgl));

    vigs_backend_init(&gl_backend_cgl->base.base,
                      &gl_backend_cgl->base.ws_info.base);

    gl_backend_cgl->handle = dlopen(LIBGL_IMAGE_NAME, RTLD_NOW | RTLD_GLOBAL);

    if (!gl_backend_cgl->handle) {
        VIGS_LOG_CRITICAL("Unable to load " LIBGL_IMAGE_NAME ": %s", dlerror());
        goto fail1;
    }

    VIGS_GL_GET_PROC(GenTextures, glGenTextures);
    VIGS_GL_GET_PROC(DeleteTextures, glDeleteTextures);
    VIGS_GL_GET_PROC(BindTexture, glBindTexture);
    VIGS_GL_GET_PROC(CullFace, glCullFace);
    VIGS_GL_GET_PROC(TexParameterf, glTexParameterf);
    VIGS_GL_GET_PROC(TexParameterfv, glTexParameterfv);
    VIGS_GL_GET_PROC(TexParameteri, glTexParameteri);
    VIGS_GL_GET_PROC(TexParameteriv, glTexParameteriv);
    VIGS_GL_GET_PROC(TexImage2D, glTexImage2D);
    VIGS_GL_GET_PROC(TexSubImage2D, glTexSubImage2D);
    VIGS_GL_GET_PROC(Clear, glClear);
    VIGS_GL_GET_PROC(ClearColor, glClearColor);
    VIGS_GL_GET_PROC(Disable, glDisable);
    VIGS_GL_GET_PROC(Enable, glEnable);
    VIGS_GL_GET_PROC(Finish, glFinish);
    VIGS_GL_GET_PROC(Flush, glFlush);
    VIGS_GL_GET_PROC(PixelStorei, glPixelStorei);
    VIGS_GL_GET_PROC(ReadPixels, glReadPixels);
    VIGS_GL_GET_PROC(Viewport, glViewport);
    VIGS_GL_GET_PROC(GenFramebuffers, glGenFramebuffersEXT);
    VIGS_GL_GET_PROC(GenRenderbuffers, glGenRenderbuffersEXT);
    VIGS_GL_GET_PROC(DeleteFramebuffers, glDeleteFramebuffersEXT);
    VIGS_GL_GET_PROC(DeleteRenderbuffers, glDeleteRenderbuffersEXT);
    VIGS_GL_GET_PROC(BindFramebuffer, glBindFramebufferEXT);
    VIGS_GL_GET_PROC(BindRenderbuffer, glBindRenderbufferEXT);
    VIGS_GL_GET_PROC(RenderbufferStorage, glRenderbufferStorageEXT);
    VIGS_GL_GET_PROC(FramebufferRenderbuffer, glFramebufferRenderbufferEXT);
    VIGS_GL_GET_PROC(FramebufferTexture2D, glFramebufferTexture2DEXT);
    VIGS_GL_GET_PROC(GetIntegerv, glGetIntegerv);
    VIGS_GL_GET_PROC(GetString, glGetString);
    VIGS_GL_GET_PROC(DrawArrays, glDrawArrays);
    VIGS_GL_GET_PROC(GenBuffers, glGenBuffers);
    VIGS_GL_GET_PROC(DeleteBuffers, glDeleteBuffers);
    VIGS_GL_GET_PROC(BindBuffer, glBindBuffer);
    VIGS_GL_GET_PROC(BufferData, glBufferData);
    VIGS_GL_GET_PROC(BufferSubData, glBufferSubData);
    VIGS_GL_GET_PROC(MapBuffer, glMapBuffer);
    VIGS_GL_GET_PROC(UnmapBuffer, glUnmapBuffer);
    VIGS_GL_GET_PROC(CreateProgram, glCreateProgram);
    VIGS_GL_GET_PROC(CreateShader, glCreateShader);
    VIGS_GL_GET_PROC(CompileShader, glCompileShader);
    VIGS_GL_GET_PROC(AttachShader, glAttachShader);
    VIGS_GL_GET_PROC(LinkProgram, glLinkProgram);
    VIGS_GL_GET_PROC(GetProgramiv, glGetProgramiv);
    VIGS_GL_GET_PROC(GetProgramInfoLog, glGetProgramInfoLog);
    VIGS_GL_GET_PROC(GetShaderiv, glGetShaderiv);
    VIGS_GL_GET_PROC(GetShaderInfoLog, glGetShaderInfoLog);
    VIGS_GL_GET_PROC(DetachShader, glDetachShader);
    VIGS_GL_GET_PROC(DeleteProgram, glDeleteProgram);
    VIGS_GL_GET_PROC(DeleteShader, glDeleteShader);
    VIGS_GL_GET_PROC(DisableVertexAttribArray, glDisableVertexAttribArray);
    VIGS_GL_GET_PROC(EnableVertexAttribArray, glEnableVertexAttribArray);
    VIGS_GL_GET_PROC(ShaderSource, glShaderSource);
    VIGS_GL_GET_PROC(UseProgram, glUseProgram);
    VIGS_GL_GET_PROC(GetAttribLocation, glGetAttribLocation);
    VIGS_GL_GET_PROC(GetUniformLocation, glGetUniformLocation);
    VIGS_GL_GET_PROC(VertexAttribPointer, glVertexAttribPointer);
    VIGS_GL_GET_PROC(Uniform4fv, glUniform4fv);
    VIGS_GL_GET_PROC(UniformMatrix4fv, glUniformMatrix4fv);

    VIGS_GL_GET_PROC_OPTIONAL(MapBufferRange, glMapBufferRange);

    if (!vigs_gl_backend_cgl_check_gl_version(gl_backend_cgl,
                                              &gl_backend_cgl->base.is_gl_2)) {
        goto fail2;
    }

    if (gl_backend_cgl->base.is_gl_2) {
        error = CGLChoosePixelFormat(pixel_format_legacy_attrs,
                                     &pixel_format,
                                     &n);
    } else {
        VIGS_GL_GET_PROC(GenVertexArrays, glGenVertexArrays);
        VIGS_GL_GET_PROC(BindVertexArray, glBindVertexArray);
        VIGS_GL_GET_PROC(DeleteVertexArrays, glDeleteVertexArrays);

        error = CGLChoosePixelFormat(pixel_format_3_2_core_attrs,
                                     &pixel_format,
                                     &n);
    }

    if (error) {
        VIGS_LOG_INFO("CGLChoosePixelFormat failed for 3_2_core attrs: %s, using OpenGL 2.1", CGLErrorString(error));
        goto fail2;
    }

    if (!pixel_format) {
        VIGS_LOG_INFO("CGLChoosePixelFormat failed to find formats for 3_2_core attrs, using OpenGL 2.1");
        goto fail2;
    }

    if (!vigs_gl_backend_cgl_create_context(gl_backend_cgl,
                                            pixel_format,
                                            NULL,
                                            &gl_backend_cgl->context)) {
        goto fail3;
    }

    if (!vigs_gl_backend_cgl_create_context(gl_backend_cgl,
                                            pixel_format,
                                            gl_backend_cgl->context,
                                            &gl_backend_cgl->read_pixels_context)) {
        goto fail4;
    }

    gl_backend_cgl->base.base.destroy = &vigs_gl_backend_cgl_destroy;
    gl_backend_cgl->base.has_current = &vigs_gl_backend_cgl_has_current;
    gl_backend_cgl->base.make_current = &vigs_gl_backend_cgl_make_current;
    gl_backend_cgl->base.read_pixels_make_current = &vigs_gl_backend_cgl_read_pixels_make_current;
    gl_backend_cgl->base.ws_info.context = &gl_backend_cgl->context;

    if (!vigs_gl_backend_init(&gl_backend_cgl->base)) {
        goto fail5;
    }

    VIGS_LOG_DEBUG("created");

    CGLDestroyPixelFormat(pixel_format);

    return &gl_backend_cgl->base.base;

fail5:
    CGLDestroyContext(gl_backend_cgl->read_pixels_context);
fail4:
    CGLDestroyContext(gl_backend_cgl->context);
fail3:
    CGLDestroyPixelFormat(pixel_format);
fail2:
    dlclose(gl_backend_cgl->handle);
fail1:
    vigs_backend_cleanup(&gl_backend_cgl->base.base);

    g_free(gl_backend_cgl);

    return NULL;
}
