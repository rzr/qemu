#include "yagl_host_egl_calls.h"
#include "yagl_egl_calls.h"
#include "yagl_egl_api_ps.h"
#include "yagl_egl_api_ts.h"
#include "yagl_egl_api.h"
#include "yagl_egl_display.h"
#include "yagl_egl_backend.h"
#include "yagl_egl_interface.h"
#include "yagl_egl_config.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_context.h"
#include "yagl_egl_image.h"
#include "yagl_egl_validate.h"
#include "yagl_eglb_display.h"
#include "yagl_eglb_context.h"
#include "yagl_eglb_surface.h"
#include "yagl_eglb_image.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_mem_egl.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_client_interface.h"
#include "yagl_client_context.h"
#include "yagl_client_image.h"
#include "yagl_sharegroup.h"
#include <EGL/eglext.h>

#define YAGL_EGL_VERSION_MAJOR 1
#define YAGL_EGL_VERSION_MINOR 4

#define YAGL_SET_ERR(err) \
    if (egl_api_ts->error == EGL_SUCCESS) { \
        egl_api_ts->error = err; \
    } \
    YAGL_LOG_ERROR("error = 0x%X", err)

#define YAGL_UNIMPLEMENTED(func, ret) \
    YAGL_LOG_FUNC_SET(func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    *retval = ret; \
    return true;

struct yagl_egl_interface_impl
{
    struct yagl_egl_interface base;

    struct yagl_egl_backend *backend;
};

static YAGL_DEFINE_TLS(struct yagl_egl_api_ts*, egl_api_ts);

static __inline bool yagl_validate_display(yagl_host_handle dpy_,
                                           struct yagl_egl_display **dpy)
{
    YAGL_LOG_FUNC_SET(yagl_validate_display);

    *dpy = yagl_egl_api_ps_display_get(egl_api_ts->api_ps, dpy_);

    if (!*dpy) {
        YAGL_SET_ERR(EGL_BAD_DISPLAY);
        return false;
    }

    if (!yagl_egl_display_is_initialized(*dpy)) {
        YAGL_SET_ERR(EGL_NOT_INITIALIZED);
        return false;
    }

    return true;
}

static __inline bool yagl_validate_config(struct yagl_egl_display *dpy,
                                          yagl_host_handle cfg_,
                                          struct yagl_egl_config **cfg)
{
    YAGL_LOG_FUNC_SET(yagl_validate_config);

    *cfg = yagl_egl_display_acquire_config(dpy, cfg_);

    if (!*cfg) {
        YAGL_SET_ERR(EGL_BAD_CONFIG);
        return false;
    }

    return true;
}

static __inline bool yagl_validate_surface(struct yagl_egl_display *dpy,
                                           yagl_host_handle sfc_,
                                           struct yagl_egl_surface **sfc)
{
    YAGL_LOG_FUNC_SET(yagl_validate_surface);

    *sfc = yagl_egl_display_acquire_surface(dpy, sfc_);

    if (!*sfc) {
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        return false;
    }

    return true;
}

static __inline bool yagl_validate_context(struct yagl_egl_display *dpy,
                                           yagl_host_handle ctx_,
                                           struct yagl_egl_context **ctx)
{
    YAGL_LOG_FUNC_SET(yagl_validate_context);

    *ctx = yagl_egl_display_acquire_context(dpy, ctx_);

    if (!*ctx) {
        YAGL_SET_ERR(EGL_BAD_CONTEXT);
        return false;
    }

    return true;
}

static __inline bool yagl_validate_image(struct yagl_egl_display *dpy,
                                         yagl_host_handle image_,
                                         struct yagl_egl_image **image)
{
    YAGL_LOG_FUNC_SET(yagl_validate_image);

    *image = yagl_egl_display_acquire_image(dpy, image_);

    if (!*image) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        return false;
    }

    return true;
}

static bool yagl_get_client_api(const EGLint *attrib_list, yagl_client_api *client_api)
{
    int i = 0;

    switch (egl_api_ts->api) {
    case EGL_OPENGL_ES_API:
        *client_api = yagl_client_api_gles1;
        if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
            while (attrib_list[i] != EGL_NONE) {
                switch (attrib_list[i]) {
                case EGL_CONTEXT_CLIENT_VERSION:
                    if (attrib_list[i + 1] == 1) {
                        *client_api = yagl_client_api_gles1;
                    } else if (attrib_list[i + 1] == 2) {
                        *client_api = yagl_client_api_gles2;
                    } else {
                        return false;
                    }
                    break;
                default:
                    return false;
                }

                i += 2;
            }
        }
        break;
    case EGL_OPENVG_API:
        *client_api = yagl_client_api_ovg;
        break;
    case EGL_OPENGL_API:
        *client_api = yagl_client_api_ogl;
        break;
    default:
        return false;
    }
    return true;
}

static struct yagl_client_context *yagl_egl_get_ctx(struct yagl_egl_interface *iface)
{
    if (egl_api_ts->context) {
        return egl_api_ts->context->backend_ctx->client_ctx;
    } else {
        return NULL;
    }
}

static struct yagl_client_image *yagl_egl_get_image(struct yagl_egl_interface *iface,
                                                    yagl_host_handle image)
{
    if (egl_api_ts->context) {
        struct yagl_egl_image *egl_image =
            yagl_egl_display_acquire_image(egl_api_ts->context->dpy, image);
        if (egl_image) {
            struct yagl_client_image *client_image =
                egl_image->backend_image->glegl_image;
            yagl_client_image_acquire(client_image);
            yagl_egl_image_release(egl_image);
            return client_image;
        }
    }

    return NULL;
}

static void yagl_egl_ensure_ctx(struct yagl_egl_interface *iface)
{
    struct yagl_egl_interface_impl *egl_iface = (struct yagl_egl_interface_impl*)iface;
    egl_iface->backend->ensure_current(egl_iface->backend);
}

static void yagl_egl_unensure_ctx(struct yagl_egl_interface *iface)
{
    struct yagl_egl_interface_impl *egl_iface = (struct yagl_egl_interface_impl*)iface;
    egl_iface->backend->unensure_current(egl_iface->backend);
}

static bool yagl_egl_release_current_context(struct yagl_egl_display *dpy)
{
    if (!egl_api_ts->context) {
        return true;
    }

    egl_api_ts->context->backend_ctx->client_ctx->flush(egl_api_ts->context->backend_ctx->client_ctx);
    egl_api_ts->context->backend_ctx->client_ctx->deactivate(egl_api_ts->context->backend_ctx->client_ctx);

    if (!egl_api_ts->backend->release_current(egl_api_ts->backend, false)) {
        /*
         * If host 'make_current' failed then re-activate.
         */
        egl_api_ts->context->backend_ctx->client_ctx->activate(egl_api_ts->context->backend_ctx->client_ctx);
        return false;
    }

    yagl_egl_context_update_surfaces(egl_api_ts->context, NULL, NULL);

    yagl_egl_api_ts_update_context(egl_api_ts, NULL);

    return true;
}

static yagl_api_func yagl_host_egl_get_func(struct yagl_api_ps *api_ps,
                                            uint32_t func_id)
{
    if ((func_id <= 0) || (func_id > yagl_egl_api_num_funcs)) {
        return NULL;
    } else {
        return yagl_egl_api_funcs[func_id - 1];
    }
}

static void yagl_host_egl_thread_init(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_thread_init, NULL);

    egl_api_ps->backend->thread_init(egl_api_ps->backend);

    egl_api_ts = g_malloc0(sizeof(*egl_api_ts));

    yagl_egl_api_ts_init(egl_api_ts, egl_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_egl_thread_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_thread_fini, NULL);

    yagl_egl_api_ts_cleanup(egl_api_ts);

    g_free(egl_api_ts);

    egl_api_ts = NULL;

    egl_api_ps->backend->thread_fini(egl_api_ps->backend);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_egl_process_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    yagl_egl_api_ps_fini(egl_api_ps);
}

static void yagl_host_egl_process_destroy(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;
    struct yagl_egl_interface_impl *egl_iface = (struct yagl_egl_interface_impl*)egl_api_ps->egl_iface;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_process_destroy, NULL);

    yagl_egl_api_ps_cleanup(egl_api_ps);

    yagl_egl_interface_cleanup(&egl_iface->base);
    g_free(egl_iface);

    yagl_api_ps_cleanup(&egl_api_ps->base);

    g_free(egl_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_api_ps *yagl_host_egl_process_init(struct yagl_api *api)
{
    struct yagl_egl_api *egl_api = (struct yagl_egl_api*)api;
    struct yagl_egl_interface_impl *egl_iface;
    struct yagl_egl_api_ps *egl_api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_process_init, NULL);

    /*
     * Create EGL interface.
     */

    egl_iface = g_malloc0(sizeof(*egl_iface));

    yagl_egl_interface_init(&egl_iface->base, egl_api->backend->render_type);

    egl_iface->base.get_ctx = &yagl_egl_get_ctx;
    egl_iface->base.get_image = &yagl_egl_get_image;
    egl_iface->base.ensure_ctx = &yagl_egl_ensure_ctx;
    egl_iface->base.unensure_ctx = &yagl_egl_unensure_ctx;
    egl_iface->backend = egl_api->backend;

    /*
     * Finally, create API ps.
     */

    egl_api_ps = g_malloc0(sizeof(*egl_api_ps));

    yagl_api_ps_init(&egl_api_ps->base, api);

    egl_api_ps->base.thread_init = &yagl_host_egl_thread_init;
    egl_api_ps->base.get_func = &yagl_host_egl_get_func;
    egl_api_ps->base.thread_fini = &yagl_host_egl_thread_fini;
    egl_api_ps->base.fini = &yagl_host_egl_process_fini;
    egl_api_ps->base.destroy = &yagl_host_egl_process_destroy;

    yagl_egl_api_ps_init(egl_api_ps, egl_api->backend, &egl_iface->base);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_api_ps->base;
}

bool yagl_host_eglGetError(EGLint* retval)
{
    *retval = egl_api_ts->error;

    egl_api_ts->error = EGL_SUCCESS;

    return true;
}

bool yagl_host_eglGetDisplay(yagl_host_handle* retval,
    target_ulong /* void* */ display_id)
{
    struct yagl_egl_display *dpy;

    dpy = yagl_egl_api_ps_display_add(egl_api_ts->api_ps, display_id);

    *retval = (dpy ? dpy->handle : 0);

    return true;
}

bool yagl_host_eglInitialize(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* EGLint* */ major,
    target_ulong /* EGLint* */ minor)
{
    struct yagl_egl_display *dpy;

    YAGL_LOG_FUNC_SET(eglInitialize);

    if (!yagl_mem_prepare_EGLint(cur_ts->mt1, major) ||
        !yagl_mem_prepare_EGLint(cur_ts->mt2, minor)) {
        return false;
    }

    dpy = yagl_egl_api_ps_display_get(egl_api_ts->api_ps, dpy_);

    if (!dpy) {
        YAGL_SET_ERR(EGL_BAD_DISPLAY);
        *retval = EGL_FALSE;
        return true;
    }

    yagl_egl_display_initialize(dpy);

    if (major) {
        yagl_mem_put_EGLint(cur_ts->mt1, YAGL_EGL_VERSION_MAJOR);
    }

    if (minor) {
        yagl_mem_put_EGLint(cur_ts->mt2, YAGL_EGL_VERSION_MINOR);
    }

    *retval = EGL_TRUE;
    return true;
}

bool yagl_host_eglTerminate(EGLBoolean* retval,
    yagl_host_handle dpy_)
{
    struct yagl_egl_display *dpy = NULL;

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    yagl_egl_display_terminate(dpy);

    *retval = EGL_TRUE;

out:
    return true;
}

bool yagl_host_eglGetConfigs(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    yagl_host_handle *configs = NULL;
    EGLint num_config = config_size;

    YAGL_LOG_FUNC_SET(eglGetConfigs);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!num_config_) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        goto out;
    }

    if (configs_) {
        configs = yagl_egl_display_get_config_handles(dpy, &num_config);
    } else {
        num_config = yagl_egl_display_get_config_count(dpy);
    }

    if (!yagl_mem_prepare(cur_ts->mt1, configs_, num_config * sizeof(*configs)) ||
        !yagl_mem_prepare_EGLint(cur_ts->mt2, num_config_)) {
        res = false;
        goto out;
    }

    if (configs_) {
        yagl_mem_put(cur_ts->mt1, configs);
    }

    yagl_mem_put_EGLint(cur_ts->mt2, num_config);

    *retval = EGL_TRUE;

out:
    g_free(configs);

    return res;
}

bool yagl_host_eglChooseConfig(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* const EGLint* */ attrib_list_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    EGLint *attrib_list = NULL;
    struct yagl_egl_native_config dummy;
    yagl_host_handle *configs = NULL;
    EGLint num_config = config_size;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglChooseConfig);

    *retval = EGL_FALSE;

    yagl_egl_native_config_init(&dummy);

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!num_config_) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        goto out;
    }

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    /*
     * Selection defaults.
     */

    dummy.surface_type = EGL_WINDOW_BIT;
    dummy.renderable_type = EGL_OPENGL_ES_BIT;
    dummy.caveat = EGL_DONT_CARE;
    dummy.config_id = EGL_DONT_CARE;
    dummy.native_renderable = EGL_DONT_CARE;
    dummy.native_visual_type = EGL_DONT_CARE;
    dummy.max_swap_interval = EGL_DONT_CARE;
    dummy.min_swap_interval = EGL_DONT_CARE;
    dummy.trans_red_val = EGL_DONT_CARE;
    dummy.trans_green_val = EGL_DONT_CARE;
    dummy.trans_blue_val = EGL_DONT_CARE;
    dummy.transparent_type = EGL_NONE;
    dummy.match_format_khr = EGL_DONT_CARE;
    dummy.bind_to_texture_rgb = EGL_DONT_CARE;
    dummy.bind_to_texture_rgba = EGL_DONT_CARE;

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        bool has_config_id = false;
        EGLint config_id = 0;

        while ((attrib_list[i] != EGL_NONE) && !has_config_id) {
            switch (attrib_list[i]) {
            case EGL_MAX_PBUFFER_WIDTH:
            case EGL_MAX_PBUFFER_HEIGHT:
            case EGL_MAX_PBUFFER_PIXELS:
            case EGL_NATIVE_VISUAL_ID:
                break;
            case EGL_BIND_TO_TEXTURE_RGB:
                dummy.bind_to_texture_rgb = attrib_list[i + 1];
                break;
            case EGL_BIND_TO_TEXTURE_RGBA:
                dummy.bind_to_texture_rgba = attrib_list[i + 1];
                break;
            case EGL_SURFACE_TYPE:
                dummy.surface_type = attrib_list[i + 1];
                break;
            case EGL_LEVEL:
                if (attrib_list[i + 1] == EGL_DONT_CARE) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.frame_buffer_level = attrib_list[i + 1];
                break;
            case EGL_BUFFER_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.buffer_size = attrib_list[i + 1];
                break;
            case EGL_RED_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.red_size = attrib_list[i + 1];
                break;
            case EGL_GREEN_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.green_size = attrib_list[i + 1];
                break;
            case EGL_BLUE_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.blue_size = attrib_list[i + 1];
                break;
            case EGL_ALPHA_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.alpha_size = attrib_list[i + 1];
                break;
            case EGL_CONFIG_CAVEAT:
                if ((attrib_list[i + 1] != EGL_NONE) &&
                    (attrib_list[i + 1] != EGL_SLOW_CONFIG) &&
                    (attrib_list[i + 1] != EGL_NON_CONFORMANT_CONFIG)) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.caveat = attrib_list[i + 1];
                break;
            case EGL_CONFIG_ID:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                config_id = attrib_list[i + 1];
                has_config_id = true;
                break;
            case EGL_DEPTH_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.depth_size = attrib_list[i + 1];
                break;
            case EGL_MAX_SWAP_INTERVAL:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.max_swap_interval = attrib_list[i + 1];
                break;
            case EGL_MIN_SWAP_INTERVAL:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.min_swap_interval = attrib_list[i + 1];
                break;
            case EGL_CONFORMANT:
                if ((attrib_list[i + 1] &
                    ~(EGL_OPENGL_ES_BIT|
                      EGL_OPENVG_BIT|
                      EGL_OPENGL_ES2_BIT|
                      EGL_OPENGL_BIT)) != 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.conformant = attrib_list[i + 1];
                break;
            case EGL_NATIVE_RENDERABLE:
                dummy.native_renderable = attrib_list[i + 1];
                break;
            case EGL_RENDERABLE_TYPE:
                dummy.renderable_type = attrib_list[i + 1];
                break;
            case EGL_NATIVE_VISUAL_TYPE:
                dummy.native_visual_type = attrib_list[i + 1];
                if ((attrib_list[i + 1] < 0) || (attrib_list[i + 1] > 1)) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                break;
            case EGL_SAMPLE_BUFFERS:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.sample_buffers_num = attrib_list[i + 1];
                break;
            case EGL_SAMPLES:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.samples_per_pixel = attrib_list[i + 1];
                break;
            case EGL_STENCIL_SIZE:
                if (attrib_list[i + 1] < 0) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.stencil_size = attrib_list[i + 1];
                break;
            case EGL_TRANSPARENT_TYPE:
                if ((attrib_list[i + 1] != EGL_NONE) &&
                    (attrib_list[i + 1] != EGL_TRANSPARENT_RGB)) {
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                dummy.transparent_type = attrib_list[i + 1];
                break;
            case EGL_TRANSPARENT_RED_VALUE:
                dummy.trans_red_val = attrib_list[i + 1];
                break;
            case EGL_TRANSPARENT_GREEN_VALUE:
                dummy.trans_green_val = attrib_list[i + 1];
                break;
            case EGL_TRANSPARENT_BLUE_VALUE:
                dummy.trans_blue_val = attrib_list[i + 1];
                break;
            case EGL_MATCH_FORMAT_KHR:
                dummy.match_format_khr = attrib_list[i + 1];
                break;
            default:
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }

            i += 2;
        }

        if (has_config_id) {
            yagl_host_handle handle = 0;
            struct yagl_egl_config *cfg =
                yagl_egl_display_acquire_config_by_id(dpy, config_id);

            if (cfg) {
                handle = cfg->res.handle;
                yagl_egl_config_release(cfg);
            }

            YAGL_LOG_DEBUG("requesting config with id = %d", config_id);

            if (handle) {
                if (!yagl_mem_prepare_host_handle(cur_ts->mt1, configs_) ||
                    !yagl_mem_prepare_EGLint(cur_ts->mt2, num_config_)) {
                    res = false;
                    goto out;
                }

                if (configs_) {
                    yagl_mem_put_host_handle(cur_ts->mt1, handle);
                }

                yagl_mem_put_EGLint(cur_ts->mt2, 1);

                *retval = EGL_TRUE;
                goto out;
            } else {
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }
        }
    }

    configs = yagl_egl_display_choose_configs(dpy,
                                              &dummy,
                                              &num_config,
                                              (configs_ == 0));

    YAGL_LOG_DEBUG("chosen %d configs", num_config);

    if (!yagl_mem_prepare(cur_ts->mt1, configs_, num_config * sizeof(*configs)) ||
        !yagl_mem_prepare_EGLint(cur_ts->mt2, num_config_)) {
        res = false;
        goto out;
    }

    if (configs_) {
        yagl_mem_put(cur_ts->mt1, configs);
    }

    yagl_mem_put_EGLint(cur_ts->mt2, num_config);

    *retval = EGL_TRUE;

out:
    g_free(configs);
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglGetConfigAttrib(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    EGLint value = 0;

    YAGL_LOG_FUNC_SET(eglGetConfigAttrib);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!yagl_egl_config_get_attrib(config, attribute, &value)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_mem_prepare_EGLint(cur_ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(cur_ts->mt1, value);
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_config_release(config);

    return res;
}

bool yagl_host_eglDestroySurface(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglDestroySurface);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!yagl_egl_display_remove_surface(dpy, surface->res.handle)) {
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglQuerySurface(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;
    EGLint value = 0;

    YAGL_LOG_FUNC_SET(eglQuerySurface);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    switch (attribute) {
    case EGL_CONFIG_ID:
        value = surface->cfg->native.config_id;
        break;
    case EGL_LARGEST_PBUFFER:
        if (surface->backend_sfc->type == EGL_PBUFFER_BIT) {
            value = surface->backend_sfc->attribs.pbuffer.largest;
        } else {
            /*
             * That's right 'value_', not 'value', when an attribute is not
             * applicable we shouldn't write back anything.
             */
            value_ = 0;
        }
        break;
    case EGL_TEXTURE_FORMAT:
        if (surface->backend_sfc->type == EGL_PBUFFER_BIT) {
            value = surface->backend_sfc->attribs.pbuffer.tex_format;
        } else {
            value_ = 0;
        }
        break;
    case EGL_TEXTURE_TARGET:
        if (surface->backend_sfc->type == EGL_PBUFFER_BIT) {
            value = surface->backend_sfc->attribs.pbuffer.tex_target;
        } else {
            value_ = 0;
        }
        break;
    case EGL_MIPMAP_TEXTURE:
        if (surface->backend_sfc->type == EGL_PBUFFER_BIT) {
            value = surface->backend_sfc->attribs.pbuffer.tex_mipmap;
        } else {
            value_ = 0;
        }
        break;
    case EGL_MIPMAP_LEVEL:
        if (surface->backend_sfc->type == EGL_PBUFFER_BIT) {
            value = 0;
        } else {
            value_ = 0;
        }
        break;
    case EGL_RENDER_BUFFER:
        switch (surface->backend_sfc->type) {
        case EGL_PBUFFER_BIT:
        case EGL_WINDOW_BIT:
            value = EGL_BACK_BUFFER;
            break;
        case EGL_PIXMAP_BIT:
            value = EGL_SINGLE_BUFFER;
            break;
        default:
            assert(0);
            value_ = 0;
        }
        break;
    case EGL_HORIZONTAL_RESOLUTION:
    case EGL_VERTICAL_RESOLUTION:
    case EGL_PIXEL_ASPECT_RATIO:
        value = EGL_UNKNOWN;
        break;
    case EGL_SWAP_BEHAVIOR:
        value = EGL_BUFFER_PRESERVED;
        break;
    case EGL_MULTISAMPLE_RESOLVE:
        value = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
        break;
    default:
        if (!surface->backend_sfc->query(surface->backend_sfc,
                                         attribute,
                                         &value)) {
            YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
            goto out;
        }
    }

    if (!yagl_mem_prepare_EGLint(cur_ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(cur_ts->mt1, value);
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return res;
}

bool yagl_host_eglBindAPI(EGLBoolean* retval,
    EGLenum api)
{
    YAGL_LOG_FUNC_SET(eglBindAPI);

    if (!yagl_egl_is_api_valid(api)) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        *retval = EGL_FALSE;
        return true;
    }

    egl_api_ts->api = api;

    *retval = EGL_TRUE;
    return true;
}

bool yagl_host_eglWaitClient(EGLBoolean* retval)
{
    struct yagl_egl_surface *sfc = NULL;

    *retval = EGL_TRUE;

    if (!egl_api_ts->context) {
        goto out;
    }

    sfc = egl_api_ts->context->draw;

    if (!sfc) {
        goto out;
    }

    sfc->backend_sfc->wait_gl(sfc->backend_sfc);

out:
    return true;
}

bool yagl_host_eglReleaseThread(EGLBoolean* retval)
{
    YAGL_LOG_FUNC_SET(eglReleaseThread);

    *retval = EGL_FALSE;

    if (egl_api_ts->context) {
        if (!yagl_egl_release_current_context(egl_api_ts->context->dpy)) {
            YAGL_SET_ERR(EGL_BAD_ACCESS);
            goto out;
        }
    }

    yagl_egl_api_ts_reset(egl_api_ts);

    *retval = EGL_TRUE;

out:
    return true;
}

bool yagl_host_eglCreatePbufferFromClientBuffer(yagl_host_handle* retval,
    yagl_host_handle dpy,
    EGLenum buftype,
    yagl_host_handle buffer,
    yagl_host_handle config,
    target_ulong /* const EGLint* */ attrib_list)
{
    YAGL_UNIMPLEMENTED(eglCreatePbufferFromClientBuffer, 0);
}

bool yagl_host_eglSurfaceAttrib(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    EGLint value)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    /*
     * TODO: implement.
     */

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglBindTexImage(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint buffer)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglBindTexImage);

    if (!egl_api_ts->context) {
        YAGL_LOG_WARN("No context");
        *retval = EGL_TRUE;
        goto out;
    }

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (buffer != EGL_BACK_BUFFER) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        goto out;
    }

    if (surface->backend_sfc->type != EGL_PBUFFER_BIT) {
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (surface->backend_sfc->attribs.pbuffer.tex_format == EGL_NO_TEXTURE) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    if (surface->backend_sfc->attribs.pbuffer.tex_target == EGL_NO_TEXTURE) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    if (!surface->backend_sfc->bind_tex_image(surface->backend_sfc)) {
        YAGL_SET_ERR(EGL_BAD_ACCESS);
        goto out;
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglReleaseTexImage(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint buffer)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglReleaseTexImage);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (buffer != EGL_BACK_BUFFER) {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        goto out;
    }

    if (surface->backend_sfc->type != EGL_PBUFFER_BIT) {
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (surface->backend_sfc->attribs.pbuffer.tex_format == EGL_NO_TEXTURE) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    if (surface->backend_sfc->attribs.pbuffer.tex_target == EGL_NO_TEXTURE) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    if (!surface->backend_sfc->release_tex_image(surface->backend_sfc)) {
        YAGL_SET_ERR(EGL_BAD_ACCESS);
        goto out;
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglCreateContext(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_host_handle share_context_,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    yagl_client_api client_api;
    struct yagl_client_interface *client_iface = NULL;
    struct yagl_egl_context *share_context = NULL;
    struct yagl_sharegroup *sg = NULL;
    struct yagl_client_context *client_ctx = NULL;
    struct yagl_egl_context *ctx = NULL;

    YAGL_LOG_FUNC_SET(eglCreateContext);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!yagl_get_client_api(attrib_list, &client_api)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    client_iface = cur_ts->ps->client_ifaces[client_api];

    if (!client_iface) {
        YAGL_LOG_ERROR("client API %d is not supported", client_api);
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (share_context_) {
        if (!yagl_validate_context(dpy, share_context_, &share_context)) {
            goto out;
        }
        sg = share_context->backend_ctx->client_ctx->sg;
        yagl_sharegroup_acquire(sg);
    } else {
        sg = yagl_sharegroup_create();
    }

    client_ctx = client_iface->create_ctx(client_iface, sg);

    if (!client_ctx) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    ctx = yagl_egl_context_create(dpy,
                                  config,
                                  client_ctx,
                                  (share_context ? share_context->backend_ctx
                                                 : NULL));

    if (!ctx) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    /*
     * Now owned by 'ctx'.
     */
    client_ctx = NULL;

    yagl_egl_display_add_context(dpy, ctx);
    yagl_egl_context_release(ctx);

    *retval = ctx->res.handle;

out:
    if (client_ctx) {
        client_ctx->destroy(client_ctx);
    }
    yagl_sharegroup_release(sg);
    yagl_egl_context_release(share_context);
    yagl_egl_config_release(config);
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglDestroyContext(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle ctx_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *ctx = NULL;

    YAGL_LOG_FUNC_SET(eglDestroyContext);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_context(dpy, ctx_, &ctx)) {
        goto out;
    }

    if (yagl_egl_display_remove_context(dpy, ctx->res.handle)) {
        *retval = EGL_TRUE;
    } else {
        YAGL_SET_ERR(EGL_BAD_CONTEXT);
    }

out:
    yagl_egl_context_release(ctx);

    return true;
}

bool yagl_host_eglMakeCurrent(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle draw_,
    yagl_host_handle read_,
    yagl_host_handle ctx_)
{
    bool bad_match = ctx_ ? (!draw_ || !read_) : (draw_ || read_);
    bool release_context = !draw_ && !read_ && !ctx_;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *prev_ctx = NULL;
    struct yagl_egl_context *ctx = NULL;
    struct yagl_egl_surface *draw = NULL;
    struct yagl_egl_surface *read = NULL;

    YAGL_LOG_FUNC_SET(eglMakeCurrent);

    *retval = EGL_FALSE;

    if (bad_match) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    prev_ctx = egl_api_ts->context;
    yagl_egl_context_acquire(prev_ctx);

    if (release_context) {
        if (prev_ctx) {
            if (!dpy_) {
                /*
                 * Workaround for the case when dpy
                 * passed is EGL_NO_DISPLAY and we're releasing
                 * the current context.
                 */

                dpy_ = prev_ctx->dpy->handle;
                dpy = prev_ctx->dpy;
            } else {
                dpy = yagl_egl_api_ps_display_get(egl_api_ts->api_ps, dpy_);
                if (!dpy) {
                    YAGL_SET_ERR(EGL_BAD_DISPLAY);
                    goto out;
                }
            }
        }
    } else {
        if (!yagl_validate_display(dpy_, &dpy)) {
            goto out;
        }
    }

    if (release_context) {
        if (!yagl_egl_release_current_context(dpy)) {
            YAGL_SET_ERR(EGL_BAD_ACCESS);
            goto out;
        }
    } else {
        if (!yagl_validate_context(dpy, ctx_, &ctx)) {
            goto out;
        }

        if (!yagl_validate_surface(dpy, draw_, &draw)) {
            goto out;
        }

        if (!yagl_validate_surface(dpy, read_, &read)) {
            goto out;
        }

        if (prev_ctx != ctx) {
            /*
             * Previous context must be detached from its surfaces.
             */

            release_context = true;
        }

        if (prev_ctx) {
            prev_ctx->backend_ctx->client_ctx->flush(prev_ctx->backend_ctx->client_ctx);
            if (prev_ctx != ctx) {
                prev_ctx->backend_ctx->client_ctx->deactivate(prev_ctx->backend_ctx->client_ctx);
            }
        }

        if (!egl_api_ts->backend->make_current(egl_api_ts->backend,
                                               dpy->backend_dpy,
                                               ctx->backend_ctx,
                                               draw->backend_sfc,
                                               read->backend_sfc)) {
            if (prev_ctx && (prev_ctx != ctx)) {
                /*
                 * If host 'make_current' failed then re-activate.
                 */
                prev_ctx->backend_ctx->client_ctx->activate(prev_ctx->backend_ctx->client_ctx);
            }
            YAGL_SET_ERR(EGL_BAD_ACCESS);
            goto out;
        }

        yagl_egl_context_update_surfaces(ctx, draw, read);

        yagl_egl_api_ts_update_context(egl_api_ts, ctx);

        if (prev_ctx != ctx) {
            ctx->backend_ctx->client_ctx->activate(ctx->backend_ctx->client_ctx);
        }
    }

    if (release_context && prev_ctx) {
        yagl_egl_context_update_surfaces(prev_ctx, NULL, NULL);
    }

    YAGL_LOG_TRACE("Context switched (%u, %u, %u, %u)",
                   dpy_,
                   draw_,
                   read_,
                   ctx_);

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(read);
    yagl_egl_surface_release(draw);
    yagl_egl_context_release(ctx);
    yagl_egl_context_release(prev_ctx);

    return true;
}

bool yagl_host_eglQueryContext(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle ctx_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *ctx = NULL;
    EGLint value = 0;

    YAGL_LOG_FUNC_SET(eglQueryContext);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_context(dpy, ctx_, &ctx)) {
        goto out;
    }

    switch (attribute) {
    case EGL_CONFIG_ID:
        value = ctx->cfg->native.config_id;
        break;
    case EGL_CONTEXT_CLIENT_TYPE:
        switch (ctx->backend_ctx->client_ctx->client_api) {
        case yagl_client_api_gles1:
        case yagl_client_api_gles2:
            value = EGL_OPENGL_ES_API;
            break;
        case yagl_client_api_ogl:
            value = EGL_OPENGL_API;
            break;
        case yagl_client_api_ovg:
            value = EGL_OPENVG_API;
            break;
        default:
            assert(false);
            value = EGL_NONE;
            break;
        }
        break;
    case EGL_CONTEXT_CLIENT_VERSION:
        switch (ctx->backend_ctx->client_ctx->client_api) {
        case yagl_client_api_gles1:
            value = 1;
            break;
        case yagl_client_api_gles2:
            value = 2;
            break;
        case yagl_client_api_ogl:
        case yagl_client_api_ovg:
        default:
            value = 0;
            break;
        }
        break;
    case EGL_RENDER_BUFFER:
        if (ctx->draw) {
            switch (ctx->draw->backend_sfc->type) {
            case EGL_PBUFFER_BIT:
            case EGL_WINDOW_BIT:
                value = EGL_BACK_BUFFER;
                break;
            case EGL_PIXMAP_BIT:
                value = EGL_SINGLE_BUFFER;
                break;
            default:
                assert(0);
                value = EGL_NONE;
            }
        } else {
            value = EGL_NONE;
        }
        break;
    default:
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_mem_prepare_EGLint(cur_ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(cur_ts->mt1, value);
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_context_release(ctx);

    return res;
}

bool yagl_host_eglSwapBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglSwapBuffers);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!surface->backend_sfc->swap_buffers(surface->backend_sfc)) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglCopyBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCopyBuffers);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!surface->backend_sfc->copy_buffers(surface->backend_sfc)) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglCreateImageKHR(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle ctx,
    EGLenum target,
    yagl_winsys_id buffer,
    target_ulong /* const EGLint* */ attrib_list_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_image *image = NULL;

    YAGL_LOG_FUNC_SET(eglCreateImageKHR);

    *retval = 0;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    image = yagl_egl_image_create(dpy, buffer);

    if (!image) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    yagl_egl_display_add_image(dpy, image);
    yagl_egl_image_release(image);

    *retval = image->res.handle;

out:
    return true;
}

bool yagl_host_eglDestroyImageKHR(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle image_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_image *image = NULL;

    YAGL_LOG_FUNC_SET(eglDestroyImageKHR);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_image(dpy, image_, &image)) {
        goto out;
    }

    if (yagl_egl_display_remove_image(dpy, image->res.handle)) {
        *retval = EGL_TRUE;
    } else {
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
    }

out:
    yagl_egl_image_release(image);

    return true;
}

bool yagl_host_eglCreateWindowSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_window_attribs attribs;
    int i = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreateWindowSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_window_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        while (attrib_list[i] != EGL_NONE) {
            switch (attrib_list[i]) {
            case EGL_RENDER_BUFFER:
                break;
            default:
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }

            i += 2;
        }
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_offscreen_surface) {
        YAGL_LOG_CRITICAL("Offscreen surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_NATIVE_WINDOW);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_offscreen_surface(dpy->backend_dpy,
                                                             &config->native,
                                                             EGL_WINDOW_BIT,
                                                             &attribs,
                                                             width,
                                                             height,
                                                             bpp,
                                                             pixels_);

    if (!backend_sfc) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_WINDOW);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pbuffer_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglCreatePbufferSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_pbuffer_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        while (attrib_list[i] != EGL_NONE) {
            switch (attrib_list[i]) {
            case EGL_LARGEST_PBUFFER:
                attribs.largest = (attrib_list[i + 1] ? EGL_TRUE : EGL_FALSE);
                break;
            case EGL_MIPMAP_TEXTURE:
                attribs.tex_mipmap = (attrib_list[i + 1] ? EGL_TRUE : EGL_FALSE);
                break;
            case EGL_TEXTURE_FORMAT:
                switch (attrib_list[i + 1]) {
                case EGL_NO_TEXTURE:
                case EGL_TEXTURE_RGB:
                case EGL_TEXTURE_RGBA:
                    attribs.tex_format = attrib_list[i + 1];
                    break;
                default:
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                break;
            case EGL_TEXTURE_TARGET:
                switch (attrib_list[i + 1]) {
                case EGL_NO_TEXTURE:
                case EGL_TEXTURE_2D:
                    attribs.tex_target = attrib_list[i + 1];
                    break;
                default:
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                break;
            case EGL_HEIGHT:
            case EGL_WIDTH:
                break;
            default:
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }

            i += 2;
        }
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_offscreen_surface) {
        YAGL_LOG_CRITICAL("Offscreen surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_offscreen_surface(dpy->backend_dpy,
                                                             &config->native,
                                                             EGL_PBUFFER_BIT,
                                                             &attribs,
                                                             width,
                                                             height,
                                                             bpp,
                                                             pixels_);

    if (!backend_sfc) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pixmap_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreatePixmapSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_pixmap_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_offscreen_surface) {
        YAGL_LOG_CRITICAL("Offscreen surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_offscreen_surface(dpy->backend_dpy,
                                                             &config->native,
                                                             EGL_PIXMAP_BIT,
                                                             &attribs,
                                                             width,
                                                             height,
                                                             bpp,
                                                             pixels_);

    if (!backend_sfc) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglResizeOffscreenSurfaceYAGL(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_eglb_surface *draw_sfc = NULL;
    struct yagl_eglb_surface *read_sfc = NULL;

    YAGL_LOG_FUNC_SET(eglResizeOffscreenSurfaceYAGL);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_offscreen_surface) {
        YAGL_LOG_CRITICAL("Offscreen surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_offscreen_surface(dpy->backend_dpy,
                                                             &surface->cfg->native,
                                                             surface->backend_sfc->type,
                                                             &surface->backend_sfc->attribs,
                                                             width,
                                                             height,
                                                             bpp,
                                                             pixels_);

    if (!backend_sfc) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    if (egl_api_ts->context->draw) {
        if (egl_api_ts->context->draw == surface) {
            draw_sfc = backend_sfc;
        } else {
            draw_sfc = egl_api_ts->context->draw->backend_sfc;
        }
    }

    if (egl_api_ts->context->read) {
        if (egl_api_ts->context->read == surface) {
            read_sfc = backend_sfc;
        } else {
            read_sfc = egl_api_ts->context->read->backend_sfc;
        }
    }

    if (!egl_api_ts->backend->make_current(egl_api_ts->backend,
                                           dpy->backend_dpy,
                                           egl_api_ts->context->backend_ctx,
                                           draw_sfc,
                                           read_sfc)) {
        YAGL_LOG_ERROR("make_current failed");
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    surface->backend_sfc->replace(surface->backend_sfc, backend_sfc);

    backend_sfc = NULL;

    *retval = EGL_TRUE;

out:
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglUpdateOffscreenImageYAGL(yagl_host_handle dpy_,
    yagl_host_handle image_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* const void* */ pixels)
{
    bool res = true;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_image *image = NULL;

    YAGL_LOG_FUNC_SET(eglUpdateOffscreenImageYAGL);

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_image(dpy, image_, &image)) {
        goto out;
    }

    if (!image->backend_image->update_offscreen) {
        YAGL_LOG_CRITICAL("Offscreen images not supported");
        YAGL_SET_ERR(EGL_BAD_PARAMETER);
        goto out;
    }

    res = image->backend_image->update_offscreen(image->backend_image,
                                                 width,
                                                 height,
                                                 bpp,
                                                 pixels);

out:
    yagl_egl_image_release(image);

    return res;
}

bool yagl_host_eglCreateWindowSurfaceOnscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id win,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_window_attribs attribs;
    int i = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreateWindowSurfaceOnscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_window_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        while (attrib_list[i] != EGL_NONE) {
            switch (attrib_list[i]) {
            case EGL_RENDER_BUFFER:
                break;
            default:
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }

            i += 2;
        }
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_onscreen_window_surface) {
        YAGL_LOG_CRITICAL("Onscreen window surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_NATIVE_WINDOW);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_onscreen_window_surface(dpy->backend_dpy,
                                                                   &config->native,
                                                                   &attribs,
                                                                   win);

    if (!backend_sfc) {
        YAGL_LOG_ERROR("Unable to create window surface for 0x%X", win);
        YAGL_SET_ERR(EGL_BAD_NATIVE_WINDOW);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglCreatePbufferSurfaceOnscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id buffer,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pbuffer_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglCreatePbufferSurfaceOnscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_pbuffer_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        while (attrib_list[i] != EGL_NONE) {
            switch (attrib_list[i]) {
            case EGL_LARGEST_PBUFFER:
                attribs.largest = (attrib_list[i + 1] ? EGL_TRUE : EGL_FALSE);
                break;
            case EGL_MIPMAP_TEXTURE:
                attribs.tex_mipmap = (attrib_list[i + 1] ? EGL_TRUE : EGL_FALSE);
                break;
            case EGL_TEXTURE_FORMAT:
                switch (attrib_list[i + 1]) {
                case EGL_NO_TEXTURE:
                case EGL_TEXTURE_RGB:
                case EGL_TEXTURE_RGBA:
                    attribs.tex_format = attrib_list[i + 1];
                    break;
                default:
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                break;
            case EGL_TEXTURE_TARGET:
                switch (attrib_list[i + 1]) {
                case EGL_NO_TEXTURE:
                case EGL_TEXTURE_2D:
                    attribs.tex_target = attrib_list[i + 1];
                    break;
                default:
                    YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                    goto out;
                }
                break;
            case EGL_HEIGHT:
            case EGL_WIDTH:
                break;
            default:
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }

            i += 2;
        }
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_onscreen_pbuffer_surface) {
        YAGL_LOG_CRITICAL("Onscreen pbuffer surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_onscreen_pbuffer_surface(dpy->backend_dpy,
                                                                    &config->native,
                                                                    &attribs,
                                                                    buffer);

    if (!backend_sfc) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglCreatePixmapSurfaceOnscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id pixmap,
    target_ulong /* const EGLint* */ attrib_list_)
{
    bool res = true;
    EGLint *attrib_list = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pixmap_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreatePixmapSurfaceOnscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    yagl_egl_pixmap_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config)) {
        goto out;
    }

    if (!dpy->backend_dpy->create_onscreen_pixmap_surface) {
        YAGL_LOG_CRITICAL("Onscreen pixmap surfaces not supported");
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    backend_sfc = dpy->backend_dpy->create_onscreen_pixmap_surface(dpy->backend_dpy,
                                                                   &config->native,
                                                                   &attribs,
                                                                   pixmap);

    if (!backend_sfc) {
        YAGL_LOG_ERROR("Unable to create pixmap surface for 0x%X", pixmap);
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    surface = yagl_egl_surface_create(dpy, config, backend_sfc);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    backend_sfc = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    g_free(attrib_list);

    return res;
}

bool yagl_host_eglInvalidateOnscreenSurfaceYAGL(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    yagl_winsys_id buffer)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    surface->backend_sfc->invalidate(surface->backend_sfc, buffer);

out:
    yagl_egl_surface_release(surface);

    return true;
}
