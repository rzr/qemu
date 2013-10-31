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

#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include <glib.h>
#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_egl_native_config.h"
#include "yagl_egl_surface_attribs.h"
#include "yagl_process.h"
#include "yagl_tls.h"
#include "yagl_thread.h"

#define YAGL_EGL_WGL_WIN_CLASS                "YaGLwinClass"

#define YAGL_EGL_WGL_ENTER(func, format, ...) \
    YAGL_LOG_FUNC_ENTER(func, format,##__VA_ARGS__)

/* Get a core WGL function address */
#define YAGL_EGL_WGL_GET_PROC(proc_type, proc_name) \
    do {                                                                      \
        egl_wgl->proc_name =                                                  \
            (proc_type)yagl_dyn_lib_get_sym(egl_driver->dyn_lib, #proc_name); \
        if (!egl_wgl->proc_name) {                                            \
            YAGL_LOG_ERROR("%s not found!", #proc_name);                      \
            goto fail;                                                        \
        }                                                                     \
        YAGL_LOG_TRACE("Got %s address", #proc_name);                         \
    } while (0)

/* Get a mandatory WGL extension function pointer */
#define YAGL_EGL_WGL_GET_EXT_PROC(ext_name, proc_name, proc_type) \
    do {                                                                      \
        if (!yagl_egl_wgl_search_ext(ext_str, #ext_name)) {                   \
            YAGL_LOG_ERROR("%s extension not supported", #ext_name);          \
            goto out;                                                         \
        }                                                                     \
        egl_wgl->proc_name = (proc_type)                                      \
                        egl_wgl->wglGetProcAddress((LPCSTR)#proc_name);       \
        if (!egl_wgl->proc_name) {                                            \
            YAGL_LOG_ERROR("Mandatory function %s not found", #proc_name);    \
            goto out;                                                         \
        }                                                                     \
        YAGL_LOG_TRACE("Got %s address", #proc_name);                         \
    } while (0)

typedef HGLRC (WINAPI *WGLCREATECONTEXTPROC)(HDC hdl);
typedef BOOL (WINAPI *WGLDELETECONTEXTPROC)(HGLRC hdl);
typedef PROC (WINAPI *WGLGETPROCADDRESSPROC)(LPCSTR sym);
typedef BOOL (WINAPI *WGLMAKECURRENTPROC)(HDC dev_ctx, HGLRC rend_ctx);
typedef BOOL (WINAPI *WGLSHARELISTSPROC)(HGLRC ctx1, HGLRC ctx2);

typedef struct YaglEglWglDC {
    HWND win;
    HDC dc;
} YaglEglWglDC;

typedef struct YaglEglWglDpy {
    GHashTable *dc_table;
} YaglEglWglDpy;

typedef struct YaglEglWglDriver {
    struct yagl_egl_driver base;

    HWND win;

    WGLCREATECONTEXTPROC wglCreateContext;
    WGLDELETECONTEXTPROC wglDeleteContext;
    WGLGETPROCADDRESSPROC wglGetProcAddress;
    WGLMAKECURRENTPROC wglMakeCurrent;
    WGLSHARELISTSPROC wglShareLists;

    /* WGL extensions */
    PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
    PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
    PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
    PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
    PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
    PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
} YaglEglWglDriver;

static inline HWND yagl_egl_wgl_dummy_win_create(void)
{
    return CreateWindow(YAGL_EGL_WGL_WIN_CLASS, "YaGLwin",
        WS_DISABLED | WS_POPUP, 0, 0, 1, 1, NULL, NULL, 0, 0);
}

static inline bool yagl_egl_wgl_dc_set_def_pixfmt(HDC dc)
{
    INT pixfmt_idx;
    PIXELFORMATDESCRIPTOR pixfmt = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
    };

    pixfmt_idx = ChoosePixelFormat(dc, &pixfmt);

    if (pixfmt_idx == 0) {
        return false;
    }

    if (SetPixelFormat(dc, pixfmt_idx, &pixfmt) == FALSE) {
        return false;
    }

    return true;
}

static void yagl_egl_wgl_cfgdc_free(gpointer data)
{
    YaglEglWglDC *dc = data;

    ReleaseDC(dc->win, dc->dc);
    DestroyWindow(dc->win);

    g_free(dc);
}

static EGLNativeDisplayType
yagl_egl_wgl_display_open(struct yagl_egl_driver *driver)
{
    YaglEglWglDpy *dpy;

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_display_open, NULL);

    dpy = g_try_new0(YaglEglWglDpy, 1);

    if (!dpy) {
        goto fail;
    }

    dpy->dc_table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                          NULL,
                                          &yagl_egl_wgl_cfgdc_free);

    YAGL_LOG_FUNC_EXIT("Display created: %p", dpy);

    return (EGLNativeDisplayType)dpy;

fail:
    YAGL_LOG_FUNC_EXIT("Display creation failed");

    return NULL;
}

static void yagl_egl_wgl_display_close(struct yagl_egl_driver *driver,
                                       EGLNativeDisplayType egl_dpy)
{
    YaglEglWglDpy *dpy = (YaglEglWglDpy *)egl_dpy;

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_display_close, "%p", dpy);

    g_hash_table_destroy(dpy->dc_table);

    g_free(dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_egl_wgl_config_fill(YaglEglWglDriver *egl_wgl,
                                     HDC dc,
                                     struct yagl_egl_native_config *cfg,
                                     int fmt_idx)
{
    PIXELFORMATDESCRIPTOR *pix_fmt = NULL;
    bool filled = false;

    enum {
        YAGL_WGL_PIXEL_TYPE = 0,
        YAGL_WGL_DRAW_TO_PBUFFER,
        YAGL_WGL_DOUBLE_BUFFER,
        YAGL_WGL_SUPPORT_OPENGL,
        YAGL_WGL_ACCELERATION,
        YAGL_WGL_NEED_PALETTE,
        YAGL_WGL_TRANSPARENT,
        YAGL_WGL_TRANSPARENT_RED_VALUE,
        YAGL_WGL_TRANSPARENT_GREEN_VALUE,
        YAGL_WGL_TRANSPARENT_BLUE_VALUE,
        YAGL_WGL_COLOR_BITS,
        YAGL_WGL_RED_BITS,
        YAGL_WGL_GREEN_BITS,
        YAGL_WGL_BLUE_BITS,
        YAGL_WGL_ALPHA_BITS,
        YAGL_WGL_DEPTH_BITS,
        YAGL_WGL_STENCIL_BITS,
        YAGL_WGL_MAX_PBUFFER_PIXELS,
        YAGL_WGL_MAX_PBUFFER_WIDTH,
        YAGL_WGL_MAX_PBUFFER_HEIGHT,
        YAGL_WGL_NUMBER_OVERLAYS,
        YAGL_WGL_NUMBER_UNDERLAYS,
        YAGL_WGL_NUM_OF_QUERY_ATTRIBS,
    };

    const int query_list[YAGL_WGL_NUM_OF_QUERY_ATTRIBS] = {
        [YAGL_WGL_PIXEL_TYPE] = WGL_PIXEL_TYPE_ARB,
        [YAGL_WGL_DRAW_TO_PBUFFER] = WGL_DRAW_TO_PBUFFER_ARB,
        [YAGL_WGL_DOUBLE_BUFFER] = WGL_DOUBLE_BUFFER_ARB,
        [YAGL_WGL_SUPPORT_OPENGL] = WGL_SUPPORT_OPENGL_ARB,
        [YAGL_WGL_ACCELERATION] = WGL_ACCELERATION_ARB,
        [YAGL_WGL_NEED_PALETTE] = WGL_NEED_PALETTE_ARB,
        [YAGL_WGL_TRANSPARENT] = WGL_TRANSPARENT_ARB,
        [YAGL_WGL_TRANSPARENT_RED_VALUE] = WGL_TRANSPARENT_RED_VALUE_ARB,
        [YAGL_WGL_TRANSPARENT_GREEN_VALUE] = WGL_TRANSPARENT_GREEN_VALUE_ARB,
        [YAGL_WGL_TRANSPARENT_BLUE_VALUE] = WGL_TRANSPARENT_BLUE_VALUE_ARB,
        [YAGL_WGL_COLOR_BITS] = WGL_COLOR_BITS_ARB,
        [YAGL_WGL_RED_BITS] = WGL_RED_BITS_ARB,
        [YAGL_WGL_GREEN_BITS] = WGL_GREEN_BITS_ARB,
        [YAGL_WGL_BLUE_BITS] = WGL_BLUE_BITS_ARB,
        [YAGL_WGL_ALPHA_BITS] = WGL_ALPHA_BITS_ARB,
        [YAGL_WGL_DEPTH_BITS] = WGL_DEPTH_BITS_ARB,
        [YAGL_WGL_STENCIL_BITS] = WGL_STENCIL_BITS_ARB,
        [YAGL_WGL_MAX_PBUFFER_PIXELS] = WGL_MAX_PBUFFER_PIXELS_ARB,
        [YAGL_WGL_MAX_PBUFFER_WIDTH] = WGL_MAX_PBUFFER_WIDTH_ARB,
        [YAGL_WGL_MAX_PBUFFER_HEIGHT] = WGL_MAX_PBUFFER_HEIGHT_ARB,
        [YAGL_WGL_NUMBER_OVERLAYS] = WGL_NUMBER_OVERLAYS_ARB,
        [YAGL_WGL_NUMBER_UNDERLAYS] = WGL_NUMBER_UNDERLAYS_ARB,
    };

    int attr_vals[YAGL_WGL_NUM_OF_QUERY_ATTRIBS];

    if (egl_wgl->wglGetPixelFormatAttribivARB(dc, fmt_idx, 0,
                                              YAGL_WGL_NUM_OF_QUERY_ATTRIBS,
                                              query_list, attr_vals) == FALSE) {
        GetLastError(); /* Clear last error */
    	goto out;
    }

    if (attr_vals[YAGL_WGL_PIXEL_TYPE] != WGL_TYPE_RGBA_ARB ||
        attr_vals[YAGL_WGL_DRAW_TO_PBUFFER] != TRUE ||
        attr_vals[YAGL_WGL_DOUBLE_BUFFER] != TRUE ||
        attr_vals[YAGL_WGL_SUPPORT_OPENGL] != TRUE ||
        attr_vals[YAGL_WGL_ACCELERATION] != WGL_FULL_ACCELERATION_ARB ||
        attr_vals[YAGL_WGL_NEED_PALETTE] != FALSE) {
        goto out;
    }

    /* This structure is used as a config driver data to pass it to
     * ChoosePixelFormat later */
    pix_fmt = g_try_new0(PIXELFORMATDESCRIPTOR, 1);
    if (!pix_fmt) {
        goto out;
    }

    if (!DescribePixelFormat(dc, fmt_idx,
                             sizeof(PIXELFORMATDESCRIPTOR),
                             pix_fmt)) {
        GetLastError(); /* Clear last error */
        g_free(pix_fmt);
        goto out;
    }

    yagl_egl_native_config_init(cfg);

    if (attr_vals[YAGL_WGL_TRANSPARENT] == TRUE) {
        cfg->transparent_type = EGL_TRANSPARENT_RGB;
        cfg->trans_red_val = attr_vals[YAGL_WGL_TRANSPARENT_RED_VALUE];
        cfg->trans_green_val = attr_vals[YAGL_WGL_TRANSPARENT_GREEN_VALUE];
        cfg->trans_blue_val = attr_vals[YAGL_WGL_TRANSPARENT_BLUE_VALUE];
    } else {
        cfg->transparent_type = EGL_NONE;
    }

    cfg->config_id = fmt_idx;
    cfg->buffer_size = attr_vals[YAGL_WGL_COLOR_BITS];
    cfg->red_size = attr_vals[YAGL_WGL_RED_BITS];
    cfg->green_size = attr_vals[YAGL_WGL_GREEN_BITS];
    cfg->blue_size = attr_vals[YAGL_WGL_BLUE_BITS];
    cfg->alpha_size = attr_vals[YAGL_WGL_ALPHA_BITS];
    cfg->depth_size = attr_vals[YAGL_WGL_DEPTH_BITS];
    cfg->stencil_size = attr_vals[YAGL_WGL_STENCIL_BITS];
    cfg->max_pbuffer_width = attr_vals[YAGL_WGL_MAX_PBUFFER_WIDTH];
    cfg->max_pbuffer_height = attr_vals[YAGL_WGL_MAX_PBUFFER_HEIGHT];
    cfg->max_pbuffer_size = attr_vals[YAGL_WGL_MAX_PBUFFER_PIXELS];
    cfg->native_visual_type = EGL_NONE;
    cfg->native_visual_id = 0;
    cfg->caveat = EGL_NONE;
    cfg->frame_buffer_level = 0;
    cfg->samples_per_pixel = 0;
    cfg->max_swap_interval = 1000;
    cfg->min_swap_interval = 0;
    cfg->driver_data = pix_fmt;

    filled = true;

out:
    return filled;
}

static struct yagl_egl_native_config
    *yagl_egl_wgl_config_enum(struct yagl_egl_driver *driver,
                              EGLNativeDisplayType egl_dpy,
                              int *num_configs)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);
    YaglEglWglDpy *dpy = (YaglEglWglDpy *)egl_dpy;
    HDC dc;
    struct yagl_egl_native_config *egl_configs = NULL;
    UINT pixfmt_cnt = 0;
    int pixfmt_cnt_max = 0, curr_fmt_idx, cfg_index = 0;
    int *pixfmt_arr;
    const int query_num_of_formats = WGL_NUMBER_PIXEL_FORMATS_ARB;
    const int attrib_list[] = {
        WGL_SUPPORT_OPENGL_ARB, TRUE,
        WGL_DOUBLE_BUFFER_ARB, TRUE,
        WGL_DRAW_TO_PBUFFER_ARB, TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        0,
    };

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_config_enum, "display %p", dpy);

    *num_configs = 0;

    dc = GetDC(egl_wgl->win);
    if (!dc) {
        goto out;
    }

    /* Query number of pixel formats */
    if (egl_wgl->wglGetPixelFormatAttribivARB(dc, 0, 0, 1,
                                              &query_num_of_formats,
                                              &pixfmt_cnt_max) == FALSE ||
        !pixfmt_cnt_max) {
    	YAGL_LOG_ERROR_WIN();
        pixfmt_cnt_max = DescribePixelFormat(dc, 1, 0, NULL);
    }

    if (pixfmt_cnt_max == 0) {
    	YAGL_LOG_ERROR_WIN();
        YAGL_LOG_ERROR("No pixel formats found");
        goto out;
    }

    pixfmt_arr = g_try_new0(int, pixfmt_cnt_max);
    if (!pixfmt_arr) {
        goto out;
    }

    if (egl_wgl->wglChoosePixelFormatARB(dc, attrib_list, NULL,
                                         pixfmt_cnt_max,
                                         pixfmt_arr,
                                         &pixfmt_cnt) == FALSE) {
    	YAGL_LOG_ERROR_WIN();

        pixfmt_cnt = pixfmt_cnt_max;

        for (curr_fmt_idx = 0; curr_fmt_idx < pixfmt_cnt; ++curr_fmt_idx) {
            pixfmt_arr[curr_fmt_idx] = curr_fmt_idx + 1;
        }
    }

    YAGL_LOG_DEBUG("got %d pixel formats", pixfmt_cnt);

    egl_configs = g_try_new0(struct yagl_egl_native_config, pixfmt_cnt);
    if (!egl_configs) {
        g_free(pixfmt_arr);
        goto out;
    }

    for (curr_fmt_idx = 0; curr_fmt_idx < pixfmt_cnt; ++curr_fmt_idx) {
        if (yagl_egl_wgl_config_fill(egl_wgl, dc,
                                     &egl_configs[cfg_index],
                                     pixfmt_arr[curr_fmt_idx])) {
            YAGL_LOG_TRACE("Added config with pixfmt=%d",
                pixfmt_arr[curr_fmt_idx]);
            ++cfg_index;
        }
    }

    if (cfg_index < pixfmt_cnt) {
        egl_configs = g_renew(struct yagl_egl_native_config,
        		              egl_configs,
        		              cfg_index);
    }

    *num_configs = cfg_index;

    g_free(pixfmt_arr);

out:
    if (dc) {
        ReleaseDC(egl_wgl->win, dc);
    }

    YAGL_LOG_INFO("WGL returned %d configs, %d are usable",
                   pixfmt_cnt, cfg_index);

    YAGL_LOG_FUNC_EXIT("Enumerated %d configs", cfg_index);

    return egl_configs;
}

static void yagl_egl_wgl_config_cleanup(struct yagl_egl_driver *driver,
                                        EGLNativeDisplayType dpy,
                                        const struct yagl_egl_native_config *cfg)
{
    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_config_cleanup,
                       "dpy = %p, cfg = %d",
                       dpy, cfg->config_id);

    g_free(cfg->driver_data);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static HDC yagl_egl_wgl_get_dc_by_cfgid(YaglEglWglDpy *dpy,
                                        const struct yagl_egl_native_config *cfg)
{
    YaglEglWglDC *dc = NULL;

    dc = g_hash_table_lookup(dpy->dc_table, GINT_TO_POINTER(cfg->config_id));

    if (dc) {
        return dc->dc;
    }

    dc = g_malloc0(sizeof(*dc));

    dc->win = yagl_egl_wgl_dummy_win_create();

    if (!dc->win) {
        goto fail;
    }

    dc->dc = GetDC(dc->win);
    if (!dc->dc) {
        goto fail;
    }

    if (!SetPixelFormat(dc->dc, cfg->config_id,
                        (PIXELFORMATDESCRIPTOR *)cfg->driver_data)) {
        goto fail;
    }

    g_hash_table_insert(dpy->dc_table,
                        GINT_TO_POINTER(cfg->config_id),
                        dc);

    return dc->dc;

fail:
    if (dc) {
        if (dc->dc) {
            ReleaseDC(dc->win, dc->dc);
        }
        if (dc->win) {
            DestroyWindow(dc->win);
        }
        g_free(dc);
    }

    return NULL;
}

static EGLContext yagl_egl_wgl_context_create(struct yagl_egl_driver *driver,
                                              EGLNativeDisplayType egl_dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              EGLContext share_context)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);
    YaglEglWglDpy *dpy = (YaglEglWglDpy *)egl_dpy;
    HGLRC egl_wgl_ctx;
    HDC dc;

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_context_create,
                       "dpy = %p, share_context = %p, cfgid=%d",
                       dpy, share_context, cfg->config_id);

    dc = yagl_egl_wgl_get_dc_by_cfgid(dpy, cfg);
    if (!dc) {
        YAGL_LOG_ERROR("failed to get DC with cfgid=%d", cfg->config_id);
        goto fail;
    }

    egl_wgl_ctx = egl_wgl->wglCreateContext(dc);
    if (!egl_wgl_ctx) {
        goto fail;
    }

    if (share_context != EGL_NO_CONTEXT) {
        if(!egl_wgl->wglShareLists((HGLRC)share_context,
                                   egl_wgl_ctx)) {
            egl_wgl->wglDeleteContext(egl_wgl_ctx);
            goto fail;
        }
    }

    YAGL_LOG_FUNC_EXIT("Context created: %p", egl_wgl_ctx);

    return (EGLContext)egl_wgl_ctx;

fail:
    YAGL_LOG_ERROR_WIN();

    YAGL_LOG_FUNC_EXIT("Failed to create new context");

    return EGL_NO_CONTEXT;
}

static bool yagl_egl_wgl_make_current(struct yagl_egl_driver *driver,
                                      EGLNativeDisplayType dc,
                                      EGLSurface egl_draw_surf,
                                      EGLSurface egl_read_surf,
                                      EGLContext egl_glc)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);
    HDC draw_dc = NULL, read_dc = NULL;
    HGLRC glc = NULL;

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_make_current,
                       "dpy = %p, draw = %p, read = %p, ctx = %p",
                       dc,
                       egl_draw_surf,
                       egl_read_surf,
                       egl_glc);

    if (egl_glc != EGL_NO_CONTEXT) {
        glc = (HGLRC)egl_glc;
    }

    if (egl_draw_surf != EGL_NO_SURFACE) {
        draw_dc = egl_wgl->wglGetPbufferDCARB((HPBUFFERARB)egl_draw_surf);

        if (!draw_dc) {
            goto fail;
        }
    }

    if (egl_read_surf != EGL_NO_SURFACE) {
        if (egl_read_surf == egl_draw_surf) {
            read_dc = draw_dc;
        } else {
            read_dc = egl_wgl->wglGetPbufferDCARB((HPBUFFERARB)egl_read_surf);

            if (!read_dc) {
                goto fail;
            }
        }
    }

    if (egl_wgl->wglMakeContextCurrentARB(draw_dc, read_dc, glc) == FALSE) {
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("context %p was made current", glc);

    return true;

fail:
    YAGL_LOG_ERROR_WIN();

    if (draw_dc) {
        egl_wgl->wglReleasePbufferDCARB((HPBUFFERARB)egl_draw_surf, draw_dc);
    }

    if (read_dc && (egl_read_surf != egl_draw_surf)) {
        egl_wgl->wglReleasePbufferDCARB((HPBUFFERARB)egl_read_surf, read_dc);
    }

    YAGL_LOG_FUNC_EXIT("Failed to make context %p current", glc);

    return false;
}

static void yagl_egl_wgl_context_destroy(struct yagl_egl_driver *driver,
                                         EGLNativeDisplayType egl_dpy,
                                         EGLContext egl_glc)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_context_destroy,
                       "dpy = %p, ctx = %p",
                       egl_dpy, egl_glc);

    if (egl_wgl->wglDeleteContext((HGLRC)egl_glc) == FALSE) {
        YAGL_LOG_ERROR_WIN();
    }

    YAGL_LOG_FUNC_EXIT("Context destroyed");
}

static EGLSurface yagl_egl_wgl_pbuffer_surface_create(struct yagl_egl_driver *driver,
                                                      EGLNativeDisplayType egl_dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      EGLint width,
                                                      EGLint height,
                                                      const struct yagl_egl_pbuffer_attribs *attribs)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);
    HDC dc = NULL;
    YaglEglWglDpy * dpy = (YaglEglWglDpy *)egl_dpy;
    HPBUFFERARB pbuffer;
    int pbuff_attribs[] = {
        WGL_PBUFFER_LARGEST_ARB, FALSE,
        WGL_TEXTURE_TARGET_ARB, WGL_NO_TEXTURE_ARB,
        WGL_TEXTURE_FORMAT_ARB, WGL_NO_TEXTURE_ARB,
        0
    };

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_pbuffer_surface_create,
                       "dpy = %p, width = %d, height = %d, cfgid=%d",
                       egl_dpy,
                       width,
                       height,
                       cfg->config_id);

    if (attribs->largest) {
        pbuff_attribs[1] = TRUE;
    }

    if (attribs->tex_target == EGL_TEXTURE_2D) {
        pbuff_attribs[3] = WGL_TEXTURE_2D_ARB;
    }

    switch (attribs->tex_format) {
    case EGL_TEXTURE_RGB:
        pbuff_attribs[5] = WGL_TEXTURE_RGB_ARB;
        break;
    case EGL_TEXTURE_RGBA:
        pbuff_attribs[5] = WGL_TEXTURE_RGBA_ARB;
        break;
    }

    dc = yagl_egl_wgl_get_dc_by_cfgid(dpy, cfg);
    if (!dc) {
        YAGL_LOG_ERROR("failed to get DC with cfgid=%d", cfg->config_id);
        goto fail;
    }

    pbuffer = egl_wgl->wglCreatePbufferARB(dc, cfg->config_id,
             width, height, pbuff_attribs);

    if (!pbuffer) {
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("Surface created: %p", pbuffer);

    return (EGLSurface)pbuffer;

fail:
    YAGL_LOG_ERROR_WIN();

    YAGL_LOG_FUNC_EXIT("Surface creation failed");

    return EGL_NO_SURFACE;
}

static void yagl_egl_wgl_pbuffer_surface_destroy(struct yagl_egl_driver *driver,
                                                 EGLNativeDisplayType egl_dpy,
                                                 EGLSurface surf)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)(driver);
    HDC pbuf_dc = NULL;

    YAGL_EGL_WGL_ENTER(yagl_egl_wgl_pbuffer_surface_destroy,
                       "dpy = %p, sfc = %p",
                       egl_dpy,
                       surf);

    pbuf_dc = egl_wgl->wglGetPbufferDCARB((HPBUFFERARB)surf);

    if (!pbuf_dc ||
        egl_wgl->wglReleasePbufferDCARB((HPBUFFERARB)surf, pbuf_dc) != 1) {
        YAGL_LOG_ERROR_WIN();
    }

    if (egl_wgl->wglDestroyPbufferARB((HPBUFFERARB)surf) == FALSE) {
        YAGL_LOG_ERROR_WIN();
        YAGL_LOG_FUNC_EXIT("Failed to destroy surface");
    } else {
        YAGL_LOG_FUNC_EXIT("Surface destroyed");
    }
}

static void yagl_egl_wgl_destroy(struct yagl_egl_driver *driver)
{
    YaglEglWglDriver *egl_wgl = (YaglEglWglDriver *)driver;

    YAGL_LOG_FUNC_ENTER(yagl_egl_wgl_destroy, NULL);

    DestroyWindow(egl_wgl->win);

    UnregisterClassA((LPCTSTR)YAGL_EGL_WGL_WIN_CLASS, NULL);

    yagl_egl_driver_cleanup(driver);

    g_free(egl_wgl);

    YAGL_LOG_FUNC_EXIT(NULL);
}

/*
 * Check if extension string 'ext_string' contains extension name 'ext_name'.
 * Needed because extension name could be a substring of a longer extension
 * name.
 */
static bool yagl_egl_wgl_search_ext(const gchar *ext_string,
                                    const gchar *ext_name)
{
    const size_t name_len = strlen(ext_name);
    gchar *ext = g_strstr_len(ext_string, -1, ext_name);

    while (ext) {
        if (ext[name_len] == '\0' || ext[name_len] == ' ') {
            break;
        }

        ext += name_len;
        ext = g_strstr_len(ext, -1, ext_name);
    }

    return ext != NULL;
}

/*
 *  A stub for YaGL window class window procedure callback
 */
static LRESULT CALLBACK yagl_egl_wgl_winproc(HWND win, UINT msg, WPARAM wparam,
                                      LPARAM lparam)
{
    return DefWindowProcA(win, msg, wparam, lparam);
}

/*
 * Initialize pointers to WGL extension functions.
 * Pointers are acquired with wglGetProcAddress() function, which works only
 * in a presence of a valid openGL context. Pointers returned by
 * wglGetProcAddress() are themselves context-specific, but we assume that
 * if two contexts refer to the same GPU, then the function pointers pulled
 * from one context will work in another.
 * Note that we only need current context to ACQUIRE function pointers,
 * we don't need a context to use them.
 */
static bool yagl_egl_wgl_init_ext(YaglEglWglDriver *egl_wgl)
{
    HWND win;
    HDC dc = NULL;
    HGLRC glc = NULL;
    const char *ext_str = NULL;
    bool ext_initialized = false;

    YAGL_LOG_FUNC_ENTER(yagl_egl_wgl_init_ext, NULL);

    win = yagl_egl_wgl_dummy_win_create();
    if (!win) {
        goto out;
    }

    dc = GetDC(win);
    if (!dc) {
        goto out;
    }

    if (!yagl_egl_wgl_dc_set_def_pixfmt(dc)) {
        goto out;
    }

    glc = egl_wgl->wglCreateContext(dc);
    if (!glc) {
        goto out;
    }

    if (!egl_wgl->wglMakeCurrent(dc, glc)) {
        goto out;
    }

    /* WGL extensions couldn't be queried by glGetString(), we need to use
     * wglGetExtensionsStringARB or wglGetExtensionsStringEXT for this, which
     * themselves are extensions */
    egl_wgl->wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
            egl_wgl->wglGetProcAddress((LPCSTR)"wglGetExtensionsStringARB");
    egl_wgl->wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
            egl_wgl->wglGetProcAddress((LPCSTR)"wglGetExtensionsStringEXT");

    if (egl_wgl->wglGetExtensionsStringARB) {
        ext_str = egl_wgl->wglGetExtensionsStringARB(dc);
    } else if (egl_wgl->wglGetExtensionsStringEXT) {
        ext_str = egl_wgl->wglGetExtensionsStringEXT();
    }

    if (!ext_str) {
        YAGL_LOG_ERROR("Couldn't acquire WGL extensions string");
        goto out;
    }

    YAGL_LOG_INFO("WGL extensions: %s", ext_str);

    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pixel_format, wglGetPixelFormatAttribivARB, PFNWGLGETPIXELFORMATATTRIBIVARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, wglCreatePbufferARB, PFNWGLCREATEPBUFFERARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, wglGetPbufferDCARB, PFNWGLGETPBUFFERDCARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, wglReleasePbufferDCARB, PFNWGLRELEASEPBUFFERDCARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, wglDestroyPbufferARB, PFNWGLDESTROYPBUFFERARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_pixel_format, wglChoosePixelFormatARB, PFNWGLCHOOSEPIXELFORMATARBPROC);
    YAGL_EGL_WGL_GET_EXT_PROC(WGL_ARB_make_current_read, wglMakeContextCurrentARB, PFNWGLMAKECONTEXTCURRENTARBPROC);

    ext_initialized = true;

out:
	if (glc) {
	    if (egl_wgl->wglMakeCurrent(NULL, NULL) == FALSE) {
	        YAGL_LOG_ERROR_WIN();
	    }

	    if (egl_wgl->wglDeleteContext(glc) == FALSE) {
	        YAGL_LOG_ERROR_WIN();
	    }
	}

	if (win) {
	    if (dc) {
	        ReleaseDC(win, dc);
	    }
	    DestroyWindow(win);
	}

    YAGL_LOG_FUNC_EXIT("Extensions initialized status: %u", ext_initialized);

    return ext_initialized;
}

struct yagl_egl_driver *yagl_egl_driver_create(void *display)
{
    YaglEglWglDriver *egl_wgl;
    struct yagl_egl_driver *egl_driver;
    WNDCLASSEXA yagl_win_class;

    YAGL_LOG_FUNC_ENTER(yagl_egl_wgl_create, NULL);

    egl_wgl = g_new0(YaglEglWglDriver, 1);

    egl_driver = &egl_wgl->base;

    yagl_egl_driver_init(egl_driver);

    egl_driver->dyn_lib = yagl_dyn_lib_create();

    if (!yagl_dyn_lib_load(egl_driver->dyn_lib, "opengl32")) {
        YAGL_LOG_ERROR("Loading opengl32.dll failed with error: %s",
                       yagl_dyn_lib_get_error(egl_driver->dyn_lib));
        goto fail;
    }

    yagl_win_class.cbSize = sizeof(WNDCLASSEXA);
    yagl_win_class.style = 0;
    yagl_win_class.lpfnWndProc = &yagl_egl_wgl_winproc;
    yagl_win_class.cbClsExtra = 0;
    yagl_win_class.cbWndExtra = 0;
    yagl_win_class.hInstance = NULL;
    yagl_win_class.hIcon = NULL;
    yagl_win_class.hCursor = NULL;
    yagl_win_class.hbrBackground = NULL;
    yagl_win_class.lpszMenuName =  NULL;
    yagl_win_class.lpszClassName = YAGL_EGL_WGL_WIN_CLASS;
    yagl_win_class.hIconSm = NULL;

    if (!RegisterClassExA(&yagl_win_class)) {
        goto fail;
    }

    egl_wgl->win = yagl_egl_wgl_dummy_win_create();

    if (!egl_wgl->win) {
        goto fail;
    }

    YAGL_EGL_WGL_GET_PROC(WGLCREATECONTEXTPROC, wglCreateContext);
    YAGL_EGL_WGL_GET_PROC(WGLDELETECONTEXTPROC, wglDeleteContext);
    YAGL_EGL_WGL_GET_PROC(WGLGETPROCADDRESSPROC, wglGetProcAddress);
    YAGL_EGL_WGL_GET_PROC(WGLMAKECURRENTPROC, wglMakeCurrent);
    YAGL_EGL_WGL_GET_PROC(WGLSHARELISTSPROC, wglShareLists);

    egl_driver->display_open = &yagl_egl_wgl_display_open;
    egl_driver->display_close = &yagl_egl_wgl_display_close;
    egl_driver->config_enum = &yagl_egl_wgl_config_enum;
    egl_driver->config_cleanup = &yagl_egl_wgl_config_cleanup;
    egl_driver->pbuffer_surface_create = &yagl_egl_wgl_pbuffer_surface_create;
    egl_driver->pbuffer_surface_destroy = &yagl_egl_wgl_pbuffer_surface_destroy;
    egl_driver->context_create = &yagl_egl_wgl_context_create;
    egl_driver->context_destroy = &yagl_egl_wgl_context_destroy;
    egl_driver->make_current = &yagl_egl_wgl_make_current;
    egl_driver->destroy = &yagl_egl_wgl_destroy;

    if (!yagl_egl_wgl_init_ext(egl_wgl)) {
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("EGL WGL driver created (%p)", egl_driver);

    return egl_driver;

fail:
    yagl_egl_driver_cleanup(egl_driver);
    g_free(egl_wgl);

    YAGL_LOG_ERROR_WIN();

    YAGL_LOG_FUNC_EXIT("EGL_WGL driver creation failed");

    return NULL;
}

static void *yagl_egl_wgl_extaddr_get(struct yagl_dyn_lib *dyn_lib,
                                      const char *sym)
{
    static WGLGETPROCADDRESSPROC wgl_get_procaddr_fn = NULL;
    static WGLCREATECONTEXTPROC wgl_create_context_fn = NULL;
    static WGLMAKECURRENTPROC wgl_make_current_fn = NULL;
    static WGLDELETECONTEXTPROC wgl_delete_context_fn = NULL;
    void *proc = NULL;
    HWND win = NULL;
    HDC dc = NULL;
    HGLRC glc;

    YAGL_LOG_FUNC_ENTER(yagl_egl_wgl_extaddr_get, "%s", sym);

    if (!wgl_get_procaddr_fn) {
        wgl_get_procaddr_fn = (WGLGETPROCADDRESSPROC)
                yagl_dyn_lib_get_sym(dyn_lib, "wglGetProcAddress");
    }

    if (!wgl_create_context_fn) {
        wgl_create_context_fn = (WGLCREATECONTEXTPROC)
                yagl_dyn_lib_get_sym(dyn_lib, "wglCreateContext");
    }

    if (!wgl_make_current_fn) {
        wgl_make_current_fn = (WGLMAKECURRENTPROC)
                yagl_dyn_lib_get_sym(dyn_lib, "wglMakeCurrent");
    }

    if (!wgl_delete_context_fn) {
        wgl_delete_context_fn = (WGLDELETECONTEXTPROC)
                yagl_dyn_lib_get_sym(dyn_lib, "wglDeleteContext");
    }

    if (!wgl_get_procaddr_fn || !wgl_create_context_fn ||
        !wgl_make_current_fn || !wgl_delete_context_fn) {
        goto out;
    }

    /* To use wglGetProcAddress, create a temporary openGL context
     * and make it current */

    win = yagl_egl_wgl_dummy_win_create();
    if (!win) {
        goto out;
    }

    dc = GetDC(win);
    if (!dc) {
        goto out;
    }

    /* We need to set pixel format of dc before we can create GL context */
    if (!yagl_egl_wgl_dc_set_def_pixfmt(dc)) {
        goto out;
    }

    glc = wgl_create_context_fn(dc);
    if (!glc) {
        goto out;
    }

    if (wgl_make_current_fn(dc, glc) == FALSE) {
        wgl_make_current_fn(NULL, NULL);
        wgl_delete_context_fn(glc);
        goto out;
    }

    proc = (void *)wgl_get_procaddr_fn((LPCSTR)sym);

    if (wgl_make_current_fn(NULL, NULL) == FALSE) {
        YAGL_LOG_ERROR_WIN();
    }

    if (wgl_delete_context_fn(glc) == FALSE) {
        YAGL_LOG_ERROR_WIN();
    }

out:
   if (!proc) {
       YAGL_LOG_ERROR_WIN();
   }

    if (win) {
        if (dc) {
            ReleaseDC(win, dc);
        }
        DestroyWindow(win);
    }

    YAGL_LOG_FUNC_EXIT(NULL);

    return proc;
}

/* GetProcAddress only works for core OpenGL v.1.1 functions, while
 * wglGetProcAddress will fail for <=v1.1 openGL functions, so we need them
 * both */
void *yagl_dyn_lib_get_ogl_procaddr(struct yagl_dyn_lib *dyn_lib,
                                    const char *sym)
{
    void *proc = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_wgl_get_procaddr,
    		"Retrieving %s address", sym);

    proc = yagl_egl_wgl_extaddr_get(dyn_lib, sym);

    if (!proc) {
    	YAGL_LOG_TRACE("wglGetProcAddress failed for %s, trying GetProcAddress",
    	    sym);
        proc = yagl_dyn_lib_get_sym(dyn_lib, sym);
    }

    YAGL_LOG_FUNC_EXIT("%s address: %p", sym, proc);

    return proc;
}
