#include <OpenGL/OpenGL.h>
#include <AGL/agl.h>
#include <glib.h>
#include "yagl_egl_driver.h"
#include "yagl_dyn_lib.h"
#include "yagl_log.h"
#include "yagl_egl_native_config.h"
#include "yagl_egl_surface_attribs.h"
#include "yagl_process.h"
#include "yagl_tls.h"
#include "yagl_thread.h"

#define LIBGL_IMAGE_NAME \
"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#define YAGL_LOG_AGL_STATUS() \
    YAGL_LOG_ERROR("AGL status: %s", aglErrorString(aglGetError()))

typedef struct YaglEglAglContext {
    AGLContext context;
} YaglEglAglContext;

typedef struct YaglEglAglDriver {
    struct yagl_egl_driver base;
} YaglEglAglDriver;

static EGLNativeDisplayType
yagl_egl_agl_display_open(struct yagl_egl_driver *driver)
{
    void *dpy = (void *)0x1;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_display_open, NULL);

    YAGL_LOG_FUNC_EXIT("Display created: %p", dpy);

    return (EGLNativeDisplayType) dpy;
}

static void yagl_egl_agl_display_close(struct yagl_egl_driver *driver,
                                       EGLNativeDisplayType egl_dpy)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_display_close, "%p", egl_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_egl_native_config
*yagl_egl_agl_config_enum(struct yagl_egl_driver *driver,
                          EGLNativeDisplayType egl_dpy, int *num_configs)
{
    struct yagl_egl_native_config *egl_configs = NULL;
    struct yagl_egl_native_config *cur_config;
    int usable_configs = 0;

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

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_config_enum, "display %p", egl_dpy);

    /* Enumerate AGL pixel formats matching our constraints */

    pixfmt = aglChoosePixelFormat(NULL, 0, attrib_list);

    while (pixfmt) {
        egl_configs = g_renew(struct yagl_egl_native_config,
                              egl_configs, usable_configs + 1);

        cur_config = &egl_configs[usable_configs];

        /* Initialize fields */
        yagl_egl_native_config_init(cur_config);

        cur_config->transparent_type = EGL_NONE;
        cur_config->config_id = usable_configs;

        aglDescribePixelFormat(pixfmt, AGL_BUFFER_SIZE,
                               &cur_config->buffer_size);
        aglDescribePixelFormat(pixfmt, AGL_RED_SIZE, &cur_config->red_size);
        aglDescribePixelFormat(pixfmt, AGL_GREEN_SIZE, &cur_config->green_size);
        aglDescribePixelFormat(pixfmt, AGL_BLUE_SIZE, &cur_config->blue_size);
        aglDescribePixelFormat(pixfmt, AGL_ALPHA_SIZE, &cur_config->alpha_size);
        aglDescribePixelFormat(pixfmt, AGL_DEPTH_SIZE, &cur_config->depth_size);
        aglDescribePixelFormat(pixfmt, AGL_STENCIL_SIZE,
                               &cur_config->stencil_size);

        cur_config->max_pbuffer_width = 4096;
        cur_config->max_pbuffer_height = 4096;
        cur_config->max_pbuffer_size = 4096 * 4096;
        cur_config->native_visual_type = EGL_NONE;
        cur_config->native_visual_id = 0;
        cur_config->caveat = EGL_NONE;
        cur_config->frame_buffer_level = 0;
        cur_config->samples_per_pixel = 0;
        cur_config->max_swap_interval = 1000;
        cur_config->min_swap_interval = 0;

        cur_config->driver_data = (void *)pixfmt;

        usable_configs++;

        pixfmt = aglNextPixelFormat(pixfmt);
    }

    YAGL_LOG_FUNC_EXIT("Enumerated %d configs", usable_configs);

    /* It's up to the caller to call config_cleanup on each
       of the returned entries, as well as call g_free on the array */

    *num_configs = usable_configs;
    return egl_configs;
}

static void yagl_egl_agl_config_cleanup(struct yagl_egl_driver *driver,
                                        EGLNativeDisplayType egl_dpy,
                                        const struct yagl_egl_native_config
                                        *cfg)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_config_cleanup,
                        "dpy = %p, cfg = %d", egl_dpy, cfg->config_id);

    aglDestroyPixelFormat(cfg->driver_data);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static EGLContext yagl_egl_agl_context_create(struct yagl_egl_driver *driver,
                                              EGLNativeDisplayType egl_dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              EGLContext share_context)
{
    YaglEglAglContext *egl_glc;
    AGLContext agl_share_glc;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_context_create,
                       "dpy = %p, share_context = %p, cfgid=%d",
                       egl_dpy, share_context, cfg->config_id);

    egl_glc = g_new0(YaglEglAglContext, 1);

    if (share_context != EGL_NO_CONTEXT) {
        agl_share_glc = ((YaglEglAglContext *) share_context)->context;
    } else
        agl_share_glc = NULL;

    egl_glc->context = aglCreateContext(cfg->driver_data, agl_share_glc);

    if (!egl_glc->context)
        goto fail;

    YAGL_LOG_FUNC_EXIT("Context created: %p", egl_glc);

    return (EGLContext) egl_glc;

 fail:
    g_free(egl_glc);

    YAGL_LOG_AGL_STATUS();
    YAGL_LOG_FUNC_EXIT("Failed to create new context");

    return EGL_NO_CONTEXT;
}

static bool yagl_egl_agl_make_current(struct yagl_egl_driver *driver,
                                      EGLNativeDisplayType egl_dpy,
                                      EGLSurface egl_draw_surf,
                                      EGLSurface egl_read_surf,
                                      EGLContext egl_glc)
{
    AGLContext context = NULL;
    AGLPbuffer draw_buf = NULL;
    AGLPbuffer read_buf = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_make_current,
                        "dpy = %p, draw = %p, read = %p, ctx = %p",
                        egl_dpy, egl_draw_surf, egl_read_surf, egl_glc);

    if (egl_glc != EGL_NO_CONTEXT) {
        context = ((YaglEglAglContext *) egl_glc)->context;
    }

    if (egl_read_surf != EGL_NO_SURFACE) {
        read_buf = (AGLPbuffer) egl_read_surf;

        if (aglSetPBuffer(context, read_buf, 0, 0, 0) == GL_FALSE) {
            goto fail;
        }
    }

    if (egl_draw_surf != EGL_NO_SURFACE) {
        draw_buf = (AGLPbuffer) egl_draw_surf;

        if (aglSetPBuffer(context, draw_buf, 0, 0, 0) == GL_FALSE) {
            goto fail;
        }
    }

    if (aglSetCurrentContext(context) == GL_FALSE) {
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("context %p was made current", context);

    return true;

 fail:
    YAGL_LOG_AGL_STATUS();

    YAGL_LOG_FUNC_EXIT("Failed to make context %p current", context);

    return false;
}

static void yagl_egl_agl_context_destroy(struct yagl_egl_driver *driver,
                                         EGLNativeDisplayType egl_dpy,
                                         EGLContext egl_glc)
{
    AGLContext context;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_context_destroy,
                        "dpy = %p, ctx = %p", egl_dpy, egl_glc);

    if (egl_glc != EGL_NO_CONTEXT) {
        context = ((YaglEglAglContext *) egl_glc)->context;

        if (aglDestroyContext(context) == GL_TRUE) {
            g_free(egl_glc);
            YAGL_LOG_FUNC_EXIT("Context destroyed");
            return;
        }

        g_free(egl_glc);
        YAGL_LOG_AGL_STATUS();
    }

    YAGL_LOG_FUNC_EXIT("Could not destroy context");
}

static EGLSurface yagl_egl_agl_pbuffer_surface_create(struct yagl_egl_driver
                                                      *driver,
                                                      EGLNativeDisplayType
                                                      egl_dpy,
                                                      const struct
                                                      yagl_egl_native_config
                                                      *cfg, EGLint width,
                                                      EGLint height,
                                                      const struct
                                                      yagl_egl_pbuffer_attribs
                                                      *attribs)
{
    AGLPbuffer pbuffer = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_pbuffer_surface_create,
                        "dpy = %p, width = %d, height = %d, cfgid=%d",
                        egl_dpy, width, height, cfg->config_id);

    if (aglCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGBA, 0, &pbuffer) ==
        GL_FALSE)
        goto fail;

    YAGL_LOG_FUNC_EXIT("Surface created: %p", pbuffer);

    return (EGLSurface) pbuffer;

 fail:
    YAGL_LOG_AGL_STATUS();
    YAGL_LOG_FUNC_EXIT("Surface creation failed");

    return EGL_NO_SURFACE;
}

static void yagl_egl_agl_pbuffer_surface_destroy(struct yagl_egl_driver *driver,
                                                 EGLNativeDisplayType egl_dpy,
                                                 EGLSurface surf)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_pbuffer_surface_destroy,
                        "dpy = %p, sfc = %p", egl_dpy, (AGLPbuffer) surf);

    if (aglDestroyPBuffer((AGLPbuffer) surf) == GL_FALSE) {
        YAGL_LOG_AGL_STATUS();
        YAGL_LOG_FUNC_EXIT("Failed to destroy surface");
    } else {
        YAGL_LOG_FUNC_EXIT("Surface destroyed");
    }
}

static void yagl_egl_agl_destroy(struct yagl_egl_driver *driver)
{
    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_destroy, NULL);

    yagl_egl_driver_cleanup(driver);

    g_free(driver);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_driver *yagl_egl_driver_create(void *display)
{
    YaglEglAglDriver *egl_agl;
    struct yagl_egl_driver *egl_driver;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_create, NULL);

    egl_agl = g_try_new0(YaglEglAglDriver, 1);

    if (!egl_agl)
        goto inconceivable;

    egl_driver = &egl_agl->base;

    /* Initialize portable YaGL machinery */
    yagl_egl_driver_init(egl_driver);

    egl_driver->display_open = &yagl_egl_agl_display_open;
    egl_driver->display_close = &yagl_egl_agl_display_close;
    egl_driver->config_enum = &yagl_egl_agl_config_enum;
    egl_driver->config_cleanup = &yagl_egl_agl_config_cleanup;
    egl_driver->pbuffer_surface_create = &yagl_egl_agl_pbuffer_surface_create;
    egl_driver->pbuffer_surface_destroy = &yagl_egl_agl_pbuffer_surface_destroy;
    egl_driver->context_create = &yagl_egl_agl_context_create;
    egl_driver->context_destroy = &yagl_egl_agl_context_destroy;
    egl_driver->make_current = &yagl_egl_agl_make_current;
    egl_driver->destroy = &yagl_egl_agl_destroy;

    egl_driver->dyn_lib = yagl_dyn_lib_create();

    if (!yagl_dyn_lib_load(egl_driver->dyn_lib, LIBGL_IMAGE_NAME)) {
        YAGL_LOG_ERROR("Loading %s failed with error: %s",
                       LIBGL_IMAGE_NAME,
                       yagl_dyn_lib_get_error(egl_driver->dyn_lib));
        goto fail;
    }

    YAGL_LOG_FUNC_EXIT("EGL AGL driver created (%p)", egl_driver);

    return egl_driver;

 fail:
    yagl_egl_driver_cleanup(egl_driver);
    g_free(egl_agl);

 inconceivable:
    YAGL_LOG_AGL_STATUS();
    YAGL_LOG_FUNC_EXIT("EGL_AGL driver creation failed");

    return NULL;
}

void *yagl_dyn_lib_get_ogl_procaddr(struct yagl_dyn_lib *dyn_lib,
                                    const char *sym)
{
    void *proc;

    YAGL_LOG_FUNC_ENTER(yagl_egl_agl_get_procaddr,
                        "Retrieving %s address", sym);

    /* The dlsym code path is shared by Linux and Mac builds */
    proc = yagl_dyn_lib_get_sym(dyn_lib, sym);

    YAGL_LOG_FUNC_EXIT("%s address: %p", sym, proc);

    return proc;
}
