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
#include <OpenGL/OpenGL.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>

#define LIBGL_IMAGE_NAME \
"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#ifndef kCGLPFAOpenGLProfile
#   define kCGLPFAOpenGLProfile 99
#   define kCGLOGLPVersion_3_2_Core 0x3200
#   define kCGLOGLPVersion_Legacy 0x1000
#endif

struct gl_context
{
    CGLContextObj base;
};

static void *handle;

int check_gl_init(void)
{
    handle = dlopen(LIBGL_IMAGE_NAME, RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        check_gl_log(gl_error, "%s", dlerror());
        return 0;
    }

    return 1;
}

struct gl_context *check_gl_context_create(struct gl_context *share_ctx,
                                           gl_version version)
{
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
    CGLError error;
    CGLPixelFormatObj pixel_format;
    int n;
    CGLContextObj base = NULL;
    struct gl_context *ctx;

    switch (version) {
    case gl_2:
        error = CGLChoosePixelFormat(pixel_format_legacy_attrs,
                                     &pixel_format,
                                     &n);

        if (error || !pixel_format) {
            break;
        }

        error = CGLCreateContext(pixel_format,
                                 (share_ctx ? share_ctx->base : NULL),
                                 &base);

        CGLDestroyPixelFormat(pixel_format);

        if (error) {
            base = NULL;
        }

        break;
    case gl_3_1:
        break;
    case gl_3_2:
        error = CGLChoosePixelFormat(pixel_format_3_2_core_attrs,
                                     &pixel_format,
                                     &n);

        if (error || !pixel_format) {
            break;
        }

        error = CGLCreateContext(pixel_format,
                                 (share_ctx ? share_ctx->base : NULL),
                                 &base);

        CGLDestroyPixelFormat(pixel_format);

        if (error) {
            base = NULL;
        }

        break;
    default:
        assert(0);
        return NULL;
    }

    if (!base) {
        return NULL;
    }

    ctx = malloc(sizeof(*ctx));

    if (!ctx) {
        CGLDestroyContext(base);
        return NULL;
    }

    ctx->base = base;

    return ctx;
}

int check_gl_make_current(struct gl_context *ctx)
{
    return !CGLSetCurrentContext(ctx ? ctx->base : NULL);
}

void check_gl_context_destroy(struct gl_context *ctx)
{
    CGLDestroyContext(ctx->base);
    free(ctx);
}

int check_gl_procaddr(void **func, const char *sym, int opt)
{
    *func = dlsym(handle, sym);

    if (!*func && !opt) {
        check_gl_log(gl_error, "Unable to find symbol \"%s\"", sym);
        return 0;
    }

    return 1;
}
