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
#include <AGL/agl.h>
#include <glib.h>
#include <dlfcn.h>

#define LIBGL_IMAGE_NAME \
"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#define VIGS_GL_GET_PROC(func, proc_name) \
    do { \
        *(void**)(&gl_backend_agl->base.func) = dlsym(gl_backend_agl->handle, #proc_name); \
        if (!gl_backend_agl->base.func) { \
            VIGS_LOG_CRITICAL("Unable to load " #proc_name " symbol"); \
            goto fail; \
        } \
    } while (0)

struct vigs_gl_backend_agl {
    struct vigs_gl_backend base;

    void *handle;
    AGLContext read_pixels_context;
    AGLPbuffer read_pixels_surface;
    AGLContext context;
    AGLPbuffer surface;
    AGLPixelFormat pixfmt;
};

static int vigs_gl_backend_agl_choose_config(struct vigs_gl_backend_agl
                                             *gl_backend_agl)
{
    const int attrib_list[] = {
        AGL_RGBA,
        AGL_ACCELERATED,
        AGL_MINIMUM_POLICY,
        AGL_BUFFER_SIZE, 32,
        AGL_RED_SIZE, 8,
        AGL_GREEN_SIZE, 8,
        AGL_BLUE_SIZE, 8,
        AGL_ALPHA_SIZE, 8,
        AGL_DEPTH_SIZE, 24,
        AGL_STENCIL_SIZE, 8,
        AGL_NO_RECOVERY,
        AGL_DOUBLEBUFFER,
        AGL_PBUFFER,
        AGL_NONE
    };
    AGLPixelFormat pixfmt;

    /* Select first AGL pixel format matching our constraints */

    pixfmt = aglChoosePixelFormat(NULL, 0, attrib_list);

    gl_backend_agl->pixfmt = pixfmt;

    return (pixfmt != NULL);
}

static bool vigs_gl_backend_agl_create_surface(struct vigs_gl_backend_agl
                                               *gl_backend_agl, int config_id,
                                               AGLPbuffer *surface)
{
    aglCreatePBuffer(1, 1, GL_TEXTURE_2D, GL_RGBA, 0,
                     surface);

    if (!*surface) {
        VIGS_LOG_CRITICAL("aglCreatePBuffer failed");
        return false;
    }

    return true;
}

static bool vigs_gl_backend_agl_create_context(struct vigs_gl_backend_agl
                                               *gl_backend_agl,
                                               AGLContext share_context,
                                               AGLContext *context)
{
    *context = aglCreateContext(gl_backend_agl->pixfmt, share_context);

    if (!*context) {
        VIGS_LOG_CRITICAL("aglCreateContext failed");
        return false;
    }

    return true;
}

static void vigs_gl_backend_agl_destroy(struct vigs_backend *backend)
{
    struct vigs_gl_backend_agl *gl_backend_agl =
        (struct vigs_gl_backend_agl *)backend;

    vigs_gl_backend_cleanup(&gl_backend_agl->base);

    if (gl_backend_agl->surface) {
        aglDestroyPBuffer(gl_backend_agl->surface);
    }

    if (gl_backend_agl->read_pixels_surface) {
        aglDestroyPBuffer(gl_backend_agl->read_pixels_surface);
    }

    if (gl_backend_agl->context) {
        aglDestroyContext(gl_backend_agl->context);
    }

    if (gl_backend_agl->read_pixels_context) {
        aglDestroyContext(gl_backend_agl->read_pixels_context);
    }

    if (gl_backend_agl->handle) {
        dlclose(gl_backend_agl->handle);
    }

    vigs_backend_cleanup(&gl_backend_agl->base.base);

    g_free(gl_backend_agl);

    VIGS_LOG_DEBUG("destroyed");
}

static bool vigs_gl_backend_agl_has_current(struct vigs_gl_backend *gl_backend)
{
    return aglGetCurrentContext() != NULL;
}

static bool vigs_gl_backend_agl_make_current(struct vigs_gl_backend *gl_backend,
                                             bool enable)
{
    struct vigs_gl_backend_agl *gl_backend_agl =
        (struct vigs_gl_backend_agl *)gl_backend;
    AGLPbuffer buf = NULL;
    AGLContext context = gl_backend_agl->context;

    if (enable) {
        buf = gl_backend_agl->surface;

        if (!buf) {
            VIGS_LOG_CRITICAL("surface retrieval failed");
            return false;
        }

        if (aglSetPBuffer(context, buf, 0, 0, 0) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetPBuffer failed");
            return false;
        }

        if (aglSetCurrentContext(context) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetCurrentContext failed");
            aglSetPBuffer(context, NULL, 0, 0, 0);
            return false;
        }
    } else {
        if (aglSetCurrentContext(NULL) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetCurrentContext(NULL) failed");
            return false;
        }
    }

    return true;
}

static bool vigs_gl_backend_agl_read_pixels_make_current(struct vigs_gl_backend *gl_backend,
                                                         bool enable)
{
    struct vigs_gl_backend_agl *gl_backend_agl =
        (struct vigs_gl_backend_agl *)gl_backend;
    AGLPbuffer buf = NULL;
    AGLContext context = gl_backend_agl->read_pixels_context;

    if (enable) {
        buf = gl_backend_agl->read_pixels_surface;

        if (aglSetPBuffer(context, buf, 0, 0, 0) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetPBuffer failed");
            return false;
        }

        if (aglSetCurrentContext(context) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetCurrentContext failed");
            aglSetPBuffer(context, NULL, 0, 0, 0);
            return false;
        }
    } else {
        if (aglSetCurrentContext(NULL) == GL_FALSE) {
            VIGS_LOG_CRITICAL("aglSetCurrentContext(NULL) failed");
            return false;
        }
    }

    return true;
}

struct vigs_backend *vigs_gl_backend_create(void *display)
{
    struct vigs_gl_backend_agl *gl_backend_agl;
    int config_id;

    gl_backend_agl = g_malloc0(sizeof(*gl_backend_agl));

    vigs_backend_init(&gl_backend_agl->base.base,
                      &gl_backend_agl->base.ws_info.base);

    gl_backend_agl->handle = dlopen(LIBGL_IMAGE_NAME, RTLD_GLOBAL);

    VIGS_GL_GET_PROC(GenTextures, glGenTextures);
    VIGS_GL_GET_PROC(DeleteTextures, glDeleteTextures);
    VIGS_GL_GET_PROC(BindTexture, glBindTexture);
    VIGS_GL_GET_PROC(Begin, glBegin);
    VIGS_GL_GET_PROC(End, glEnd);
    VIGS_GL_GET_PROC(CullFace, glCullFace);
    VIGS_GL_GET_PROC(TexParameterf, glTexParameterf);
    VIGS_GL_GET_PROC(TexParameterfv, glTexParameterfv);
    VIGS_GL_GET_PROC(TexParameteri, glTexParameteri);
    VIGS_GL_GET_PROC(TexParameteriv, glTexParameteriv);
    VIGS_GL_GET_PROC(TexImage2D, glTexImage2D);
    VIGS_GL_GET_PROC(TexSubImage2D, glTexSubImage2D);
    VIGS_GL_GET_PROC(TexEnvf, glTexEnvf);
    VIGS_GL_GET_PROC(TexEnvfv, glTexEnvfv);
    VIGS_GL_GET_PROC(TexEnvi, glTexEnvi);
    VIGS_GL_GET_PROC(TexEnviv, glTexEnviv);
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
    VIGS_GL_GET_PROC(LoadIdentity, glLoadIdentity);
    VIGS_GL_GET_PROC(MatrixMode, glMatrixMode);
    VIGS_GL_GET_PROC(Ortho, glOrtho);
    VIGS_GL_GET_PROC(EnableClientState, glEnableClientState);
    VIGS_GL_GET_PROC(DisableClientState, glDisableClientState);
    VIGS_GL_GET_PROC(Color4f, glColor4f);
    VIGS_GL_GET_PROC(TexCoordPointer, glTexCoordPointer);
    VIGS_GL_GET_PROC(VertexPointer, glVertexPointer);
    VIGS_GL_GET_PROC(DrawArrays, glDrawArrays);
    VIGS_GL_GET_PROC(Color4ub, glColor4ub);
    VIGS_GL_GET_PROC(WindowPos2f, glWindowPos2f);
    VIGS_GL_GET_PROC(PixelZoom, glPixelZoom);
    VIGS_GL_GET_PROC(DrawPixels, glDrawPixels);
    VIGS_GL_GET_PROC(BlendFunc, glBlendFunc);
    VIGS_GL_GET_PROC(CopyTexImage2D, glCopyTexImage2D);
    VIGS_GL_GET_PROC(BlitFramebuffer, glBlitFramebufferEXT);
    VIGS_GL_GET_PROC(GenBuffers, glGenBuffers);
    VIGS_GL_GET_PROC(DeleteBuffers, glDeleteBuffers);
    VIGS_GL_GET_PROC(BindBuffer, glBindBuffer);
    VIGS_GL_GET_PROC(BufferData, glBufferData);
    VIGS_GL_GET_PROC(MapBuffer, glMapBuffer);
    VIGS_GL_GET_PROC(UnmapBuffer, glUnmapBuffer);

    config_id = vigs_gl_backend_agl_choose_config(gl_backend_agl);

    if (!config_id) {
        goto fail;
    }

    if (!vigs_gl_backend_agl_create_surface(gl_backend_agl, config_id,
                                            &gl_backend_agl->surface)) {
        goto fail;
    }

    if (!vigs_gl_backend_agl_create_surface(gl_backend_agl, config_id,
                                            &gl_backend_agl->read_pixels_surface)) {
        goto fail;
    }

    if (!vigs_gl_backend_agl_create_context(gl_backend_agl, NULL,
                                            &gl_backend_agl->context)) {
        goto fail;
    }

    if (!vigs_gl_backend_agl_create_context(gl_backend_agl, gl_backend_agl->context,
                                            &gl_backend_agl->read_pixels_context)) {
        goto fail;
    }

    gl_backend_agl->base.base.destroy = &vigs_gl_backend_agl_destroy;
    gl_backend_agl->base.has_current = &vigs_gl_backend_agl_has_current;
    gl_backend_agl->base.make_current = &vigs_gl_backend_agl_make_current;
    gl_backend_agl->base.read_pixels_make_current = &vigs_gl_backend_agl_read_pixels_make_current;
    gl_backend_agl->base.ws_info.context = &gl_backend_agl->context;

    if (!vigs_gl_backend_init(&gl_backend_agl->base)) {
        goto fail;
    }

    VIGS_LOG_DEBUG("created");

    return &gl_backend_agl->base.base;

 fail:

    if (gl_backend_agl->handle) {
        dlclose(gl_backend_agl->handle);
    }

    if (gl_backend_agl->surface) {
        aglDestroyPBuffer(gl_backend_agl->surface);
    }

    if (gl_backend_agl->read_pixels_surface) {
        aglDestroyPBuffer(gl_backend_agl->read_pixels_surface);
    }

    if (gl_backend_agl->context) {
        aglDestroyContext(gl_backend_agl->context);
    }

    if (gl_backend_agl->read_pixels_context) {
        aglDestroyContext(gl_backend_agl->read_pixels_context);
    }

    vigs_backend_cleanup(&gl_backend_agl->base.base);

    g_free(gl_backend_agl);

    return NULL;
}
