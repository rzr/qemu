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

#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_egl_native_config.h"
#include "yagl_egl_surface_attribs.h"
#include <GL/glx.h>

#define YAGL_EGL_GLX_ENTER(func, format, ...) \
    YAGL_LOG_FUNC_ENTER(func, format,##__VA_ARGS__)

#define YAGL_EGL_GLX_GET_PROC(proc_type, proc_name) \
    do { \
        egl_glx->proc_name = \
            (proc_type)egl_glx->glXGetProcAddress((const GLubyte*)#proc_name); \
        if (!egl_glx->proc_name) { \
            YAGL_LOG_ERROR("Unable to get symbol: %s", \
                           yagl_dyn_lib_get_error(egl_driver->dyn_lib)); \
            goto fail; \
        } \
    } while (0)

#define YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(attribute, value) \
    if (egl_glx->glXGetFBConfigAttrib(dpy, glx_cfg, (attribute), (value)) != Success) { \
        YAGL_LOG_WARN("glXGetFBConfigAttrib failed to get " #attribute); \
        YAGL_LOG_FUNC_EXIT(NULL); \
        return false; \
    }

#ifndef GLX_VERSION_1_4
#error GL/glx.h must be equal to or greater than GLX 1.4
#endif

/* GLX 1.0 */
typedef GLXContext (*GLXCREATECONTEXTPROC)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
typedef void (*GLXDESTROYCONTEXTPROC)( Display *dpy, GLXContext ctx );
typedef Bool (*GLXMAKECURRENTPROC)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
typedef void (*GLXSWAPBUFFERSPROC)( Display *dpy, GLXDrawable drawable );
typedef GLXPixmap (*GLXCREATEGLXPIXMAPPROC)( Display *dpy, XVisualInfo *visual, Pixmap pixmap );
typedef void (*GLXDESTROYGLXPIXMAPPROC)( Display *dpy, GLXPixmap pixmap );
typedef Bool (*GLXQUERYVERSIONPROC)( Display *dpy, int *maj, int *min );
typedef int (*GLXGETCONFIGPROC)( Display *dpy, XVisualInfo *visual, int attrib, int *value );
typedef void (*GLXWAITGLPROC)( void );
typedef void (*GLXWAITXPROC)( void );

/* GLX 1.1 */
typedef const char *(*GLXQUERYEXTENSIONSSTRINGPROC)( Display *dpy, int screen );
typedef const char *(*GLXQUERYSERVERSTRINGPROC)( Display *dpy, int screen, int name );
typedef const char *(*GLXGETCLIENTSTRINGPROC)( Display *dpy, int name );

struct yagl_egl_glx
{
    struct yagl_egl_driver base;

    EGLNativeDisplayType global_dpy;

    /* GLX 1.0 */
    GLXCREATECONTEXTPROC glXCreateContext;
    GLXDESTROYCONTEXTPROC glXDestroyContext;
    GLXMAKECURRENTPROC glXMakeCurrent;
    GLXSWAPBUFFERSPROC glXSwapBuffers;
    GLXCREATEGLXPIXMAPPROC glXCreateGLXPixmap;
    GLXDESTROYGLXPIXMAPPROC glXDestroyGLXPixmap;
    GLXQUERYVERSIONPROC glXQueryVersion;
    GLXGETCONFIGPROC glXGetConfig;
    GLXWAITGLPROC glXWaitGL;
    GLXWAITXPROC glXWaitX;

    /* GLX 1.1 */
    GLXQUERYEXTENSIONSSTRINGPROC glXQueryExtensionsString;
    GLXQUERYSERVERSTRINGPROC glXQueryServerString;
    GLXGETCLIENTSTRINGPROC glXGetClientString;

    /* GLX 1.3 or (GLX_SGI_make_current_read and GLX_SGIX_fbconfig) */
    PFNGLXGETFBCONFIGSPROC glXGetFBConfigs;
    PFNGLXGETFBCONFIGATTRIBPROC glXGetFBConfigAttrib;
    PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfig;
    PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig;
    PFNGLXCREATEWINDOWPROC glXCreateWindow;
    PFNGLXDESTROYWINDOWPROC glXDestroyWindow;
    PFNGLXCREATEPIXMAPPROC glXCreatePixmap;
    PFNGLXDESTROYPIXMAPPROC glXDestroyPixmap;
    PFNGLXCREATEPBUFFERPROC glXCreatePbuffer;
    PFNGLXDESTROYPBUFFERPROC glXDestroyPbuffer;
    PFNGLXCREATENEWCONTEXTPROC glXCreateNewContext;
    PFNGLXMAKECONTEXTCURRENTPROC glXMakeContextCurrent;

    /* GLX 1.4 or GLX_ARB_get_proc_address */
    PFNGLXGETPROCADDRESSPROC glXGetProcAddress;

    /* GLX_ARB_create_context */
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
};

static bool yagl_egl_glx_get_gl_version(struct yagl_egl_glx *egl_glx,
                                        yagl_gl_version *version)
{
    int config_attribs[] =
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
    int ctx_attribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        GLX_RENDER_TYPE, GLX_RGBA_TYPE,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };
    int surface_attribs[] = {
        GLX_PBUFFER_WIDTH, 1,
        GLX_PBUFFER_HEIGHT, 1,
        GLX_LARGEST_PBUFFER, False,
        GLX_PRESERVED_CONTENTS, False,
        None
    };
    bool res = false;
    const char *tmp;
    int n = 0;
    GLXFBConfig *configs = NULL;
    GLXContext ctx = NULL;
    GLXPbuffer pbuffer = 0;
    const GLubyte *(GLAPIENTRY *GetStringi)(GLenum, GLuint) = NULL;
    void (GLAPIENTRY *GetIntegerv)(GLenum, GLint*) = NULL;
    GLint i, num_extensions = 0;
    GLint major = 0, minor = 0;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_get_gl_version, NULL);

    tmp = getenv("GL_VERSION");

    if (tmp) {
        if (strcmp(tmp, "2") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 2.1");
            *version = yagl_gl_2;
            res = true;
        } else if (strcmp(tmp, "3_1") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 3.1");
            *version = yagl_gl_3_1;
            res = true;
        } else if (strcmp(tmp, "3_1_es3") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 3.1 ES3");
            *version = yagl_gl_3_1_es3;
            res = true;
        } else if (strcmp(tmp, "3_2") == 0) {
            YAGL_LOG_INFO("GL_VERSION forces OpenGL version to 3.2");
            *version = yagl_gl_3_2;
            res = true;
        } else {
            YAGL_LOG_CRITICAL("Bad GL_VERSION value = %s", tmp);
        }

        goto out;
    }

    configs = egl_glx->glXChooseFBConfig(egl_glx->global_dpy,
                                         DefaultScreen(egl_glx->global_dpy),
                                         config_attribs,
                                         &n);

    if (n <= 0) {
        YAGL_LOG_ERROR("glXChooseFBConfig failed");
        goto out;
    }

    ctx = egl_glx->glXCreateContextAttribsARB(egl_glx->global_dpy,
                                              configs[0],
                                              NULL,
                                              True,
                                              ctx_attribs);

    if (!ctx) {
        YAGL_LOG_INFO("glXCreateContextAttribsARB failed, using OpenGL 2.1");
        *version = yagl_gl_2;
        res = true;
        goto out;
    }

    pbuffer = egl_glx->glXCreatePbuffer(egl_glx->global_dpy,
                                        configs[0],
                                        surface_attribs);

    if (!pbuffer) {
        YAGL_LOG_ERROR("glXCreatePbuffer failed");
        goto out;
    }

    if (!egl_glx->glXMakeContextCurrent(egl_glx->global_dpy,
                                        pbuffer, pbuffer, ctx)) {
        YAGL_LOG_ERROR("glXMakeContextCurrent failed");
        goto out;
    }

    GetStringi = yagl_dyn_lib_get_ogl_procaddr(egl_glx->base.dyn_lib,
                                               "glGetStringi");

    if (!GetStringi) {
        YAGL_LOG_ERROR("Unable to get symbol: %s",
                       yagl_dyn_lib_get_error(egl_glx->base.dyn_lib));
        goto out;
    }

    GetIntegerv = yagl_dyn_lib_get_ogl_procaddr(egl_glx->base.dyn_lib,
                                                "glGetIntegerv");

    if (!GetIntegerv) {
        YAGL_LOG_ERROR("Unable to get symbol: %s",
                       yagl_dyn_lib_get_error(egl_glx->base.dyn_lib));
        goto out;
    }

    GetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    for (i = 0; i < num_extensions; ++i) {
        tmp = (const char*)GetStringi(GL_EXTENSIONS, i);
        if (strcmp(tmp, "GL_ARB_ES3_compatibility") == 0) {
            YAGL_LOG_INFO("GL_ARB_ES3_compatibility supported, using OpenGL 3.1 ES3");
            *version = yagl_gl_3_1_es3;
            res = true;
            goto out;
        }
    }

    /*
     * No GL_ARB_ES3_compatibility, so we need at least OpenGL 3.2 to be
     * able to patch shaders and run them with GLSL 1.50.
     */

    GetIntegerv(GL_MAJOR_VERSION, &major);
    GetIntegerv(GL_MINOR_VERSION, &minor);

    if ((major > 3) ||
        ((major == 3) && (minor >= 2))) {
        YAGL_LOG_INFO("GL_ARB_ES3_compatibility not supported, using OpenGL 3.2");
        *version = yagl_gl_3_2;
        res = true;
        goto out;
    }

    YAGL_LOG_INFO("GL_ARB_ES3_compatibility not supported, OpenGL 3.2 not supported, using OpenGL 3.1");
    *version = yagl_gl_3_1;
    res = true;

out:
    if (pbuffer) {
        egl_glx->glXDestroyPbuffer(egl_glx->global_dpy, pbuffer);
    }
    if (ctx) {
        egl_glx->glXMakeContextCurrent(egl_glx->global_dpy, 0, 0, NULL);
        egl_glx->glXDestroyContext(egl_glx->global_dpy, ctx);
    }
    if (configs) {
        XFree(configs);
    }

    if (res) {
        YAGL_LOG_FUNC_EXIT("%d, version = %u", res, *version);
    } else {
        YAGL_LOG_FUNC_EXIT("%d", res);
    }

    return res;
}

/*
 * INTERNAL IMPLEMENTATION FUNCTIONS
 * @{
 */

static bool yagl_egl_glx_config_fill(struct yagl_egl_driver *driver,
                                     EGLNativeDisplayType dpy,
                                     struct yagl_egl_native_config *cfg,
                                     GLXFBConfig glx_cfg)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;
    int tmp = 0;
    int transparent_type = 0;
    int trans_red_val = 0;
    int trans_green_val = 0;
    int trans_blue_val = 0;
    int buffer_size = 0;
    int red_size = 0;
    int green_size = 0;
    int blue_size = 0;
    int alpha_size = 0;
    int depth_size = 0;
    int stencil_size = 0;
    int native_visual_id = 0;
    int native_visual_type = 0;
    int caveat = 0;
    int max_pbuffer_width = 0;
    int max_pbuffer_height = 0;
    int max_pbuffer_size = 0;
    int frame_buffer_level = 0;
    int config_id = 0;
    int samples_per_pixel = 0;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_config_fill, NULL);

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_FBCONFIG_ID, &config_id);

    YAGL_LOG_TRACE("id = %d", config_id);

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_RENDER_TYPE, &tmp);

    if ((tmp & GLX_RGBA_BIT) == 0) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return false;
    }

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_DRAWABLE_TYPE, &tmp);

    if ((tmp & GLX_PBUFFER_BIT) == 0) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return false;
    }

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_TRANSPARENT_TYPE, &tmp);

    if (tmp == GLX_TRANSPARENT_INDEX) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return false;
    } else if(tmp == GLX_NONE) {
        transparent_type = EGL_NONE;
    } else {
        transparent_type = EGL_TRANSPARENT_RGB;

        YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_TRANSPARENT_RED_VALUE, &trans_red_val);
        YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_TRANSPARENT_GREEN_VALUE, &trans_green_val);
        YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_TRANSPARENT_BLUE_VALUE, &trans_blue_val);
    }

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_BUFFER_SIZE, &buffer_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_RED_SIZE, &red_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_GREEN_SIZE, &green_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_BLUE_SIZE, &blue_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_ALPHA_SIZE, &alpha_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_DEPTH_SIZE, &depth_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_STENCIL_SIZE, &stencil_size);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_X_VISUAL_TYPE, &native_visual_type);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_VISUAL_ID, &native_visual_id);

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_CONFIG_CAVEAT, &tmp);

    if (tmp == GLX_NONE) {
        caveat = EGL_NONE;
    } else if (tmp == GLX_SLOW_CONFIG) {
        caveat = EGL_SLOW_CONFIG;
    } else if (tmp == GLX_NON_CONFORMANT_CONFIG) {
        caveat = EGL_NON_CONFORMANT_CONFIG;
    }

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_MAX_PBUFFER_WIDTH, &max_pbuffer_width);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_MAX_PBUFFER_HEIGHT, &max_pbuffer_height);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_MAX_PBUFFER_PIXELS, &max_pbuffer_size);

    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_LEVEL, &frame_buffer_level);
    YAGL_EGL_GLX_GET_CONFIG_ATTRIB_RET(GLX_SAMPLES, &samples_per_pixel);

    yagl_egl_native_config_init(cfg);

    cfg->red_size = red_size;
    cfg->green_size = green_size;
    cfg->blue_size = blue_size;
    cfg->alpha_size = alpha_size;
    cfg->buffer_size = buffer_size;
    cfg->caveat = caveat;
    cfg->config_id = config_id;
    cfg->frame_buffer_level = frame_buffer_level;
    cfg->depth_size = depth_size;
    cfg->max_pbuffer_width = max_pbuffer_width;
    cfg->max_pbuffer_height = max_pbuffer_height;
    cfg->max_pbuffer_size = max_pbuffer_size;
    cfg->max_swap_interval = 1000;
    cfg->min_swap_interval = 0;
    cfg->native_visual_id = native_visual_id;
    cfg->native_visual_type = native_visual_type;
    cfg->samples_per_pixel = samples_per_pixel;
    cfg->stencil_size = stencil_size;
    cfg->transparent_type = transparent_type;
    cfg->trans_red_val = trans_red_val;
    cfg->trans_green_val = trans_green_val;
    cfg->trans_blue_val = trans_blue_val;

    cfg->driver_data = glx_cfg;

    YAGL_LOG_FUNC_EXIT(NULL);

    return true;
}

/*
 * @}
 */

/*
 * PUBLIC FUNCTIONS
 * @{
 */

static EGLNativeDisplayType yagl_egl_glx_display_open(struct yagl_egl_driver *driver)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_display_open, NULL);

    YAGL_LOG_FUNC_EXIT("%p", egl_glx->global_dpy);

    return egl_glx->global_dpy;
}

static void yagl_egl_glx_display_close(struct yagl_egl_driver *driver,
                                       EGLNativeDisplayType dpy)
{
    YAGL_EGL_GLX_ENTER(yagl_egl_glx_display_close, "%p", dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
    *yagl_egl_glx_config_enum(struct yagl_egl_driver *driver,
                              EGLNativeDisplayType dpy,
                              int *num_configs)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;
    struct yagl_egl_native_config *configs = NULL;
    int glx_cfg_index, cfg_index, n;
    GLXFBConfig *glx_configs;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_config_enum, "%p", dpy);

    *num_configs = 0;

    glx_configs = egl_glx->glXGetFBConfigs(dpy, DefaultScreen(dpy), &n);

    YAGL_LOG_TRACE("got %d configs", n);

    configs = g_malloc0(n * sizeof(*configs));

    for (glx_cfg_index = 0, cfg_index = 0; glx_cfg_index < n; ++glx_cfg_index) {
        if (yagl_egl_glx_config_fill(driver,
                                     dpy,
                                     &configs[cfg_index],
                                     glx_configs[glx_cfg_index])) {
            ++cfg_index;
        } else {
            /*
             * Clean up so we could construct here again.
             */
            memset(&configs[cfg_index],
                   0,
                   sizeof(struct yagl_egl_native_config));
        }
    }

    *num_configs = cfg_index;

    XFree(glx_configs);

    YAGL_LOG_DEBUG("glXGetFBConfigs returned %d configs, %d are usable",
                   n,
                   *num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return configs;
}

static void yagl_egl_glx_config_cleanup(struct yagl_egl_driver *driver,
                                        EGLNativeDisplayType dpy,
                                        const struct yagl_egl_native_config *cfg)
{
    YAGL_EGL_GLX_ENTER(yagl_egl_glx_config_cleanup,
                       "dpy = %p, cfg = %d",
                       dpy,
                       cfg->config_id);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLSurface yagl_egl_glx_pbuffer_surface_create(struct yagl_egl_driver *driver,
                                                      EGLNativeDisplayType dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      EGLint width,
                                                      EGLint height,
                                                      const struct yagl_egl_pbuffer_attribs *attribs)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;
    int glx_attribs[] = {
        GLX_PBUFFER_WIDTH, width,
        GLX_PBUFFER_HEIGHT, height,
        GLX_LARGEST_PBUFFER, False,
        GLX_PRESERVED_CONTENTS, True,
        None
    };
    GLXPbuffer pbuffer;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_pbuffer_surface_create,
                       "dpy = %p, width = %d, height = %d",
                       dpy,
                       width,
                       height);

    pbuffer = egl_glx->glXCreatePbuffer(dpy,
                                        (GLXFBConfig)cfg->driver_data,
                                        glx_attribs);

    if (!pbuffer) {
        YAGL_LOG_ERROR("glXCreatePbuffer failed");

        YAGL_LOG_FUNC_EXIT(NULL);

        return EGL_NO_SURFACE;
    } else {
        YAGL_LOG_FUNC_EXIT("%p", (EGLSurface)pbuffer);

        return (EGLSurface)pbuffer;
    }
}

static void yagl_egl_glx_pbuffer_surface_destroy(struct yagl_egl_driver *driver,
                                                 EGLNativeDisplayType dpy,
                                                 EGLSurface sfc)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_pbuffer_surface_destroy,
                       "dpy = %p, sfc = %p",
                       dpy,
                       sfc);

    egl_glx->glXDestroyPbuffer(dpy, (GLXPbuffer)sfc);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLContext yagl_egl_glx_context_create(struct yagl_egl_driver *driver,
                                              EGLNativeDisplayType dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              EGLContext share_context,
                                              int version)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;
    GLXContext ctx;
    int attribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        GLX_RENDER_TYPE, GLX_RGBA_TYPE,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_context_create,
                       "dpy = %p, share_context = %p, version = %d",
                       dpy,
                       share_context,
                       version);

    if ((egl_glx->base.gl_version > yagl_gl_2) && (version != 1)) {
        ctx = egl_glx->glXCreateContextAttribsARB(dpy,
                                                  (GLXFBConfig)cfg->driver_data,
                                                  ((share_context == EGL_NO_CONTEXT) ?
                                                      NULL
                                                    : (GLXContext)share_context),
                                                  True,
                                                  attribs);
    } else {
        ctx = egl_glx->glXCreateNewContext(dpy,
                                           (GLXFBConfig)cfg->driver_data,
                                           GLX_RGBA_TYPE,
                                           ((share_context == EGL_NO_CONTEXT) ?
                                               NULL
                                             : (GLXContext)share_context),
                                           True);
    }

    if (!ctx) {
        YAGL_LOG_ERROR("glXCreateContextAttribsARB/glXCreateNewContext failed");

        YAGL_LOG_FUNC_EXIT(NULL);

        return EGL_NO_CONTEXT;
    } else {
        YAGL_LOG_FUNC_EXIT("%p", (EGLContext)ctx);

        return (EGLContext)ctx;
    }
}

static void yagl_egl_glx_context_destroy(struct yagl_egl_driver *driver,
                                         EGLNativeDisplayType dpy,
                                         EGLContext ctx)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_context_destroy,
                       "dpy = %p, ctx = %p",
                       dpy,
                       ctx);

    egl_glx->glXDestroyContext(dpy, (GLXContext)ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_egl_glx_make_current(struct yagl_egl_driver *driver,
                                      EGLNativeDisplayType dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;
    bool ret;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_make_current,
                       "dpy = %p, draw = %p, read = %p, ctx = %p",
                       dpy,
                       draw,
                       read,
                       ctx);

    ret = egl_glx->glXMakeContextCurrent(dpy, (GLXDrawable)draw, (GLXDrawable)read, (GLXContext)ctx);

    YAGL_LOG_FUNC_EXIT("%u", (uint32_t)ret);

    return ret;
}

/*
 * @}
 */

static void yagl_egl_glx_destroy(struct yagl_egl_driver *driver)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;

    YAGL_LOG_FUNC_ENTER(yagl_egl_glx_destroy, NULL);

    yagl_egl_driver_cleanup(&egl_glx->base);

    g_free(egl_glx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_driver *yagl_egl_driver_create(void *display)
{
    struct yagl_egl_driver *egl_driver;
    struct yagl_egl_glx *egl_glx;
    Display *x_display = display;

    YAGL_LOG_FUNC_ENTER(yagl_egl_glx_create, NULL);

    egl_glx = g_malloc0(sizeof(*egl_glx));

    egl_glx->global_dpy = x_display;

    egl_driver = &egl_glx->base;

    yagl_egl_driver_init(egl_driver);

    egl_driver->dyn_lib = yagl_dyn_lib_create();

    if (!egl_driver->dyn_lib) {
        goto fail;
    }

    if (!yagl_dyn_lib_load(egl_driver->dyn_lib, "libGL.so.1")) {
        YAGL_LOG_ERROR("Unable to load libGL.so.1: %s",
                       yagl_dyn_lib_get_error(egl_driver->dyn_lib));
        goto fail;
    }

    egl_glx->glXGetProcAddress =
        yagl_dyn_lib_get_sym(egl_driver->dyn_lib, "glXGetProcAddress");

    if (!egl_glx->glXGetProcAddress) {
        egl_glx->glXGetProcAddress =
            yagl_dyn_lib_get_sym(egl_driver->dyn_lib, "glXGetProcAddressARB");
    }

    if (!egl_glx->glXGetProcAddress) {
        YAGL_LOG_ERROR("Unable to get symbol: %s",
                       yagl_dyn_lib_get_error(egl_driver->dyn_lib));
        goto fail;
    }

    /* GLX 1.0 */
    YAGL_EGL_GLX_GET_PROC(GLXCREATECONTEXTPROC, glXCreateContext);
    YAGL_EGL_GLX_GET_PROC(GLXDESTROYCONTEXTPROC, glXDestroyContext);
    YAGL_EGL_GLX_GET_PROC(GLXMAKECURRENTPROC, glXMakeCurrent);
    YAGL_EGL_GLX_GET_PROC(GLXSWAPBUFFERSPROC, glXSwapBuffers);
    YAGL_EGL_GLX_GET_PROC(GLXCREATEGLXPIXMAPPROC, glXCreateGLXPixmap);
    YAGL_EGL_GLX_GET_PROC(GLXDESTROYGLXPIXMAPPROC, glXDestroyGLXPixmap);
    YAGL_EGL_GLX_GET_PROC(GLXQUERYVERSIONPROC, glXQueryVersion);
    YAGL_EGL_GLX_GET_PROC(GLXGETCONFIGPROC, glXGetConfig);
    YAGL_EGL_GLX_GET_PROC(GLXWAITGLPROC, glXWaitGL);
    YAGL_EGL_GLX_GET_PROC(GLXWAITXPROC, glXWaitX);

    /* GLX 1.1 */
    YAGL_EGL_GLX_GET_PROC(GLXQUERYEXTENSIONSSTRINGPROC, glXQueryExtensionsString);
    YAGL_EGL_GLX_GET_PROC(GLXQUERYSERVERSTRINGPROC, glXQueryServerString);
    YAGL_EGL_GLX_GET_PROC(GLXGETCLIENTSTRINGPROC, glXGetClientString);

    /* GLX 1.3 */
    YAGL_EGL_GLX_GET_PROC(PFNGLXGETFBCONFIGSPROC, glXGetFBConfigs);
    YAGL_EGL_GLX_GET_PROC(PFNGLXGETFBCONFIGATTRIBPROC, glXGetFBConfigAttrib);
    YAGL_EGL_GLX_GET_PROC(PFNGLXGETVISUALFROMFBCONFIGPROC, glXGetVisualFromFBConfig);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCHOOSEFBCONFIGPROC, glXChooseFBConfig);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEWINDOWPROC, glXCreateWindow);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYWINDOWPROC, glXDestroyWindow);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEPIXMAPPROC, glXCreatePixmap);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYPIXMAPPROC, glXDestroyPixmap);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEPBUFFERPROC, glXCreatePbuffer);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYPBUFFERPROC, glXDestroyPbuffer);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATENEWCONTEXTPROC, glXCreateNewContext);
    YAGL_EGL_GLX_GET_PROC(PFNGLXMAKECONTEXTCURRENTPROC, glXMakeContextCurrent);

    /* GLX_ARB_create_context */
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATECONTEXTATTRIBSARBPROC, glXCreateContextAttribsARB);

    if (!yagl_egl_glx_get_gl_version(egl_glx, &egl_glx->base.gl_version)) {
        goto fail;
    }

    egl_glx->base.display_open = &yagl_egl_glx_display_open;
    egl_glx->base.display_close = &yagl_egl_glx_display_close;
    egl_glx->base.config_enum = &yagl_egl_glx_config_enum;
    egl_glx->base.config_cleanup = &yagl_egl_glx_config_cleanup;
    egl_glx->base.pbuffer_surface_create = &yagl_egl_glx_pbuffer_surface_create;
    egl_glx->base.pbuffer_surface_destroy = &yagl_egl_glx_pbuffer_surface_destroy;
    egl_glx->base.context_create = &yagl_egl_glx_context_create;
    egl_glx->base.context_destroy = &yagl_egl_glx_context_destroy;
    egl_glx->base.make_current = &yagl_egl_glx_make_current;
    egl_glx->base.destroy = &yagl_egl_glx_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_glx->base;

fail:
    yagl_egl_driver_cleanup(&egl_glx->base);
    g_free(egl_glx);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}

void *yagl_dyn_lib_get_ogl_procaddr(struct yagl_dyn_lib *dyn_lib,
                                    const char *sym_name)
{
    PFNGLXGETPROCADDRESSPROC get_proc_addr;
    void *res = NULL;

    get_proc_addr = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddress");

    if (!get_proc_addr) {
        get_proc_addr = yagl_dyn_lib_get_sym(dyn_lib, "glXGetProcAddressARB");
    }

    if (get_proc_addr) {
        res = get_proc_addr((const GLubyte*)sym_name);
    }

    if (!res) {
        res = yagl_dyn_lib_get_sym(dyn_lib, sym_name);
    }

    return res;
}
