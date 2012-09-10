#include "yagl_egl_display.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_config.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_context.h"
#include "yagl_process.h"
#include "yagl_log.h"
#include "yagl_handle_gen.h"

struct yagl_egl_display
    *yagl_egl_display_create(struct yagl_egl_driver_ps *driver_ps,
                             target_ulong display_id)
{
    EGLNativeDisplayType native_dpy;
    struct yagl_egl_display *dpy;

    native_dpy = driver_ps->display_create(driver_ps);

    if (!native_dpy) {
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    dpy->driver_ps = driver_ps;
    dpy->display_id = display_id;
    dpy->handle = yagl_handle_gen();
    dpy->native_dpy = native_dpy;

    qemu_mutex_init(&dpy->mutex);

    dpy->initialized = false;

    yagl_resource_list_init(&dpy->configs);
    yagl_resource_list_init(&dpy->contexts);
    yagl_resource_list_init(&dpy->surfaces);

    return dpy;
}

void yagl_egl_display_destroy(struct yagl_egl_display *dpy)
{
    yagl_egl_display_terminate(dpy);

    yagl_resource_list_cleanup(&dpy->surfaces);
    yagl_resource_list_cleanup(&dpy->contexts);
    yagl_resource_list_cleanup(&dpy->configs);

    qemu_mutex_destroy(&dpy->mutex);

    dpy->driver_ps->display_destroy(dpy->driver_ps, dpy->native_dpy);

    g_free(dpy);
}

void yagl_egl_display_initialize(struct yagl_egl_display *dpy)
{
    struct yagl_egl_config **cfgs;
    int i, num_configs = 0;

    YAGL_LOG_FUNC_ENTER(dpy->driver_ps->ps->id, 0, yagl_egl_display_initialize, NULL);

    qemu_mutex_lock(&dpy->mutex);

    if (dpy->initialized) {
        goto out;
    }

    cfgs = yagl_egl_config_enum(dpy, &num_configs);

    if (num_configs <= 0) {
        goto out;
    }

    yagl_egl_config_sort(cfgs, num_configs);

    for (i = 0; i < num_configs; ++i) {
        yagl_resource_list_add(&dpy->configs, &cfgs[i]->res);
        yagl_egl_config_release(cfgs[i]);
    }

    g_free(cfgs);

    YAGL_LOG_TRACE("initialized with %d configs", num_configs);

out:
    dpy->initialized = true;

    qemu_mutex_unlock(&dpy->mutex);

    YAGL_LOG_FUNC_EXIT(NULL);
}

bool yagl_egl_display_is_initialized(struct yagl_egl_display *dpy)
{
    bool ret;

    qemu_mutex_lock(&dpy->mutex);

    ret = dpy->initialized;

    qemu_mutex_unlock(&dpy->mutex);

    return ret;
}

void yagl_egl_display_terminate(struct yagl_egl_display *dpy)
{
    struct yagl_resource_list tmp_list;

    /*
     * First, move all resources to a temporary list to release later.
     */

    yagl_resource_list_init(&tmp_list);

    qemu_mutex_lock(&dpy->mutex);

    yagl_resource_list_move(&dpy->surfaces, &tmp_list);
    yagl_resource_list_move(&dpy->contexts, &tmp_list);
    yagl_resource_list_move(&dpy->configs, &tmp_list);

    dpy->initialized = false;

    qemu_mutex_unlock(&dpy->mutex);

    /*
     * We release here because we don't want the resources to be released
     * when display mutex is held.
     */

    yagl_resource_list_cleanup(&tmp_list);
}

int yagl_egl_display_get_config_count(struct yagl_egl_display *dpy)
{
    int ret;

    qemu_mutex_lock(&dpy->mutex);

    ret = yagl_resource_list_get_count(&dpy->configs);

    qemu_mutex_unlock(&dpy->mutex);

    return ret;
}

yagl_host_handle
    *yagl_egl_display_get_config_handles(struct yagl_egl_display *dpy,
                                         int *num_configs)
{
    yagl_host_handle *handles = g_malloc(*num_configs * sizeof(yagl_host_handle));
    struct yagl_resource *res;
    int i = 0;

    qemu_mutex_lock(&dpy->mutex);

    QTAILQ_FOREACH(res, &dpy->configs.resources, entry) {
        if (i >= *num_configs) {
            break;
        }
        handles[i] = res->handle;
        ++i;
    }

    qemu_mutex_unlock(&dpy->mutex);

    *num_configs = i;

    return handles;
}

yagl_host_handle
    *yagl_egl_display_choose_configs(struct yagl_egl_display *dpy,
                                     const struct yagl_egl_native_config *dummy,
                                     int *num_configs,
                                     bool count_only)
{
    yagl_host_handle *handles = NULL;
    struct yagl_resource *res;
    int i = 0;

    if (!count_only) {
        handles = g_malloc(*num_configs * sizeof(yagl_host_handle));
    }

    qemu_mutex_lock(&dpy->mutex);

    QTAILQ_FOREACH(res, &dpy->configs.resources, entry) {
        if (!count_only && (i >= *num_configs)) {
            break;
        }
        if (yagl_egl_config_is_chosen_by((struct yagl_egl_config*)res, dummy)) {
            if (!count_only) {
                handles[i] = res->handle;
            }
            ++i;
        }
    }

    qemu_mutex_unlock(&dpy->mutex);

    *num_configs = i;

    return handles;
}

struct yagl_egl_config
    *yagl_egl_display_acquire_config(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle)
{
    struct yagl_egl_config *cfg;

    qemu_mutex_lock(&dpy->mutex);

    cfg = (struct yagl_egl_config*)yagl_resource_list_acquire(&dpy->configs, handle);

    qemu_mutex_unlock(&dpy->mutex);

    return cfg;
}

struct yagl_egl_config
    *yagl_egl_display_acquire_config_by_id(struct yagl_egl_display *dpy,
                                           EGLint config_id)
{
    struct yagl_resource *res;

    qemu_mutex_lock(&dpy->mutex);

    QTAILQ_FOREACH(res, &dpy->configs.resources, entry) {
        if (((struct yagl_egl_config*)res)->native.config_id == config_id) {
            yagl_resource_acquire(res);

            qemu_mutex_unlock(&dpy->mutex);

            return (struct yagl_egl_config*)res;
        }
    }

    qemu_mutex_unlock(&dpy->mutex);

    return NULL;
}

void yagl_egl_display_add_context(struct yagl_egl_display *dpy,
                                  struct yagl_egl_context *ctx)
{
    qemu_mutex_lock(&dpy->mutex);

    yagl_resource_list_add(&dpy->contexts, &ctx->res);

    qemu_mutex_unlock(&dpy->mutex);
}

struct yagl_egl_context
    *yagl_egl_display_acquire_context(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle)
{
    struct yagl_egl_context *ctx;

    qemu_mutex_lock(&dpy->mutex);

    ctx = (struct yagl_egl_context*)yagl_resource_list_acquire(&dpy->contexts, handle);

    qemu_mutex_unlock(&dpy->mutex);

    return ctx;
}

bool yagl_egl_display_remove_context(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle)
{
    bool res;

    qemu_mutex_lock(&dpy->mutex);

    res = yagl_resource_list_remove(&dpy->contexts, handle);

    qemu_mutex_unlock(&dpy->mutex);

    return res;
}

void yagl_egl_display_add_surface(struct yagl_egl_display *dpy,
                                  struct yagl_egl_surface *sfc)
{
    qemu_mutex_lock(&dpy->mutex);

    yagl_resource_list_add(&dpy->surfaces, &sfc->res);

    qemu_mutex_unlock(&dpy->mutex);
}

struct yagl_egl_surface
    *yagl_egl_display_acquire_surface(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle)
{
    struct yagl_egl_surface *sfc;

    qemu_mutex_lock(&dpy->mutex);

    sfc = (struct yagl_egl_surface*)yagl_resource_list_acquire(&dpy->surfaces, handle);

    qemu_mutex_unlock(&dpy->mutex);

    return sfc;
}

bool yagl_egl_display_remove_surface(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle)
{
    bool res;

    qemu_mutex_lock(&dpy->mutex);

    res = yagl_resource_list_remove(&dpy->surfaces, handle);

    qemu_mutex_unlock(&dpy->mutex);

    return res;
}
