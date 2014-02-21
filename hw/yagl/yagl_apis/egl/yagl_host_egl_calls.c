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
#include "yagl_egl_validate.h"
#include "yagl_eglb_display.h"
#include "yagl_eglb_context.h"
#include "yagl_eglb_surface.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_object_map.h"
#include <EGL/eglext.h>

#define YAGL_EGL_VERSION_MAJOR 1
#define YAGL_EGL_VERSION_MINOR 4

#define YAGL_SET_ERR(err) \
    *error = err; \
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

static uint32_t yagl_egl_get_ctx_id(struct yagl_egl_interface *iface)
{
    if (egl_api_ts) {
        return egl_api_ts->context ? egl_api_ts->context->res.handle : 0;
    } else {
        return 0;
    }
}

static void yagl_egl_ensure_ctx(struct yagl_egl_interface *iface, uint32_t ctx_id)
{
    struct yagl_egl_interface_impl *egl_iface = (struct yagl_egl_interface_impl*)iface;
    uint32_t current_ctx_id = yagl_egl_get_ctx_id(iface);

    if (!current_ctx_id || (ctx_id && (current_ctx_id != ctx_id))) {
        egl_iface->backend->ensure_current(egl_iface->backend);
    }
}

static void yagl_egl_unensure_ctx(struct yagl_egl_interface *iface, uint32_t ctx_id)
{
    struct yagl_egl_interface_impl *egl_iface = (struct yagl_egl_interface_impl*)iface;
    uint32_t current_ctx_id = yagl_egl_get_ctx_id(iface);

    if (!current_ctx_id || (ctx_id && (current_ctx_id != ctx_id))) {
        egl_iface->backend->unensure_current(egl_iface->backend);
    }
}

static __inline bool yagl_validate_display(yagl_host_handle dpy_,
                                           struct yagl_egl_display **dpy,
                                           EGLint *error)
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
                                          struct yagl_egl_config **cfg,
                                          EGLint *error)
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
                                           struct yagl_egl_surface **sfc,
                                           EGLint *error)
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
                                           struct yagl_egl_context **ctx,
                                           EGLint *error)
{
    YAGL_LOG_FUNC_SET(yagl_validate_context);

    *ctx = yagl_egl_display_acquire_context(dpy, ctx_);

    if (!*ctx) {
        YAGL_SET_ERR(EGL_BAD_CONTEXT);
        return false;
    }

    return true;
}

static bool yagl_egl_release_current_context(struct yagl_egl_display *dpy)
{
    if (!egl_api_ts->context) {
        return true;
    }

    if (!egl_api_ts->backend->release_current(egl_api_ts->backend, false)) {
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

    cur_ts->egl_api_ts = egl_api_ts;

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_egl_batch_start(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    egl_api_ts = cur_ts->egl_api_ts;

    egl_api_ps->backend->batch_start(egl_api_ps->backend);
}

static void yagl_host_egl_batch_end(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    egl_api_ps->backend->batch_end(egl_api_ps->backend);
}

static void yagl_host_egl_thread_fini(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_thread_fini, NULL);

    egl_api_ts = cur_ts->egl_api_ts;

    egl_api_ps->backend->batch_start(egl_api_ps->backend);

    yagl_egl_api_ts_cleanup(egl_api_ts);

    g_free(egl_api_ts);

    egl_api_ts = cur_ts->egl_api_ts = NULL;

    egl_api_ps->backend->batch_end(egl_api_ps->backend);
    egl_api_ps->backend->thread_fini(egl_api_ps->backend);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static void yagl_host_egl_process_destroy(struct yagl_api_ps *api_ps)
{
    struct yagl_egl_api_ps *egl_api_ps = (struct yagl_egl_api_ps*)api_ps;

    YAGL_LOG_FUNC_ENTER(yagl_host_egl_process_destroy, NULL);

    yagl_egl_api_ps_cleanup(egl_api_ps);

    g_free(egl_api_ps->egl_iface);

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

    egl_iface->base.get_ctx_id = &yagl_egl_get_ctx_id;
    egl_iface->base.ensure_ctx = &yagl_egl_ensure_ctx;
    egl_iface->base.unensure_ctx = &yagl_egl_unensure_ctx;
    egl_iface->backend = egl_api->backend;

    /*
     * Finally, create API ps.
     */

    egl_api_ps = g_malloc0(sizeof(*egl_api_ps));

    yagl_api_ps_init(&egl_api_ps->base, api);

    egl_api_ps->base.thread_init = &yagl_host_egl_thread_init;
    egl_api_ps->base.batch_start = &yagl_host_egl_batch_start;
    egl_api_ps->base.get_func = &yagl_host_egl_get_func;
    egl_api_ps->base.batch_end = &yagl_host_egl_batch_end;
    egl_api_ps->base.thread_fini = &yagl_host_egl_thread_fini;
    egl_api_ps->base.destroy = &yagl_host_egl_process_destroy;

    yagl_egl_api_ps_init(egl_api_ps, egl_api->backend, &egl_iface->base);

    YAGL_LOG_FUNC_EXIT(NULL);

    return &egl_api_ps->base;
}

yagl_host_handle yagl_host_eglGetDisplay(uint32_t display_id,
    EGLint *error)
{
    struct yagl_egl_display *dpy;

    dpy = yagl_egl_api_ps_display_add(egl_api_ts->api_ps, display_id);

    return (dpy ? dpy->handle : 0);
}

EGLBoolean yagl_host_eglInitialize(yagl_host_handle dpy_,
    EGLint *major,
    EGLint *minor,
    EGLint *error)
{
    struct yagl_egl_display *dpy;

    YAGL_LOG_FUNC_SET(eglInitialize);

    dpy = yagl_egl_api_ps_display_get(egl_api_ts->api_ps, dpy_);

    if (!dpy) {
        YAGL_SET_ERR(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    yagl_egl_display_initialize(dpy);

    if (major) {
        *major = YAGL_EGL_VERSION_MAJOR;
    }

    if (minor) {
        *minor = YAGL_EGL_VERSION_MINOR;
    }

    return EGL_TRUE;
}

EGLBoolean yagl_host_eglTerminate(yagl_host_handle dpy_,
    EGLint *error)
{
    struct yagl_egl_display *dpy = NULL;

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        return EGL_FALSE;
    }

    yagl_egl_display_terminate(dpy);

    return EGL_TRUE;
}

EGLBoolean yagl_host_eglGetConfigs(yagl_host_handle dpy_,
    yagl_host_handle *configs, int32_t configs_maxcount, int32_t *configs_count,
    EGLint *error)
{
    struct yagl_egl_display *dpy = NULL;

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        return EGL_FALSE;
    }

    if (configs) {
        *configs_count = configs_maxcount;
        yagl_egl_display_get_config_handles(dpy, configs, configs_count);
    } else {
        *configs_count = yagl_egl_display_get_config_count(dpy);
    }

    return EGL_TRUE;
}

EGLBoolean yagl_host_eglChooseConfig(yagl_host_handle dpy_,
    const EGLint *attrib_list, int32_t attrib_list_count,
    yagl_host_handle *configs, int32_t configs_maxcount, int32_t *configs_count,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_native_config dummy;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglChooseConfig);

    yagl_egl_native_config_init(&dummy);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
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
                if (configs) {
                    *configs = handle;
                }

                *configs_count = 1;

                res = EGL_TRUE;
                goto out;
            } else {
                YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
                goto out;
            }
        }
    }

    *configs_count = configs_maxcount;
    yagl_egl_display_choose_configs(dpy, &dummy, configs, configs_count);

    YAGL_LOG_DEBUG("chosen %d configs", *configs_count);

    res = EGL_TRUE;

out:
    return res;
}

EGLBoolean yagl_host_eglGetConfigAttrib(yagl_host_handle dpy_,
    yagl_host_handle config_,
    EGLint attribute,
    EGLint *value,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    EGLint tmp;

    YAGL_LOG_FUNC_SET(eglGetConfigAttrib);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
        goto out;
    }

    if (!yagl_egl_config_get_attrib(config, attribute, &tmp)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (value) {
        *value = tmp;
    }

    res = EGL_TRUE;

out:
    yagl_egl_config_release(config);

    return res;
}

EGLBoolean yagl_host_eglDestroySurface(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglDestroySurface);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, error)) {
        goto out;
    }

    if (!yagl_egl_display_remove_surface(dpy, surface->res.handle)) {
        YAGL_SET_ERR(EGL_BAD_SURFACE);
        goto out;
    }

    res = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return res;
}

EGLBoolean yagl_host_eglQuerySurface(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    EGLint *value,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;
    EGLint tmp;

    YAGL_LOG_FUNC_SET(eglQuerySurface);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, error)) {
        goto out;
    }

    switch (attribute) {
    case EGL_CONFIG_ID:
        if (value) {
            *value = surface->cfg->native.config_id;
        }
        break;
    case EGL_LARGEST_PBUFFER:
        if ((surface->backend_sfc->type == EGL_PBUFFER_BIT) && value) {
            *value = surface->backend_sfc->attribs.pbuffer.largest;
        }
        break;
    case EGL_TEXTURE_FORMAT:
        if ((surface->backend_sfc->type == EGL_PBUFFER_BIT) && value) {
            *value = surface->backend_sfc->attribs.pbuffer.tex_format;
        }
        break;
    case EGL_TEXTURE_TARGET:
        if ((surface->backend_sfc->type == EGL_PBUFFER_BIT) && value) {
            *value = surface->backend_sfc->attribs.pbuffer.tex_target;
        }
        break;
    case EGL_MIPMAP_TEXTURE:
        if ((surface->backend_sfc->type == EGL_PBUFFER_BIT) && value) {
            *value = surface->backend_sfc->attribs.pbuffer.tex_mipmap;
        }
        break;
    case EGL_MIPMAP_LEVEL:
        if ((surface->backend_sfc->type == EGL_PBUFFER_BIT) && value) {
            *value = 0;
        }
        break;
    case EGL_RENDER_BUFFER:
        switch (surface->backend_sfc->type) {
        case EGL_PBUFFER_BIT:
        case EGL_WINDOW_BIT:
            if (value) {
                *value = EGL_BACK_BUFFER;
            }
            break;
        case EGL_PIXMAP_BIT:
            if (value) {
                *value = EGL_SINGLE_BUFFER;
            }
            break;
        default:
            break;
        }
        break;
    case EGL_HORIZONTAL_RESOLUTION:
    case EGL_VERTICAL_RESOLUTION:
    case EGL_PIXEL_ASPECT_RATIO:
        if (value) {
            *value = EGL_UNKNOWN;
        }
        break;
    case EGL_SWAP_BEHAVIOR:
        if (value) {
            *value = EGL_BUFFER_PRESERVED;
        }
        break;
    case EGL_MULTISAMPLE_RESOLVE:
        if (value) {
            *value = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
        }
        break;
    default:
        if (!surface->backend_sfc->query(surface->backend_sfc,
                                         attribute,
                                         &tmp)) {
            YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
            goto out;
        }
        if (value) {
            *value = tmp;
        }
        break;
    }

    res = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return res;
}

void yagl_host_eglBindAPI(EGLenum api)
{
    egl_api_ts->api = api;
}

void yagl_host_eglWaitClient(void)
{
    struct yagl_egl_surface *sfc = NULL;

    if (!egl_api_ts->context) {
        return;
    }

    sfc = egl_api_ts->context->draw;

    if (!sfc) {
        return;
    }

    sfc->backend_sfc->wait_gl(sfc->backend_sfc);
}

EGLBoolean yagl_host_eglReleaseThread(EGLint *error)
{
    EGLBoolean res = EGL_FALSE;

    YAGL_LOG_FUNC_SET(eglReleaseThread);

    if (egl_api_ts->context) {
        if (!yagl_egl_release_current_context(egl_api_ts->context->dpy)) {
            YAGL_SET_ERR(EGL_BAD_ACCESS);
            goto out;
        }
    }

    yagl_egl_api_ts_reset(egl_api_ts);

    res = EGL_TRUE;

out:
    return res;
}

EGLBoolean yagl_host_eglSurfaceAttrib(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    EGLint value,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, error)) {
        goto out;
    }

    /*
     * TODO: implement.
     */

    res = EGL_TRUE;

out:
    yagl_egl_surface_release(surface);

    return res;
}

yagl_host_handle yagl_host_eglCreateContext(yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_host_handle share_context_,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    int i = 0, version = 1;
    yagl_host_handle res = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_context *share_context = NULL;
    struct yagl_egl_context *ctx = NULL;

    YAGL_LOG_FUNC_SET(eglCreateContext);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
        goto out;
    }

    if (egl_api_ts->api == EGL_OPENGL_ES_API) {
        if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
            while (attrib_list[i] != EGL_NONE) {
                switch (attrib_list[i]) {
                case EGL_CONTEXT_CLIENT_VERSION:
                    version = attrib_list[i + 1];
                    break;
                default:
                    break;
                }

                i += 2;
            }
        }
    }

    if (share_context_) {
        if (!yagl_validate_context(dpy, share_context_, &share_context, error)) {
            goto out;
        }
    }

    ctx = yagl_egl_context_create(dpy,
                                  config,
                                  (share_context ? share_context->backend_ctx
                                                 : NULL),
                                  version);

    if (!ctx) {
        YAGL_SET_ERR(EGL_BAD_MATCH);
        goto out;
    }

    yagl_egl_display_add_context(dpy, ctx);
    yagl_egl_context_release(ctx);

    res = ctx->res.handle;

out:
    yagl_egl_context_release(share_context);
    yagl_egl_config_release(config);

    return res;
}

EGLBoolean yagl_host_eglDestroyContext(yagl_host_handle dpy_,
    yagl_host_handle ctx_,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *ctx = NULL;

    YAGL_LOG_FUNC_SET(eglDestroyContext);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_context(dpy, ctx_, &ctx, error)) {
        goto out;
    }

    if (yagl_egl_display_remove_context(dpy, ctx->res.handle)) {
        res = EGL_TRUE;
    } else {
        YAGL_SET_ERR(EGL_BAD_CONTEXT);
    }

out:
    yagl_egl_context_release(ctx);

    return res;
}

void yagl_host_eglMakeCurrent(yagl_host_handle dpy_,
    yagl_host_handle draw_,
    yagl_host_handle read_,
    yagl_host_handle ctx_)
{
    EGLint error = 0;
    bool release_context = !draw_ && !read_ && !ctx_;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *prev_ctx = NULL;
    struct yagl_egl_context *ctx = NULL;
    struct yagl_egl_surface *draw = NULL;
    struct yagl_egl_surface *read = NULL;

    YAGL_LOG_FUNC_SET(eglMakeCurrent);

    if (!yagl_validate_display(dpy_, &dpy, &error)) {
        goto out;
    }

    prev_ctx = egl_api_ts->context;
    yagl_egl_context_acquire(prev_ctx);

    if (release_context) {
        if (!yagl_egl_release_current_context(dpy)) {
            YAGL_LOG_ERROR("cannot release current context");
            goto out;
        }
    } else {
        if (!yagl_validate_context(dpy, ctx_, &ctx, &error)) {
            goto out;
        }

        if (draw_ && !yagl_validate_surface(dpy, draw_, &draw, &error)) {
            goto out;
        }

        if (read_ && !yagl_validate_surface(dpy, read_, &read, &error)) {
            goto out;
        }

        if (prev_ctx != ctx) {
            /*
             * Previous context must be detached from its surfaces.
             */

            release_context = true;
        }

        if (!egl_api_ts->backend->make_current(egl_api_ts->backend,
                                               dpy->backend_dpy,
                                               ctx->backend_ctx,
                                               (draw ? draw->backend_sfc : NULL),
                                               (read ? read->backend_sfc : NULL))) {
            YAGL_LOG_ERROR("make_current failed");
            goto out;
        }

        yagl_egl_context_update_surfaces(ctx, draw, read);

        yagl_egl_api_ts_update_context(egl_api_ts, ctx);
    }

    if (release_context && prev_ctx) {
        yagl_egl_context_update_surfaces(prev_ctx, NULL, NULL);
    }

    YAGL_LOG_TRACE("Context switched (%u, %u, %u, %u)",
                   dpy_,
                   draw_,
                   read_,
                   ctx_);

out:
    yagl_egl_surface_release(read);
    yagl_egl_surface_release(draw);
    yagl_egl_context_release(ctx);
    yagl_egl_context_release(prev_ctx);
}

EGLBoolean yagl_host_eglQueryContext(yagl_host_handle dpy_,
    yagl_host_handle ctx_,
    EGLint attribute,
    EGLint *value,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_context *ctx = NULL;

    YAGL_LOG_FUNC_SET(eglQueryContext);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_context(dpy, ctx_, &ctx, error)) {
        goto out;
    }

    switch (attribute) {
    case EGL_CONFIG_ID:
        if (value) {
            *value = ctx->cfg->native.config_id;
        }
        break;
    case EGL_RENDER_BUFFER:
        if (ctx->draw) {
            switch (ctx->draw->backend_sfc->type) {
            case EGL_PBUFFER_BIT:
            case EGL_WINDOW_BIT:
                if (value) {
                    *value = EGL_BACK_BUFFER;
                }
                break;
            case EGL_PIXMAP_BIT:
                if (value) {
                    *value = EGL_SINGLE_BUFFER;
                }
                break;
            default:
                if (value) {
                    *value = EGL_NONE;
                }
                break;
            }
        } else if (value) {
            *value = EGL_NONE;
        }
        break;
    default:
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    res = EGL_TRUE;

out:
    yagl_egl_context_release(ctx);

    return res;
}

void yagl_host_eglSwapBuffers(yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    EGLint error = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    if (!yagl_validate_display(dpy_, &dpy, &error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, &error)) {
        goto out;
    }

    surface->backend_sfc->swap_buffers(surface->backend_sfc);

out:
    yagl_egl_surface_release(surface);
}

void yagl_host_eglCopyBuffers(yagl_host_handle dpy_,
    yagl_host_handle surface_)
{
    EGLint error = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    if (!yagl_validate_display(dpy_, &dpy, &error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, &error)) {
        goto out;
    }

    surface->backend_sfc->copy_buffers(surface->backend_sfc);

out:
    yagl_egl_surface_release(surface);
}

yagl_host_handle yagl_host_eglCreateWindowSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_window_attribs attribs;
    int i = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreateWindowSurfaceOffscreenYAGL);

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

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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
                                                             pixels);

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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

yagl_host_handle yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pbuffer_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglCreatePbufferSurfaceOffscreenYAGL);

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

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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
                                                             pixels);

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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

yagl_host_handle yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pixmap_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreatePixmapSurfaceOffscreenYAGL);

    yagl_egl_pixmap_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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
                                                             pixels);

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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

EGLBoolean yagl_host_eglResizeOffscreenSurfaceYAGL(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_eglb_surface *draw_sfc = NULL;
    struct yagl_eglb_surface *read_sfc = NULL;

    YAGL_LOG_FUNC_SET(eglResizeOffscreenSurfaceYAGL);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, error)) {
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
                                                             pixels);

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

    res = EGL_TRUE;

out:
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }
    yagl_egl_surface_release(surface);

    return res;
}

yagl_host_handle yagl_host_eglCreateWindowSurfaceOnscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id win,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_window_attribs attribs;
    int i = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreateWindowSurfaceOnscreenYAGL);

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

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

yagl_host_handle yagl_host_eglCreatePbufferSurfaceOnscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id buffer,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pbuffer_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;
    int i = 0;

    YAGL_LOG_FUNC_SET(eglCreatePbufferSurfaceOnscreenYAGL);

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

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

yagl_host_handle yagl_host_eglCreatePixmapSurfaceOnscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_winsys_id pixmap,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error)
{
    yagl_host_handle res = 0;
    struct yagl_eglb_surface *backend_sfc = NULL;
    struct yagl_egl_pixmap_attribs attribs;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_config *config = NULL;
    struct yagl_egl_surface *surface = NULL;

    YAGL_LOG_FUNC_SET(eglCreatePixmapSurfaceOnscreenYAGL);

    yagl_egl_pixmap_attribs_init(&attribs);

    if (!yagl_egl_is_attrib_list_empty(attrib_list)) {
        YAGL_SET_ERR(EGL_BAD_ATTRIBUTE);
        goto out;
    }

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    if (!yagl_validate_config(dpy, config_, &config, error)) {
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

    res = surface->res.handle;

out:
    yagl_egl_config_release(config);
    if (backend_sfc) {
        backend_sfc->destroy(backend_sfc);
    }

    return res;
}

void yagl_host_eglInvalidateOnscreenSurfaceYAGL(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    yagl_winsys_id buffer)
{
    EGLint error = 0;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_egl_surface *surface = NULL;

    if (!yagl_validate_display(dpy_, &dpy, &error)) {
        goto out;
    }

    if (!yagl_validate_surface(dpy, surface_, &surface, &error)) {
        goto out;
    }

    surface->backend_sfc->invalidate(surface->backend_sfc, buffer);

out:
    yagl_egl_surface_release(surface);
}

EGLBoolean yagl_host_eglCreateImageYAGL(uint32_t texture,
    yagl_host_handle dpy_,
    yagl_winsys_id buffer,
    EGLint *error)
{
    EGLBoolean res = EGL_FALSE;
    struct yagl_egl_display *dpy = NULL;
    struct yagl_object *image;

    YAGL_LOG_FUNC_SET(eglCreateImageYAGL);

    if (!yagl_validate_display(dpy_, &dpy, error)) {
        goto out;
    }

    image = dpy->backend_dpy->create_image(dpy->backend_dpy, buffer);

    if (!image) {
        YAGL_SET_ERR(EGL_BAD_ALLOC);
        goto out;
    }

    yagl_object_map_add(cur_ts->ps->object_map, texture, image);

    res = EGL_TRUE;

out:
    return res;
}
