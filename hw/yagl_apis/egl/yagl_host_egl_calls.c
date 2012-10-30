#include "yagl_host_egl_calls.h"
#include "yagl_egl_calls.h"
#include "yagl_egl_api_ps.h"
#include "yagl_egl_api_ts.h"
#include "yagl_egl_api.h"
#include "yagl_egl_display.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_interface.h"
#include "yagl_egl_config.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_context.h"
#include "yagl_egl_validate.h"
#include "yagl_compiled_transfer.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_mem_egl.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_client_interface.h"
#include "yagl_client_context.h"
#include "yagl_sharegroup.h"

#define YAGL_EGL_VERSION_MAJOR 1
#define YAGL_EGL_VERSION_MINOR 4

#define YAGL_SET_ERR(err) \
    if (egl_api_ts->error == EGL_SUCCESS) { \
        egl_api_ts->error = err; \
    } \
    YAGL_LOG_ERROR("error = 0x%X", err)

#define YAGL_UNIMPLEMENTED(func, ret) \
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, func); \
    YAGL_LOG_WARN("NOT IMPLEMENTED!!!"); \
    *retval = ret; \
    return true;

static YAGL_DEFINE_TLS(struct yagl_egl_api_ts*, egl_api_ts);

static __inline bool yagl_validate_display(yagl_host_handle dpy_,
                                           struct yagl_egl_display **dpy)
{
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, yagl_validate_display);

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
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, yagl_validate_config);

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
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, yagl_validate_surface);

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
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, yagl_validate_context);

    *ctx = yagl_egl_display_acquire_context(dpy, ctx_);

    if (!*ctx) {
        YAGL_SET_ERR(EGL_BAD_CONTEXT);
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
        return egl_api_ts->context->client_ctx;
    } else {
        return NULL;
    }
}

static bool yagl_egl_release_current_context(struct yagl_egl_display *dpy)
{
    if (!egl_api_ts->context) {
        return true;
    }

    egl_api_ts->context->client_ctx->flush(egl_api_ts->context->client_ctx);
    egl_api_ts->context->client_ctx->deactivate(egl_api_ts->context->client_ctx);

    if (!egl_api_ts->driver_ps->make_current(egl_api_ts->driver_ps,
                                             dpy->native_dpy,
                                             EGL_NO_SURFACE,
                                             EGL_NO_SURFACE,
                                             EGL_NO_CONTEXT)) {
        /*
         * If host 'make_current' failed then re-activate.
         */
        egl_api_ts->context->client_ctx->activate(egl_api_ts->context->client_ctx,
                                                  egl_api_ts->ts);
        return false;
    }

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

static void yagl_host_egl_thread_init(struct yagl_api_ps *api_ps,
                                      struct yagl_thread_state *ts)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER_TS(ts, yagl_host_egl_thread_init, NULL);

    egl_api_ps->driver_ps->thread_init(egl_api_ps->driver_ps, ts);

    egl_api_ts = g_malloc0(sizeof(*egl_api_ts));

    yagl_egl_api_ts_init(egl_api_ts, egl_api_ps, ts);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_egl_thread_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER_TS(egl_api_ts->ts, yagl_host_egl_thread_fini, NULL);

    yagl_egl_api_ts_cleanup(egl_api_ts);

    g_free(egl_api_ts);

    egl_api_ts = NULL;

    egl_api_ps->driver_ps->thread_fini(egl_api_ps->driver_ps);

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

    YAGL_LOG_FUNC_ENTER(api_ps->ps->id, 0, yagl_host_egl_process_destroy, NULL);

    yagl_egl_api_ps_cleanup(egl_api_ps);
    yagl_api_ps_cleanup(&egl_api_ps->base);

    g_free(egl_api_ps);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_api_ps
    *yagl_host_egl_process_init(struct yagl_api *api,
                                struct yagl_process_state *ps)
{
    struct yagl_egl_api *egl_api = (struct yagl_egl_api*)api;
    struct yagl_egl_driver_ps *driver_ps;
    struct yagl_egl_interface *egl_iface;
    struct yagl_egl_api_ps *egl_api_ps;

    YAGL_LOG_FUNC_ENTER(ps->id, 0, yagl_host_egl_process_init, NULL);

    /*
     * Create driver ps.
     */

    driver_ps = egl_api->driver->process_init(egl_api->driver, ps);
    assert(driver_ps);

    /*
     * Create EGL interface.
     */

    egl_iface = g_malloc0(sizeof(*egl_iface));

    yagl_egl_interface_init(egl_iface);

    egl_iface->get_ctx = &yagl_egl_get_ctx;

    /*
     * Finally, create API ps.
     */

    egl_api_ps = g_malloc0(sizeof(*egl_api_ps));

    yagl_api_ps_init(&egl_api_ps->base, api, ps);

    egl_api_ps->base.thread_init = &yagl_host_egl_thread_init;
    egl_api_ps->base.get_func = &yagl_host_egl_get_func;
    egl_api_ps->base.thread_fini = &yagl_host_egl_thread_fini;
    egl_api_ps->base.fini = &yagl_host_egl_process_fini;
    egl_api_ps->base.destroy = &yagl_host_egl_process_destroy;

    yagl_egl_api_ps_init(egl_api_ps, driver_ps, egl_iface);

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
    target_ulong /* Display* */ display_id)
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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglInitialize);

    if (!yagl_mem_prepare_EGLint(egl_api_ts->ts->mt1, major) ||
        !yagl_mem_prepare_EGLint(egl_api_ts->ts->mt2, minor)) {
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
        yagl_mem_put_EGLint(egl_api_ts->ts->mt1, YAGL_EGL_VERSION_MAJOR);
    }

    if (minor) {
        yagl_mem_put_EGLint(egl_api_ts->ts->mt2, YAGL_EGL_VERSION_MINOR);
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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglGetConfigs);

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

    if (!yagl_mem_prepare(egl_api_ts->ts->mt1, configs_, num_config * sizeof(*configs)) ||
        !yagl_mem_prepare_EGLint(egl_api_ts->ts->mt2, num_config_)) {
        res = false;
        goto out;
    }

    if (configs_) {
        yagl_mem_put(egl_api_ts->ts->mt1, configs);
    }

    yagl_mem_put_EGLint(egl_api_ts->ts->mt2, num_config);

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglChooseConfig);

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
        attrib_list = yagl_mem_get_attrib_list(egl_api_ts->ts,
                                               attrib_list_);

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

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        bool has_config_id = false;
        EGLint config_id = 0;

        while ((attrib_list[i] != EGL_NONE) && !has_config_id) {
            switch (attrib_list[i]) {
            case EGL_MAX_PBUFFER_WIDTH:
            case EGL_MAX_PBUFFER_HEIGHT:
            case EGL_MAX_PBUFFER_PIXELS:
            case EGL_NATIVE_VISUAL_ID:
            case EGL_BIND_TO_TEXTURE_RGB:
            case EGL_BIND_TO_TEXTURE_RGBA:
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
                if (!yagl_mem_prepare_host_handle(egl_api_ts->ts->mt1, configs_) ||
                    !yagl_mem_prepare_EGLint(egl_api_ts->ts->mt2, num_config_)) {
                    res = false;
                    goto out;
                }

                if (configs_) {
                    yagl_mem_put_host_handle(egl_api_ts->ts->mt1, handle);
                }

                yagl_mem_put_EGLint(egl_api_ts->ts->mt2, 1);

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

    if (!yagl_mem_prepare(egl_api_ts->ts->mt1, configs_, num_config * sizeof(configs)) ||
        !yagl_mem_prepare_EGLint(egl_api_ts->ts->mt2, num_config_)) {
        res = false;
        goto out;
    }

    if (configs_) {
        yagl_mem_put(egl_api_ts->ts->mt1, configs);
    }

    yagl_mem_put_EGLint(egl_api_ts->ts->mt2, num_config);

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglGetConfigAttrib);

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

    if (!yagl_mem_prepare_EGLint(egl_api_ts->ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(egl_api_ts->ts->mt1, value);
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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglDestroySurface);

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

    yagl_egl_surface_lock(surface);

    assert(surface->bimage_ct);

    if (!surface->bimage_ct) {
        yagl_egl_surface_unlock(surface);
        YAGL_LOG_CRITICAL("we're the one who destroy the surface, but bimage isn't there!");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    yagl_egl_surface_invalidate(surface);

    *retval = EGL_TRUE;

    yagl_egl_surface_unlock(surface);

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglQuerySurface);

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
    case EGL_HEIGHT:
        value = surface->height;
        break;
    case EGL_WIDTH:
        value = surface->width;
        break;
    case EGL_LARGEST_PBUFFER:
        if (surface->type == EGL_PBUFFER_BIT) {
            value = surface->attribs.pbuffer.largest;
        } else {
            /*
             * That's right 'value_', not 'value', when an attribute is not
             * applicable we shouldn't write back anything.
             */
            value_ = 0;
        }
        break;
    case EGL_TEXTURE_FORMAT:
        if (surface->type == EGL_PBUFFER_BIT) {
            value = surface->attribs.pbuffer.tex_format;
        } else {
            value_ = 0;
        }
        break;
    case EGL_TEXTURE_TARGET:
        if (surface->type == EGL_PBUFFER_BIT) {
            value = surface->attribs.pbuffer.tex_target;
        } else {
            value_ = 0;
        }
        break;
    case EGL_MIPMAP_TEXTURE:
        if (surface->type == EGL_PBUFFER_BIT) {
            value = surface->attribs.pbuffer.tex_mipmap;
        } else {
            value_ = 0;
        }
        break;
    case EGL_MIPMAP_LEVEL:
        if (surface->type == EGL_PBUFFER_BIT) {
            value = 0;
        } else {
            value_ = 0;
        }
        break;
    case EGL_RENDER_BUFFER:
        switch (surface->type) {
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
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_mem_prepare_EGLint(egl_api_ts->ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(egl_api_ts->ts->mt1, value);
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return res;
}

bool yagl_host_eglBindAPI(EGLBoolean* retval,
    EGLenum api)
{
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglBindAPI);

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglWaitClient);

    *retval = EGL_FALSE;

    if (!egl_api_ts->context) {
        *retval = EGL_TRUE;
        goto out;
    }

    if (!egl_api_ts->context->draw) {
        YAGL_SET_ERR(EGL_BAD_CURRENT_SURFACE);
        goto out;
    }

    sfc = yagl_egl_display_acquire_surface(egl_api_ts->context->dpy,
                                           egl_api_ts->context->draw->res.handle);

    if (!sfc || (sfc != egl_api_ts->context->draw)) {
        YAGL_SET_ERR(EGL_BAD_CURRENT_SURFACE);
        goto out;
    }

    egl_api_ts->context->client_ctx->finish(egl_api_ts->context->client_ctx);

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(sfc);

    return true;
}

bool yagl_host_eglReleaseThread(EGLBoolean* retval)
{
    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglReleaseThread);

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
    yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer)
{
    YAGL_UNIMPLEMENTED(eglBindTexImage, EGL_FALSE);
}

bool yagl_host_eglReleaseTexImage(EGLBoolean* retval,
    yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer)
{
    YAGL_UNIMPLEMENTED(eglReleaseTexImage, EGL_FALSE);
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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglCreateContext);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(egl_api_ts->ts,
                                               attrib_list_);

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

    client_iface = egl_api_ts->ts->ps->client_ifaces[client_api];

    if (!client_iface) {
        YAGL_LOG_ERROR("client API %d is not supported", client_api);
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (share_context_) {
        if (!yagl_validate_context(dpy, share_context_, &share_context)) {
            goto out;
        }
        sg = share_context->client_ctx->sg;
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
                                  (share_context ? share_context->native_ctx
                                                 : EGL_NO_CONTEXT));

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglDestroyContext);

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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglMakeCurrent);

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
            prev_ctx->client_ctx->flush(prev_ctx->client_ctx);
            if (prev_ctx != ctx) {
                prev_ctx->client_ctx->deactivate(prev_ctx->client_ctx);
            }
        }

        if (!egl_api_ts->driver_ps->make_current(egl_api_ts->driver_ps,
                                                 dpy->native_dpy,
                                                 draw->native_sfc,
                                                 read->native_sfc,
                                                 ctx->native_ctx)) {
            if (prev_ctx && (prev_ctx != ctx)) {
                /*
                 * If host 'make_current' failed then re-activate.
                 */
                prev_ctx->client_ctx->activate(prev_ctx->client_ctx, egl_api_ts->ts);
            }
            YAGL_SET_ERR(EGL_BAD_ACCESS);
            goto out;
        }

        yagl_egl_context_update_surfaces(ctx, draw, read);

        yagl_egl_api_ts_update_context(egl_api_ts, ctx);

        if (prev_ctx != ctx) {
            ctx->client_ctx->activate(ctx->client_ctx, egl_api_ts->ts);
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

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglQueryContext);

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
        switch (ctx->client_ctx->client_api) {
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
        switch (ctx->client_ctx->client_api) {
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
            switch (ctx->draw->type) {
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

    if (!yagl_mem_prepare_EGLint(egl_api_ts->ts->mt1, value_)) {
        res = false;
        goto out;
    }

    if (value_) {
        yagl_mem_put_EGLint(egl_api_ts->ts->mt1, value);
    }

    *retval = EGL_TRUE;

out:
    yagl_egl_context_release(ctx);

    return res;
}

bool yagl_host_eglWaitGL(EGLBoolean* retval)
{
    EGLBoolean tmp;
    EGLenum api = egl_api_ts->api;

    if (!yagl_host_eglBindAPI(&tmp, EGL_OPENGL_ES_API)) {
        return false;
    }

    if (!yagl_host_eglWaitClient(retval)) {
        return false;
    }

    return yagl_host_eglBindAPI(&tmp, api);
}

bool yagl_host_eglWaitNative(EGLBoolean* retval,
    EGLint engine)
{
    struct yagl_egl_surface *sfc = NULL;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglWaitNative);

    *retval = EGL_FALSE;

    if (!egl_api_ts->context) {
        *retval = EGL_TRUE;
        goto out;
    }

    if (!egl_api_ts->context->draw) {
        YAGL_SET_ERR(EGL_BAD_CURRENT_SURFACE);
        goto out;
    }

    sfc = yagl_egl_display_acquire_surface(egl_api_ts->context->dpy,
                                           egl_api_ts->context->draw->res.handle);

    if (!sfc || (sfc != egl_api_ts->context->draw)) {
        YAGL_SET_ERR(EGL_BAD_CURRENT_SURFACE);
        goto out;
    }

    egl_api_ts->driver_ps->wait_native(egl_api_ts->driver_ps);

    *retval = EGL_TRUE;

out:
    yagl_egl_surface_release(sfc);

    return true;
}

bool yagl_host_eglSwapBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglSwapBuffers);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!egl_api_ts->context) {
        YAGL_LOG_ERROR("No current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!yagl_egl_context_uses_surface(egl_api_ts->context, surface)) {
        YAGL_LOG_ERROR("Surface not attached to current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!egl_api_ts->context->client_ctx->read_pixels(egl_api_ts->context->client_ctx,
                                                      surface->width,
                                                      surface->height,
                                                      surface->bpp,
                                                      surface->host_pixels)) {
        YAGL_LOG_ERROR("read_pixels failed");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    yagl_egl_surface_lock(surface);

    if (surface->bimage_ct) {
        yagl_compiled_transfer_exec(surface->bimage_ct, surface->host_pixels);

        *retval = EGL_TRUE;
    } else {
        YAGL_LOG_ERROR("surface was destroyed, weird scenario!");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
    }

    yagl_egl_surface_unlock(surface);

out:
    yagl_egl_surface_release(surface);

    return true;
}

bool yagl_host_eglCopyBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLNativePixmapType target)
{
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglCopyBuffers);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!egl_api_ts->context) {
        YAGL_LOG_ERROR("No current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!yagl_egl_context_uses_surface(egl_api_ts->context, surface)) {
        YAGL_LOG_ERROR("Surface not attached to current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!egl_api_ts->context->client_ctx->read_pixels(egl_api_ts->context->client_ctx,
                                                      surface->width,
                                                      surface->height,
                                                      surface->bpp,
                                                      surface->host_pixels)) {
        YAGL_LOG_ERROR("read_pixels failed");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    yagl_egl_surface_lock(surface);

    if (surface->bimage_ct) {
        yagl_compiled_transfer_exec(surface->bimage_ct, surface->host_pixels);

        *retval = EGL_TRUE;
    } else {
        YAGL_LOG_ERROR("surface was destroyed, weird scenario!");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
    }

    yagl_egl_surface_unlock(surface);

out:
    yagl_egl_surface_release(surface);

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
    struct yagl_compiled_transfer *bimage_ct = NULL;
    struct yagl_egl_window_attribs attribs;
    int i = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglCreateWindowSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(egl_api_ts->ts,
                                               attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    /*
     * 'pixels_' are locked in target's memory, no page fault possible.
     */

    bimage_ct = yagl_compiled_transfer_create(egl_api_ts->ts,
                                              pixels_,
                                              (width * height * bpp),
                                              true);

    if (!bimage_ct) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_WINDOW);
        goto out;
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

    surface = yagl_egl_surface_create_window(dpy,
                                             config,
                                             &attribs,
                                             bimage_ct,
                                             width,
                                             height,
                                             bpp);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    bimage_ct = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (bimage_ct) {
        yagl_compiled_transfer_destroy(bimage_ct);
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
    struct yagl_compiled_transfer *bimage_ct = NULL;
    struct yagl_egl_pbuffer_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;
    int i = 0;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglCreatePbufferSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(egl_api_ts->ts,
                                               attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    bimage_ct = yagl_compiled_transfer_create(egl_api_ts->ts,
                                              pixels_,
                                              (width * height * bpp),
                                              true);

    if (!bimage_ct) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
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

    surface = yagl_egl_surface_create_pbuffer(dpy,
                                              config,
                                              &attribs,
                                              bimage_ct,
                                              width,
                                              height,
                                              bpp);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    bimage_ct = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (bimage_ct) {
        yagl_compiled_transfer_destroy(bimage_ct);
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
    struct yagl_compiled_transfer *bimage_ct = NULL;
    struct yagl_egl_pixmap_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglCreatePixmapSurfaceOffscreenYAGL);

    *retval = 0;

    if (attrib_list_) {
        attrib_list = yagl_mem_get_attrib_list(egl_api_ts->ts,
                                               attrib_list_);

        if (!attrib_list) {
            res = false;
            goto out;
        }
    }

    bimage_ct = yagl_compiled_transfer_create(egl_api_ts->ts,
                                              pixels_,
                                              (width * height * bpp),
                                              true);

    if (!bimage_ct) {
        YAGL_SET_ERR(EGL_BAD_NATIVE_PIXMAP);
        goto out;
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

    surface = yagl_egl_surface_create_pixmap(dpy,
                                             config,
                                             &attribs,
                                             bimage_ct,
                                             width,
                                             height,
                                             bpp);

    if (!surface) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    /*
     * Owned by 'surface' now.
     */
    bimage_ct = NULL;

    yagl_egl_display_add_surface(dpy, surface);
    yagl_egl_surface_release(surface);

    *retval = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (bimage_ct) {
        yagl_compiled_transfer_destroy(bimage_ct);
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
    struct yagl_compiled_transfer *bimage_ct = NULL;
    EGLSurface native_sfc = EGL_NO_SURFACE;
    EGLSurface draw_sfc = EGL_NO_SURFACE;
    EGLSurface read_sfc = EGL_NO_SURFACE;

    YAGL_LOG_FUNC_SET_TS(egl_api_ts->ts, eglResizeOffscreenSurfaceYAGL);

    *retval = EGL_FALSE;

    if (!yagl_validate_display(dpy_, &dpy)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface)) {
        goto out;
    }

    if (!egl_api_ts->context) {
        YAGL_LOG_ERROR("No current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!yagl_egl_context_uses_surface(egl_api_ts->context, surface)) {
        YAGL_LOG_ERROR("Surface not attached to current context");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    bimage_ct = yagl_compiled_transfer_create(egl_api_ts->ts,
                                              pixels_,
                                              (width * height * bpp),
                                              true);

    if (!bimage_ct) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    native_sfc = egl_api_ts->driver_ps->pbuffer_surface_create(egl_api_ts->driver_ps,
                                                               dpy->native_dpy,
                                                               &surface->cfg->native,
                                                               width,
                                                               height,
                                                               &surface->native_sfc_attribs);

    if (native_sfc == EGL_NO_SURFACE) {
        YAGL_LOG_ERROR("pbuffer_surface_create failed");
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    egl_api_ts->context->client_ctx->flush(egl_api_ts->context->client_ctx);

    if (egl_api_ts->context->draw) {
        if (egl_api_ts->context->draw == surface) {
            draw_sfc = native_sfc;
        } else {
            draw_sfc = egl_api_ts->context->draw->native_sfc;
        }
    }

    if (egl_api_ts->context->read) {
        if (egl_api_ts->context->read == surface) {
            read_sfc = native_sfc;
        } else {
            read_sfc = egl_api_ts->context->read->native_sfc;
        }
    }

    yagl_egl_surface_lock(surface);

    if (!surface->bimage_ct) {
        yagl_egl_surface_unlock(surface);
        YAGL_LOG_ERROR("surface was destroyed, weird scenario!");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    if (!egl_api_ts->driver_ps->make_current(egl_api_ts->driver_ps,
                                             dpy->native_dpy,
                                             draw_sfc,
                                             read_sfc,
                                             egl_api_ts->context->native_ctx)) {
        yagl_egl_surface_unlock(surface);
        YAGL_LOG_ERROR("make_current failed");
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    yagl_egl_surface_update(surface,
                            native_sfc,
                            bimage_ct,
                            width,
                            height,
                            bpp);

    native_sfc = EGL_NO_SURFACE;
    bimage_ct = NULL;

    *retval = EGL_TRUE;

    yagl_egl_surface_unlock(surface);

out:
    if (native_sfc != EGL_NO_SURFACE) {
        egl_api_ts->driver_ps->pbuffer_surface_destroy(egl_api_ts->driver_ps,
                                                       dpy->native_dpy,
                                                       native_sfc);
    }
    if (bimage_ct) {
        yagl_compiled_transfer_destroy(bimage_ct);
    }
    yagl_egl_surface_release(surface);

    return true;
}
