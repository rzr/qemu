#include "yagl_egl_glx.h"
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
    YAGL_LOG_FUNC_ENTER((egl_glx_ts ? egl_glx_ts->ps->id : driver_ps->ps->id), \
                        (egl_glx_ts ? egl_glx_ts->id : 0), \
                        func, format,##__VA_ARGS__)

#define YAGL_EGL_GLX_GET_PROC(proc_type, proc_name) \
    do { \
        egl_glx->proc_name = \
            (proc_type)egl_glx->glXGetProcAddress((const GLubyte*)#proc_name); \
        if (!egl_glx->proc_name) { \
            YAGL_LOG_ERROR("Unable to get symbol: %s", \
                           yagl_dyn_lib_get_error(egl_glx->dyn_lib)); \
            goto fail2; \
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

static YAGL_DEFINE_TLS(struct yagl_thread_state*, egl_glx_ts);

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

struct yagl_egl_glx_ps
{
    struct yagl_egl_driver_ps base;
};

struct yagl_egl_glx
{
    struct yagl_egl_driver base;

    struct yagl_dyn_lib *dyn_lib;

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
};

/*
 * INTERNAL IMPLEMENTATION FUNCTIONS
 * @{
 */

static bool yagl_egl_glx_config_fill(struct yagl_egl_driver_ps *driver_ps,
                                     EGLNativeDisplayType dpy,
                                     struct yagl_egl_native_config *cfg,
                                     GLXFBConfig glx_cfg)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);
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
    cfg->max_swap_interval = 10;
    cfg->min_swap_interval = 1;
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

static EGLNativeDisplayType yagl_egl_glx_display_create(struct yagl_egl_driver_ps *driver_ps)
{
    EGLNativeDisplayType dpy;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_display_create, NULL);

    dpy = XOpenDisplay(0);

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}

static void yagl_egl_glx_display_destroy(struct yagl_egl_driver_ps *driver_ps,
                                         EGLNativeDisplayType dpy)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_display_destroy, "%p", dpy);

    /*
     * Nvidia X driver workaround: We need to make at least one GLX call
     * before XCloseDisplay or nvidia X driver might crash our app. This
     * is happening when there were no GLX calls on a thread and the thread is
     * calling XCloseDisplay. From X point of view this is perfectly legal, but
     * nvidia seems to maintain some sort of per-thread state in TLS and
     * crashes.
     */
    egl_glx->glXQueryVersion(dpy, 0, 0);

    XCloseDisplay(dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
    *yagl_egl_glx_config_enum(struct yagl_egl_driver_ps *driver_ps,
                              EGLNativeDisplayType dpy,
                              int *num_configs)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);
    struct yagl_egl_native_config *configs = NULL;
    int glx_cfg_index, cfg_index, n;
    GLXFBConfig *glx_configs;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_config_enum, "%p", dpy);

    *num_configs = 0;

    glx_configs = egl_glx->glXGetFBConfigs(dpy, DefaultScreen(dpy), &n);

    YAGL_LOG_TRACE("got %d configs", n);

    configs = g_malloc0(n * sizeof(*configs));

    for (glx_cfg_index = 0, cfg_index = 0; glx_cfg_index < n; ++glx_cfg_index) {
        if (yagl_egl_glx_config_fill(driver_ps,
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

static void yagl_egl_glx_config_cleanup(struct yagl_egl_driver_ps *driver_ps,
                                        EGLNativeDisplayType dpy,
                                        struct yagl_egl_native_config *cfg)
{
    YAGL_EGL_GLX_ENTER(yagl_egl_glx_config_cleanup,
                       "dpy = %p, cfg = %d",
                       dpy,
                       cfg->config_id);

    cfg->driver_data = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLSurface yagl_egl_glx_pbuffer_surface_create(struct yagl_egl_driver_ps *driver_ps,
                                                      EGLNativeDisplayType dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      EGLint width,
                                                      EGLint height,
                                                      const struct yagl_egl_pbuffer_attribs *attribs)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);
    int glx_attribs[] = {
        GLX_PBUFFER_WIDTH, width,
        GLX_PBUFFER_HEIGHT, height,
        GLX_LARGEST_PBUFFER, (attribs->largest ? True : False),
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

static void yagl_egl_glx_pbuffer_surface_destroy(struct yagl_egl_driver_ps *driver_ps,
                                                 EGLNativeDisplayType dpy,
                                                 EGLSurface sfc)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_pbuffer_surface_destroy,
                       "dpy = %p, sfc = %p",
                       dpy,
                       sfc);

    egl_glx->glXDestroyPbuffer(dpy, (GLXPbuffer)sfc);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLContext yagl_egl_glx_context_create(struct yagl_egl_driver_ps *driver_ps,
                                              EGLNativeDisplayType dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              yagl_client_api client_api,
                                              EGLContext share_context)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);
    GLXContext ctx;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_context_create,
                       "dpy = %p, client_api = %u, share_context = %p",
                       dpy,
                       client_api,
                       share_context);

    ctx = egl_glx->glXCreateNewContext(dpy,
                                       (GLXFBConfig)cfg->driver_data,
                                       GLX_RGBA_TYPE,
                                       ((share_context == EGL_NO_CONTEXT) ?
                                           NULL
                                         : (GLXContext)share_context),
                                       True);

    if (!ctx) {
        YAGL_LOG_ERROR("glXCreateNewContext failed");

        YAGL_LOG_FUNC_EXIT(NULL);

        return EGL_NO_CONTEXT;
    } else {
        YAGL_LOG_FUNC_EXIT("%p", (EGLContext)ctx);

        return (EGLContext)ctx;
    }
}

static void yagl_egl_glx_context_destroy(struct yagl_egl_driver_ps *driver_ps,
                                         EGLNativeDisplayType dpy,
                                         EGLContext ctx)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_context_destroy,
                       "dpy = %p, ctx = %p",
                       dpy,
                       ctx);

    egl_glx->glXDestroyContext(dpy, (GLXContext)ctx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static bool yagl_egl_glx_make_current(struct yagl_egl_driver_ps *driver_ps,
                                      EGLNativeDisplayType dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);
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

static void yagl_egl_glx_wait_native(struct yagl_egl_driver_ps *driver_ps)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)(driver_ps->driver);

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_wait_native, NULL);

    egl_glx->glXWaitX();

    YAGL_LOG_FUNC_EXIT(NULL);
}

/*
 * @}
 */

static void yagl_egl_glx_thread_init(struct yagl_egl_driver_ps *driver_ps,
                                     struct yagl_thread_state *ts)
{
    egl_glx_ts = ts;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_thread_init, NULL);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_glx_thread_fini(struct yagl_egl_driver_ps *driver_ps)
{
    YAGL_EGL_GLX_ENTER(yagl_egl_glx_thread_fini, NULL);

    egl_glx_ts = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_egl_glx_process_destroy(struct yagl_egl_driver_ps *driver_ps)
{
    struct yagl_egl_glx_ps *egl_glx_ps = (struct yagl_egl_glx_ps*)driver_ps;

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_process_destroy, NULL);

    yagl_egl_driver_ps_cleanup(&egl_glx_ps->base);

    g_free(egl_glx_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_driver_ps
    *yagl_egl_glx_process_init(struct yagl_egl_driver *driver,
                               struct yagl_process_state *ps)
{
    struct yagl_egl_glx_ps *egl_glx_ps =
        g_malloc0(sizeof(struct yagl_egl_glx_ps));
    struct yagl_egl_driver_ps *driver_ps = &egl_glx_ps->base;

    yagl_egl_driver_ps_init(driver_ps, driver, ps);

    YAGL_EGL_GLX_ENTER(yagl_egl_glx_process_init, NULL);

    egl_glx_ps->base.thread_init = &yagl_egl_glx_thread_init;
    egl_glx_ps->base.display_create = &yagl_egl_glx_display_create;
    egl_glx_ps->base.display_destroy = &yagl_egl_glx_display_destroy;
    egl_glx_ps->base.config_enum = &yagl_egl_glx_config_enum;
    egl_glx_ps->base.config_cleanup = &yagl_egl_glx_config_cleanup;
    egl_glx_ps->base.pbuffer_surface_create = &yagl_egl_glx_pbuffer_surface_create;
    egl_glx_ps->base.pbuffer_surface_destroy = &yagl_egl_glx_pbuffer_surface_destroy;
    egl_glx_ps->base.context_create = &yagl_egl_glx_context_create;
    egl_glx_ps->base.context_destroy = &yagl_egl_glx_context_destroy;
    egl_glx_ps->base.make_current = &yagl_egl_glx_make_current;
    egl_glx_ps->base.wait_native = &yagl_egl_glx_wait_native;
    egl_glx_ps->base.thread_fini = &yagl_egl_glx_thread_fini;
    egl_glx_ps->base.destroy = &yagl_egl_glx_process_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_glx_ps->base;
}

static void yagl_egl_glx_destroy(struct yagl_egl_driver *driver)
{
    struct yagl_egl_glx *egl_glx = (struct yagl_egl_glx*)driver;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_egl_glx_destroy, NULL);

    yagl_dyn_lib_destroy(egl_glx->dyn_lib);

    yagl_egl_driver_cleanup(&egl_glx->base);

    g_free(egl_glx);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_driver *yagl_egl_glx_create(void)
{
    struct yagl_egl_glx *egl_glx;

    YAGL_LOG_FUNC_ENTER_NPT(yagl_egl_glx_create, NULL);

    egl_glx = g_malloc0(sizeof(*egl_glx));

    yagl_egl_driver_init(&egl_glx->base);

    egl_glx->dyn_lib = yagl_dyn_lib_create();

    if (!egl_glx->dyn_lib) {
        goto fail1;
    }

    if (!yagl_dyn_lib_load(egl_glx->dyn_lib, "libGL.so.1")) {
        YAGL_LOG_ERROR("Unable to load libGL.so.1: %s",
                       yagl_dyn_lib_get_error(egl_glx->dyn_lib));
        goto fail2;
    }

    egl_glx->glXGetProcAddress =
        yagl_dyn_lib_get_sym(egl_glx->dyn_lib, "glXGetProcAddress");

    if (!egl_glx->glXGetProcAddress) {
        egl_glx->glXGetProcAddress =
            yagl_dyn_lib_get_sym(egl_glx->dyn_lib, "glXGetProcAddressARB");
    }

    if (!egl_glx->glXGetProcAddress) {
        YAGL_LOG_ERROR("Unable to get symbol: %s",
                       yagl_dyn_lib_get_error(egl_glx->dyn_lib));
        goto fail2;
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
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEWINDOWPROC, glXCreateWindow);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYWINDOWPROC, glXDestroyWindow);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEPIXMAPPROC, glXCreatePixmap);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYPIXMAPPROC, glXDestroyPixmap);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATEPBUFFERPROC, glXCreatePbuffer);
    YAGL_EGL_GLX_GET_PROC(PFNGLXDESTROYPBUFFERPROC, glXDestroyPbuffer);
    YAGL_EGL_GLX_GET_PROC(PFNGLXCREATENEWCONTEXTPROC, glXCreateNewContext);
    YAGL_EGL_GLX_GET_PROC(PFNGLXMAKECONTEXTCURRENTPROC, glXMakeContextCurrent);

    egl_glx->base.process_init = &yagl_egl_glx_process_init;
    egl_glx->base.destroy = &yagl_egl_glx_destroy;

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_glx->base;

fail2:
    yagl_dyn_lib_destroy(egl_glx->dyn_lib);
fail1:
    yagl_egl_driver_cleanup(&egl_glx->base);
    g_free(egl_glx);

    YAGL_LOG_FUNC_EXIT(NULL);

    return NULL;
}
