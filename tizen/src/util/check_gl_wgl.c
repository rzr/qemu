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
#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include <assert.h>
#include <stdlib.h>

#define WGL_GET_PROC(func, sym) \
    do { \
        *(void**)(&func) = (void*)GetProcAddress(handle, #sym); \
        if (!func) { \
            check_gl_log(gl_error, "Unable to load %s symbol", #sym); \
            goto fail; \
        } \
    } while (0)

#define WGL_GET_EXT_PROC(func, ext, sym) \
    do { \
        if ((strstr(ext_str, #ext " ") == NULL)) { \
            check_gl_log(gl_error, "Extension %s not supported", #ext); \
            goto fail; \
        } \
        *(void**)(&func) = (void*)get_proc_address((LPCSTR)#sym); \
        if (!func) { \
            check_gl_log(gl_error, "Unable to load %s symbol", #sym); \
            goto fail; \
        } \
    } while (0)

struct gl_context
{
    HGLRC base;
    HPBUFFERARB sfc;
    HDC sfc_dc;
};

typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTPROC)(HDC hdl);
typedef BOOL (WINAPI *PFNWGLDELETECONTEXTPROC)(HGLRC hdl);
typedef PROC (WINAPI *PFNWGLGETPROCADDRESSPROC)(LPCSTR sym);
typedef BOOL (WINAPI *PFNWGLMAKECURRENTPROC)(HDC dev_ctx, HGLRC rend_ctx);
typedef BOOL (WINAPI *PFNWGLSHARELISTSPROC)(HGLRC ctx1, HGLRC ctx2);

static HINSTANCE handle = NULL;
static HWND init_win = NULL;
static HDC init_dc = NULL;
static HGLRC init_ctx = NULL;
static HWND win = NULL;
static HDC dc = NULL;
static int config_id = 0;
static struct gl_context *current = NULL;
static PFNWGLCREATECONTEXTPROC create_context;
static PFNWGLDELETECONTEXTPROC delete_context;
static PFNWGLGETPROCADDRESSPROC get_proc_address;
static PFNWGLMAKECURRENTPROC make_current;
static PFNWGLSHARELISTSPROC share_lists;

/* WGL extensions */
static PFNWGLGETEXTENSIONSSTRINGEXTPROC get_extensions_string_ext;
static PFNWGLGETEXTENSIONSSTRINGARBPROC get_extensions_string_arb;
static PFNWGLCHOOSEPIXELFORMATARBPROC choose_pixel_format;
static PFNWGLCREATEPBUFFERARBPROC create_pbuffer;
static PFNWGLGETPBUFFERDCARBPROC get_pbuffer_dc;
static PFNWGLRELEASEPBUFFERDCARBPROC release_pbuffer_dc;
static PFNWGLDESTROYPBUFFERARBPROC destroy_pbuffer;

/* WGL_ARB_create_context */
static PFNWGLCREATECONTEXTATTRIBSARBPROC create_context_attribs;

int check_gl_init(void)
{
    WNDCLASSEXA win_class;
    PIXELFORMATDESCRIPTOR init_pixfmt =
    {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
    };
    int init_config_id = 0;
    const char *ext_str = NULL;
    const int config_attribs[] =
    {
        WGL_SUPPORT_OPENGL_ARB, TRUE,
        WGL_DOUBLE_BUFFER_ARB, TRUE,
        WGL_DRAW_TO_PBUFFER_ARB, TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_RED_BITS_ARB, 8,
        WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0,
    };
    UINT n = 0;
    PIXELFORMATDESCRIPTOR pixfmt;

    win_class.cbSize = sizeof(WNDCLASSEXA);
    win_class.style = 0;
    win_class.lpfnWndProc = &DefWindowProcA;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = NULL;
    win_class.hIcon = NULL;
    win_class.hCursor = NULL;
    win_class.hbrBackground = NULL;
    win_class.lpszMenuName =  NULL;
    win_class.lpszClassName = "CheckGLWinClass";
    win_class.hIconSm = NULL;

    if (!RegisterClassExA(&win_class)) {
        check_gl_log(gl_error, "Unable to register window class");
        return 0;
    }

    handle = LoadLibraryA("opengl32");

    if (!handle) {
        check_gl_log(gl_error, "Unable to load opengl32.dll");
        goto fail;
    }

    WGL_GET_PROC(create_context, wglCreateContext);
    WGL_GET_PROC(delete_context, wglDeleteContext);
    WGL_GET_PROC(get_proc_address, wglGetProcAddress);
    WGL_GET_PROC(make_current, wglMakeCurrent);
    WGL_GET_PROC(share_lists, wglShareLists);

    init_win = CreateWindow("CheckGLWinClass", "CheckGLWin",
                            WS_DISABLED | WS_POPUP,
                            0, 0, 1, 1, NULL, NULL, 0, 0);

    if (!init_win) {
        check_gl_log(gl_error, "Unable to create window");
        goto fail;
    }

    init_dc = GetDC(init_win);

    if (!init_dc) {
        check_gl_log(gl_error, "Unable to get window DC");
        goto fail;
    }

    init_config_id = ChoosePixelFormat(init_dc, &init_pixfmt);

    if (!init_config_id) {
        check_gl_log(gl_error, "ChoosePixelFormat failed");
        goto fail;
    }

    if (!SetPixelFormat(init_dc, init_config_id, &init_pixfmt)) {
        check_gl_log(gl_error, "SetPixelFormat failed");
        goto fail;
    }

    init_ctx = create_context(init_dc);
    if (!init_ctx) {
        check_gl_log(gl_error, "wglCreateContext failed");
        goto fail;
    }

    if (!make_current(init_dc, init_ctx)) {
        check_gl_log(gl_error, "wglMakeCurrent failed");
        goto fail;
    }

    /*
     * WGL extensions couldn't be queried by glGetString(), we need to use
     * wglGetExtensionsStringARB or wglGetExtensionsStringEXT for this, which
     * themselves are extensions
     */
    get_extensions_string_arb = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
        get_proc_address((LPCSTR)"wglGetExtensionsStringARB");
    get_extensions_string_ext = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
        get_proc_address((LPCSTR)"wglGetExtensionsStringEXT");

    if (get_extensions_string_arb) {
        ext_str = get_extensions_string_arb(init_dc);
    } else if (get_extensions_string_ext) {
        ext_str = get_extensions_string_ext();
    }

    if (!ext_str) {
        check_gl_log(gl_error, "Unable to obtain WGL extension string");
        goto fail;
    }

    WGL_GET_EXT_PROC(create_pbuffer, WGL_ARB_pbuffer, wglCreatePbufferARB);
    WGL_GET_EXT_PROC(get_pbuffer_dc, WGL_ARB_pbuffer, wglGetPbufferDCARB);
    WGL_GET_EXT_PROC(release_pbuffer_dc, WGL_ARB_pbuffer, wglReleasePbufferDCARB);
    WGL_GET_EXT_PROC(destroy_pbuffer, WGL_ARB_pbuffer, wglDestroyPbufferARB);
    WGL_GET_EXT_PROC(choose_pixel_format, WGL_ARB_pixel_format, wglChoosePixelFormatARB);
    WGL_GET_EXT_PROC(create_context_attribs, WGL_ARB_create_context, wglCreateContextAttribsARB);

    make_current(NULL, NULL);

    win = CreateWindow("CheckGLWinClass", "CheckGLWin2",
                       WS_DISABLED | WS_POPUP,
                       0, 0, 1, 1, NULL, NULL, 0, 0);

    if (!win) {
        check_gl_log(gl_error, "Unable to create window");
        goto fail;
    }

    dc = GetDC(win);

    if (!dc) {
        check_gl_log(gl_error, "Unable to get window DC");
        goto fail;
    }

    if (!choose_pixel_format(dc,
                             config_attribs,
                             NULL,
                             1,
                             &config_id,
                             &n) || (n == 0)) {
        check_gl_log(gl_error, "wglChoosePixelFormat failed");
        goto fail;
    }

    if (!DescribePixelFormat(dc,
                             config_id,
                             sizeof(PIXELFORMATDESCRIPTOR),
                             &pixfmt)) {
        check_gl_log(gl_error, "DescribePixelFormat failed");
        goto fail;
    }

    if (!SetPixelFormat(dc,
                        config_id,
                        &pixfmt)) {
        check_gl_log(gl_error, "SetPixelFormat failed");
        goto fail;
    }

    return 1;

fail:
    if (dc) {
        ReleaseDC(win, dc);
    }
    if (win) {
        DestroyWindow(win);
    }
    if (init_ctx) {
        make_current(NULL, NULL);
        delete_context(init_ctx);
    }
    if (init_dc) {
        ReleaseDC(init_win, init_dc);
    }
    if (init_win) {
        DestroyWindow(init_win);
    }
    if (handle) {
        FreeLibrary(handle);
    }

    UnregisterClassA((LPCTSTR)"CheckGLWinClass", NULL);

    return 0;
}

void check_gl_cleanup(void)
{
    ReleaseDC(win, dc);
    DestroyWindow(win);
    delete_context(init_ctx);
    ReleaseDC(init_win, init_dc);
    DestroyWindow(init_win);

    UnregisterClassA((LPCTSTR)"CheckGLWinClass", NULL);
}

struct gl_context *check_gl_context_create(struct gl_context *share_ctx,
                                           gl_version version)
{
    static const int ctx_attribs_3_1[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    static const int ctx_attribs_3_2[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    static const int surface_attribs[] =
    {
        WGL_PBUFFER_LARGEST_ARB, FALSE,
        WGL_TEXTURE_TARGET_ARB, WGL_NO_TEXTURE_ARB,
        WGL_TEXTURE_FORMAT_ARB, WGL_NO_TEXTURE_ARB,
        0
    };
    HGLRC base;
    HPBUFFERARB sfc;
    HDC sfc_dc;
    struct gl_context *ctx;

    switch (version) {
    case gl_2:
        base = create_context(dc);
        if (share_ctx && !share_lists(share_ctx->base, base)) {
            delete_context(base);
            base = NULL;
        }
        break;
    case gl_3_1:
        base = create_context_attribs(dc,
                                      (share_ctx ? share_ctx->base : NULL),
                                      ctx_attribs_3_1);
        break;
    case gl_3_2:
        base = create_context_attribs(dc,
                                      (share_ctx ? share_ctx->base : NULL),
                                      ctx_attribs_3_2);
        break;
    default:
        assert(0);
        return NULL;
    }

    if (!base) {
        return NULL;
    }

    sfc = create_pbuffer(dc,
                         config_id, 1, 1,
                         surface_attribs);

    if (!sfc) {
        delete_context(base);
        return NULL;
    }

    sfc_dc = get_pbuffer_dc(sfc);

    if (!sfc_dc) {
        destroy_pbuffer(sfc);
        delete_context(base);
        return NULL;
    }

    ctx = malloc(sizeof(*ctx));

    if (!ctx) {
        release_pbuffer_dc(sfc, sfc_dc);
        destroy_pbuffer(sfc);
        delete_context(base);
        return NULL;
    }

    ctx->base = base;
    ctx->sfc = sfc;
    ctx->sfc_dc = sfc_dc;

    return ctx;
}

int check_gl_make_current(struct gl_context *ctx)
{
    current = ctx;
    return make_current((ctx ? ctx->sfc_dc : NULL),
                        (ctx ? ctx->base : NULL));
}

void check_gl_context_destroy(struct gl_context *ctx)
{
    release_pbuffer_dc(ctx->sfc, ctx->sfc_dc);
    destroy_pbuffer(ctx->sfc);
    delete_context(ctx->base);
    free(ctx);
}

int check_gl_procaddr(void **func, const char *sym, int opt)
{
    if (!make_current(init_dc, init_ctx)) {
        return 0;
    }

    *func = (void*)get_proc_address((LPCSTR)sym);

    if (!*func) {
        *func = GetProcAddress(handle, sym);
    }

    make_current((current ? current->sfc_dc : NULL),
                 (current ? current->base : NULL));

    if (!*func && !opt) {
        check_gl_log(gl_error, "Unable to find symbol \"%s\"", sym);
        return 0;
    }

    return 1;
}
