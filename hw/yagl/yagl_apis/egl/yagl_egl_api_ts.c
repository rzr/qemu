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

#include "yagl_egl_api_ts.h"
#include "yagl_egl_api_ps.h"
#include "yagl_egl_context.h"
#include "yagl_eglb_context.h"
#include "yagl_egl_backend.h"

void yagl_egl_api_ts_init(struct yagl_egl_api_ts *egl_api_ts,
                          struct yagl_egl_api_ps *api_ps)
{
    egl_api_ts->api_ps = api_ps;
    egl_api_ts->backend = api_ps->backend;
    egl_api_ts->context = NULL;

    yagl_egl_api_ts_reset(egl_api_ts);
}

void yagl_egl_api_ts_cleanup(struct yagl_egl_api_ts *egl_api_ts)
{
    if (egl_api_ts->context) {
        /*
         * Force release current.
         */
        egl_api_ts->backend->release_current(egl_api_ts->backend, true);

        yagl_egl_context_update_surfaces(egl_api_ts->context, NULL, NULL);
    }

    yagl_egl_api_ts_update_context(egl_api_ts, NULL);
}

void yagl_egl_api_ts_update_context(struct yagl_egl_api_ts *egl_api_ts,
                                    struct yagl_egl_context *ctx)
{
    yagl_egl_context_acquire(ctx);

    yagl_egl_context_release(egl_api_ts->context);

    egl_api_ts->context = ctx;
}

void yagl_egl_api_ts_reset(struct yagl_egl_api_ts *egl_api_ts)
{
    egl_api_ts->api = EGL_OPENGL_ES_API;
}
