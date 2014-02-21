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

#include "yagl_egl_context.h"
#include "yagl_egl_surface.h"
#include "yagl_egl_display.h"
#include "yagl_egl_config.h"
#include "yagl_eglb_context.h"
#include "yagl_eglb_display.h"

static void yagl_egl_context_destroy(struct yagl_ref *ref)
{
    struct yagl_egl_context *ctx = (struct yagl_egl_context*)ref;

    assert(!ctx->draw);
    assert(!ctx->read);

    ctx->backend_ctx->destroy(ctx->backend_ctx);

    yagl_egl_config_release(ctx->cfg);

    yagl_resource_cleanup(&ctx->res);

    g_free(ctx);
}

struct yagl_egl_context
    *yagl_egl_context_create(struct yagl_egl_display *dpy,
                             struct yagl_egl_config *cfg,
                             struct yagl_eglb_context *backend_share_ctx,
                             int version)
{
    struct yagl_eglb_context *backend_ctx;
    struct yagl_egl_context *ctx;

    backend_ctx = dpy->backend_dpy->create_context(dpy->backend_dpy,
                                                   &cfg->native,
                                                   backend_share_ctx,
                                                   version);

    if (!backend_ctx) {
        return NULL;
    }

    ctx = g_malloc0(sizeof(*ctx));

    yagl_resource_init(&ctx->res, &yagl_egl_context_destroy);

    ctx->dpy = dpy;
    yagl_egl_config_acquire(cfg);
    ctx->cfg = cfg;
    ctx->backend_ctx = backend_ctx;
    ctx->read = NULL;
    ctx->draw = NULL;

    return ctx;
}

void yagl_egl_context_update_surfaces(struct yagl_egl_context *ctx,
                                      struct yagl_egl_surface *draw,
                                      struct yagl_egl_surface *read)
{
    struct yagl_egl_surface *tmp_draw, *tmp_read;

    yagl_egl_surface_acquire(draw);
    yagl_egl_surface_acquire(read);

    tmp_draw = ctx->draw;
    tmp_read = ctx->read;

    ctx->draw = draw;
    ctx->read = read;

    yagl_egl_surface_release(tmp_draw);
    yagl_egl_surface_release(tmp_read);
}

void yagl_egl_context_acquire(struct yagl_egl_context *ctx)
{
    if (ctx) {
        yagl_resource_acquire(&ctx->res);
    }
}

void yagl_egl_context_release(struct yagl_egl_context *ctx)
{
    if (ctx) {
        yagl_resource_release(&ctx->res);
    }
}
