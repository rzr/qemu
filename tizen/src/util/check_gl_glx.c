/*
 * Copyright (c) 2011 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "check_gl.h"
#include <GL/glx.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

#ifndef GLX_VERSION_1_4
#error GL/glx.h must be equal to or greater than GLX 1.4
#endif

#define GLX_GET_PROC(func, sym) \
    do { \
        *(void**)(&func) = (void*)get_proc_address((const GLubyte*)#sym); \
        if (!func) { \
            check_gl_log(gl_error, "%s", dlerror()); \
            return 0; \
        } \
    } while (0)

struct gl_context
{
    GLXContext base;
    GLXPbuffer sfc;
};

typedef void (*PFNGLXDESTROYCONTEXTPROC)(Display *dpy, GLXContext ctx);

static void *handle;
static Display *x_dpy;
static GLXFBConfig x_config;
static PFNGLXGETPROCADDRESSPROC get_proc_address;
static PFNGLXCHOOSEFBCONFIGPROC choose_fb_config;
static PFNGLXCREATEPBUFFERPROC create_pbuffer;
static PFNGLXDESTROYPBUFFERPROC destroy_pbuffer;
static PFNGLXCREATENEWCONTEXTPROC create_context;
static PFNGLXDESTROYCONTEXTPROC destroy_context;
static PFNGLXMAKECONTEXTCURRENTPROC make_current;

/* GLX_ARB_create_context */
static PFNGLXCREATECONTEXTATTRIBSARBPROC create_context_attribs;

static int check_gl_error_handler(Display *dpy, XErrorEvent *e)
{
    return 0;
}

int check_gl_init(void)
{
    static const int config_attribs[] =
    {
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_BUFFER_SIZE, 32,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        None
    };
    int n = 0;
    GLXFBConfig *configs;

    x_dpy = XOpenDisplay(NULL);

    if (!x_dpy) {
        check_gl_log(gl_error, "Unable to open X display");
        return 0;
    }

    XSetErrorHandler(check_gl_error_handler);

    handle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        check_gl_log(gl_error, "%s", dlerror());
        goto fail;
    }

    get_proc_address = dlsym(handle, "glXGetProcAddress");

    if (!get_proc_address) {
        get_proc_address = dlsym(handle, "glXGetProcAddressARB");
    }

    if (!get_proc_address) {
        check_gl_log(gl_error, "%s", dlerror());
        goto fail;
    }

    GLX_GET_PROC(choose_fb_config, glXChooseFBConfig);
    GLX_GET_PROC(create_pbuffer, glXCreatePbuffer);
    GLX_GET_PROC(destroy_pbuffer, glXDestroyPbuffer);
    GLX_GET_PROC(create_context, glXCreateNewContext);
    GLX_GET_PROC(destroy_context, glXDestroyContext);
    GLX_GET_PROC(make_current, glXMakeContextCurrent);
    GLX_GET_PROC(create_context_attribs, glXCreateContextAttribsARB);

    configs = choose_fb_config(x_dpy,
                               DefaultScreen(x_dpy),
                               config_attribs,
                               &n);

    if (!configs || (n <= 0)) {
        check_gl_log(gl_error, "Unable to find suitable FB config");
        goto fail;
    }

    x_config = configs[0];

    XFree(configs);

    return 1;

fail:
    XCloseDisplay(x_dpy);

    return 0;
}

void check_gl_cleanup(void)
{
    XCloseDisplay(x_dpy);
}

struct gl_context *check_gl_context_create(struct gl_context *share_ctx,
                                           gl_version version)
{
    static const int ctx_attribs_3_1[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        GLX_RENDER_TYPE, GLX_RGBA_TYPE,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };
    static const int ctx_attribs_3_2[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 2,
        GLX_RENDER_TYPE, GLX_RGBA_TYPE,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };
    static const int surface_attribs[] =
    {
        GLX_PBUFFER_WIDTH, 1,
        GLX_PBUFFER_HEIGHT, 1,
        GLX_LARGEST_PBUFFER, False,
        None
    };
    GLXContext base;
    GLXPbuffer sfc;
    struct gl_context *ctx;

    switch (version) {
    case gl_2:
        base = create_context(x_dpy,
                              x_config,
                              GLX_RGBA_TYPE,
                              (share_ctx ? share_ctx->base : NULL),
                              True);
        break;
    case gl_3_1:
        base = create_context_attribs(x_dpy,
                                      x_config,
                                      (share_ctx ? share_ctx->base : NULL),
                                      True,
                                      ctx_attribs_3_1);
        break;
    case gl_3_2:
        base = create_context_attribs(x_dpy,
                                      x_config,
                                      (share_ctx ? share_ctx->base : NULL),
                                      True,
                                      ctx_attribs_3_2);
        break;
    default:
        assert(0);
        return NULL;
    }

    if (!base) {
        return NULL;
    }

    sfc = create_pbuffer(x_dpy,
                         x_config,
                         surface_attribs);

    if (!sfc) {
        destroy_context(x_dpy, base);
        return NULL;
    }

    ctx = malloc(sizeof(*ctx));

    if (!ctx) {
        destroy_pbuffer(x_dpy, sfc);
        destroy_context(x_dpy, base);
        return NULL;
    }

    ctx->base = base;
    ctx->sfc = sfc;

    return ctx;
}

int check_gl_make_current(struct gl_context *ctx)
{
    return make_current(x_dpy,
                        (ctx ? ctx->sfc : None),
                        (ctx ? ctx->sfc : None),
                        (ctx ? ctx->base : NULL));
}

void check_gl_context_destroy(struct gl_context *ctx)
{
    destroy_pbuffer(x_dpy, ctx->sfc);
    destroy_context(x_dpy, ctx->base);
    free(ctx);
}

int check_gl_procaddr(void **func, const char *sym, int opt)
{
    *func = (void*)get_proc_address((const GLubyte*)sym);

    if (!*func) {
        *func = dlsym(handle, sym);
    }

    if (!*func && !opt) {
        check_gl_log(gl_error, "Unable to find symbol \"%s\"", sym);
        return 0;
    }

    return 1;
}
