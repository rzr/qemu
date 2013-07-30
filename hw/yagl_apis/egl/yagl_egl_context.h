#ifndef _QEMU_YAGL_EGL_CONTEXT_H
#define _QEMU_YAGL_EGL_CONTEXT_H

#include "yagl_types.h"
#include "yagl_resource.h"
#include <EGL/egl.h>

struct yagl_egl_display;
struct yagl_egl_config;
struct yagl_egl_surface;
struct yagl_client_context;

struct yagl_egl_context
{
    struct yagl_resource res;

    struct yagl_egl_display *dpy;

    struct yagl_egl_config *cfg;

    struct yagl_client_context *client_ctx;

    EGLContext native_ctx;

    struct yagl_egl_surface *read;

    struct yagl_egl_surface *draw;
};

/*
 * Takes ownership of 'client_ctx'.
 */
struct yagl_egl_context
    *yagl_egl_context_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_client_context *client_ctx,
                             EGLContext native_share_ctx);

void yagl_egl_context_update_surfaces(struct yagl_egl_context *ctx,
                                      struct yagl_egl_surface *draw,
                                      struct yagl_egl_surface *read);

bool yagl_egl_context_uses_surface(struct yagl_egl_context *ctx,
                                   struct yagl_egl_surface *sfc);

/*
 * Helper functions that simply acquire/release yagl_egl_context::res
 * @{
 */

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_context_acquire(struct yagl_egl_context *ctx);

/*
 * Passing NULL won't hurt, this is for convenience.
 */
void yagl_egl_context_release(struct yagl_egl_context *ctx);

/*
 * @}
 */

#endif
