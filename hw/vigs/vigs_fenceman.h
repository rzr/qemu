/*
 * vigs
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

#ifndef _QEMU_VIGS_FENCEMAN_H
#define _QEMU_VIGS_FENCEMAN_H

#include "vigs_types.h"
#include "qemu/queue.h"
#include "qemu/thread.h"

struct vigs_fence_ack
{
    QTAILQ_ENTRY(vigs_fence_ack) entry;

    uint32_t lower;
    uint32_t upper;
};

struct vigs_fenceman
{
    QemuMutex mutex;

    QTAILQ_HEAD(, vigs_fence_ack) acks;

    uint32_t last_upper;
};

struct vigs_fenceman *vigs_fenceman_create(void);

void vigs_fenceman_destroy(struct vigs_fenceman *fenceman);

void vigs_fenceman_reset(struct vigs_fenceman *fenceman);

void vigs_fenceman_ack(struct vigs_fenceman *fenceman, uint32_t fence_seq);

uint32_t vigs_fenceman_get_lower(struct vigs_fenceman *fenceman);

uint32_t vigs_fenceman_get_upper(struct vigs_fenceman *fenceman);

bool vigs_fenceman_pending(struct vigs_fenceman *fenceman);

#endif
