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

#ifndef _QEMU_YAGL_SERVER_H
#define _QEMU_YAGL_SERVER_H

#include "yagl_types.h"
#include "qemu/queue.h"

struct yagl_api;
struct yagl_process_state;
struct yagl_egl_backend;
struct yagl_gles_driver;
struct work_queue;
struct winsys_interface;

struct yagl_server_state
{
    yagl_render_type render_type;

    yagl_gl_version gl_version;

    struct yagl_api *apis[YAGL_NUM_APIS];

    QLIST_HEAD(, yagl_process_state) processes;

    struct work_queue *render_queue;

    struct winsys_interface *wsi;
};

/*
 * Create/destroy YaGL server state. Typically, there should be one and only
 * server state which is a part of YaGL device.
 * @{
 */

/*
 * 'egl_backend' and 'gles_driver' will be owned by
 * returned server state or destroyed in case of error.
 */
struct yagl_server_state
    *yagl_server_state_create(struct yagl_egl_backend *egl_backend,
                              struct yagl_gles_driver *gles_driver,
                              struct work_queue *render_queue,
                              struct winsys_interface *wsi);

void yagl_server_state_destroy(struct yagl_server_state *ss);
/*
 * @}
 */

/*
 * Don't destroy the state, just drop all processes/threads.
 */
void yagl_server_reset(struct yagl_server_state *ss);

/*
 * This is called for first YaGL call.
 */
bool yagl_server_dispatch_init(struct yagl_server_state *ss,
                               uint8_t *buff,
                               yagl_pid *target_pid,
                               yagl_tid *target_tid);

/*
 * This is called for each YaGL transport buffer update.
 */
void yagl_server_dispatch_update(struct yagl_server_state *ss,
                                 yagl_pid target_pid,
                                 yagl_tid target_tid,
                                 uint8_t *buff);

/*
 * This is called for each YaGL batch.
 */
void yagl_server_dispatch_batch(struct yagl_server_state *ss,
                                yagl_pid target_pid,
                                yagl_tid target_tid,
                                bool sync);

/*
 * This is called for last YaGL call.
 */
void yagl_server_dispatch_exit(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid);

#endif
