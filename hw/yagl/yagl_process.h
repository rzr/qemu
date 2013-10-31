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

#ifndef _QEMU_YAGL_PROCESS_H
#define _QEMU_YAGL_PROCESS_H

#include "yagl_types.h"
#include "qemu/queue.h"

struct yagl_server_state;
struct yagl_thread_state;
struct yagl_object_map;
struct yagl_api_ps;
struct yagl_egl_interface;

struct yagl_process_state
{
    struct yagl_server_state *ss;

    QLIST_ENTRY(yagl_process_state) entry;

    yagl_pid id;

    struct yagl_api_ps *api_states[YAGL_NUM_APIS];

    struct yagl_egl_interface *egl_iface;

    struct yagl_object_map *object_map;

    QLIST_HEAD(, yagl_thread_state) threads;

#ifdef CONFIG_KVM
    target_ulong cr[5];
#endif
};

struct yagl_process_state
    *yagl_process_state_create(struct yagl_server_state *ss,
                               yagl_pid id);

void yagl_process_state_destroy(struct yagl_process_state *ps);

void yagl_process_register_egl_interface(struct yagl_process_state *ps,
                                         struct yagl_egl_interface *egl_iface);

void yagl_process_unregister_egl_interface(struct yagl_process_state *ps);

struct yagl_thread_state*
    yagl_process_find_thread(struct yagl_process_state *ps,
                            yagl_tid id);

struct yagl_thread_state*
    yagl_process_add_thread(struct yagl_process_state *ps,
                            yagl_tid id);

void yagl_process_remove_thread(struct yagl_process_state *ps,
                                yagl_tid id);

bool yagl_process_has_threads(struct yagl_process_state *ps);

#endif
