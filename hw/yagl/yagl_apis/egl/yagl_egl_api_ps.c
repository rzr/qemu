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

#include "yagl_egl_api_ps.h"
#include "yagl_egl_display.h"
#include "yagl_egl_backend.h"
#include "yagl_process.h"
#include "yagl_thread.h"

void yagl_egl_api_ps_init(struct yagl_egl_api_ps *egl_api_ps,
                          struct yagl_egl_backend *backend,
                          struct yagl_egl_interface *egl_iface)
{
    egl_api_ps->backend = backend;
    egl_api_ps->egl_iface = egl_iface;

    yagl_process_register_egl_interface(cur_ts->ps, egl_api_ps->egl_iface);

    QLIST_INIT(&egl_api_ps->displays);
}

void yagl_egl_api_ps_cleanup(struct yagl_egl_api_ps *egl_api_ps)
{
    struct yagl_egl_display *dpy, *next;

    QLIST_FOREACH_SAFE(dpy, &egl_api_ps->displays, entry, next) {
        QLIST_REMOVE(dpy, entry);
        yagl_egl_display_destroy(dpy);
    }

    assert(QLIST_EMPTY(&egl_api_ps->displays));

    yagl_process_unregister_egl_interface(cur_ts->ps);
}

struct yagl_egl_display *yagl_egl_api_ps_display_get(struct yagl_egl_api_ps *egl_api_ps,
                                                     yagl_host_handle handle)
{
    struct yagl_egl_display *dpy;

    QLIST_FOREACH(dpy, &egl_api_ps->displays, entry) {
        if (dpy->handle == handle) {
            return dpy;
        }
    }

    return NULL;
}

struct yagl_egl_display *yagl_egl_api_ps_display_add(struct yagl_egl_api_ps *egl_api_ps,
                                                     uint32_t display_id)
{
    struct yagl_egl_display *dpy;

    QLIST_FOREACH(dpy, &egl_api_ps->displays, entry) {
        if (dpy->display_id == display_id) {
            return dpy;
        }
    }

    dpy = yagl_egl_display_create(egl_api_ps->backend, display_id);

    if (!dpy) {
        return NULL;
    }

    QLIST_INSERT_HEAD(&egl_api_ps->displays, dpy, entry);

    return dpy;
}
