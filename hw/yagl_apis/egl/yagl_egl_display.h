#ifndef _QEMU_YAGL_EGL_DISPLAY_H
#define _QEMU_YAGL_EGL_DISPLAY_H

#include "yagl_types.h"
#include "yagl_resource_list.h"
#include "qemu-thread.h"
#include "qemu-queue.h"
#include <EGL/egl.h>

struct yagl_egl_driver_ps;
struct yagl_egl_config;
struct yagl_egl_native_config;
struct yagl_egl_surface;
struct yagl_egl_context;

struct yagl_egl_display
{
    /*
     * Don't need to lock these.
     * @{
     */

    QLIST_ENTRY(yagl_egl_display) entry;

    struct yagl_egl_driver_ps *driver_ps;

    target_ulong display_id;

    yagl_host_handle handle;

    EGLNativeDisplayType native_dpy;

    /*
     * @}
     */

    QemuMutex mutex;

    bool initialized;

    struct yagl_resource_list configs;

    struct yagl_resource_list contexts;

    struct yagl_resource_list surfaces;
};

struct yagl_egl_display
    *yagl_egl_display_create(struct yagl_egl_driver_ps *driver_ps,
                             target_ulong display_id);

void yagl_egl_display_destroy(struct yagl_egl_display *dpy);

void yagl_egl_display_initialize(struct yagl_egl_display *dpy);

bool yagl_egl_display_is_initialized(struct yagl_egl_display *dpy);

void yagl_egl_display_terminate(struct yagl_egl_display *dpy);

/*
 * Configs.
 * @{
 */

int yagl_egl_display_get_config_count(struct yagl_egl_display *dpy);

/*
 * Gets at most '*num_configs' config handles and updates
 * 'num_configs' with number of configs actually returned.
 * The configs themselves are not acquired.
 * Use 'g_free' to free up the returned handles array.
 */
yagl_host_handle
    *yagl_egl_display_get_config_handles(struct yagl_egl_display *dpy,
                                         int *num_configs);

/*
 * Gets at most '*num_configs' config handles and updates
 * 'num_configs' with number of configs actually returned.
 * The configs themselves are not acquired.
 * Use 'g_free' to free up the returned handles array.
 *
 * 'dummy' is used for matching, only handles of configs that matched
 * 'dummy' will be returned. if 'count_only' is true then no handles are
 * returned (return value is NULL), but 'num_configs' is still updated.
 */
yagl_host_handle
    *yagl_egl_display_choose_configs(struct yagl_egl_display *dpy,
                                     const struct yagl_egl_native_config *dummy,
                                     int *num_configs,
                                     bool count_only);

struct yagl_egl_config
    *yagl_egl_display_acquire_config(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

struct yagl_egl_config
    *yagl_egl_display_acquire_config_by_id(struct yagl_egl_display *dpy,
                                           EGLint config_id);

/*
 * @}
 */

/*
 * Contexts.
 * @{
 */

/*
 * This acquires 'ctx', so the caller should
 * release 'ctx' if he doesn't want to use it and wants it to belong to the
 * display alone.
 */
void yagl_egl_display_add_context(struct yagl_egl_display *dpy,
                                  struct yagl_egl_context *ctx);

struct yagl_egl_context
    *yagl_egl_display_acquire_context(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle);

bool yagl_egl_display_remove_context(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

/*
 * @}
 */

/*
 * Surfaces.
 * @{
 */

/*
 * This acquires 'sfc', so the caller should
 * release 'sfc' if he doesn't want to use it and wants it to belong to the
 * display alone.
 */
void yagl_egl_display_add_surface(struct yagl_egl_display *dpy,
                                  struct yagl_egl_surface *sfc);

struct yagl_egl_surface
    *yagl_egl_display_acquire_surface(struct yagl_egl_display *dpy,
                                      yagl_host_handle handle);

bool yagl_egl_display_remove_surface(struct yagl_egl_display *dpy,
                                     yagl_host_handle handle);

/*
 * @}
 */

#endif
