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

#include "yagl_event.h"
#include <assert.h>

void yagl_event_init(struct yagl_event *event,
                     bool manual_reset,
                     bool initial_state)
{
    assert(event);

    memset(event, 0, sizeof(*event));

    event->manual_reset = manual_reset;
    event->state = initial_state;
    qemu_mutex_init(&event->mutex);
    qemu_cond_init(&event->cond);
}

void yagl_event_cleanup(struct yagl_event *event)
{
    assert(event);

    qemu_cond_destroy(&event->cond);
    qemu_mutex_destroy(&event->mutex);
}

void yagl_event_set(struct yagl_event *event)
{
    assert(event);
    qemu_mutex_lock(&event->mutex);
    event->state = true;
    qemu_cond_broadcast(&event->cond);
    qemu_mutex_unlock(&event->mutex);
}

void yagl_event_reset(struct yagl_event *event)
{
    assert(event);
    qemu_mutex_lock(&event->mutex);
    event->state = false;
    qemu_mutex_unlock(&event->mutex);
}

void yagl_event_wait(struct yagl_event *event)
{
    assert(event);
    qemu_mutex_lock(&event->mutex);
    while (!event->state) {
        qemu_cond_wait(&event->cond, &event->mutex);
    }
    if (!event->manual_reset) {
        event->state = false;
    }
    qemu_mutex_unlock(&event->mutex);
}

bool yagl_event_is_set(struct yagl_event *event)
{
    bool ret;
    assert(event);
    qemu_mutex_lock(&event->mutex);
    ret = event->state;
    qemu_mutex_unlock(&event->mutex);
    return ret;
}
