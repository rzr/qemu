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

#include "vigs_fenceman.h"
#include "vigs_log.h"

static __inline uint32_t vigs_fenceman_seq_next(uint32_t seq)
{
    if (++seq == 0) {
        ++seq;
    }
    return seq;
}

static __inline uint32_t vigs_fenceman_seq_prev(uint32_t seq)
{
    if (--seq == 0) {
        --seq;
    }
    return seq;
}

static __inline bool vigs_fenceman_seq_before(uint32_t a, uint32_t b)
{
    return ((int32_t)a - (int32_t)b) < 0;
}

struct vigs_fenceman *vigs_fenceman_create(void)
{
    struct vigs_fenceman *fenceman;

    fenceman = g_malloc0(sizeof(*fenceman));

    qemu_mutex_init(&fenceman->mutex);
    QTAILQ_INIT(&fenceman->acks);

    return fenceman;
}

void vigs_fenceman_destroy(struct vigs_fenceman *fenceman)
{
    vigs_fenceman_reset(fenceman);
    qemu_mutex_destroy(&fenceman->mutex);
    g_free(fenceman);
}

void vigs_fenceman_reset(struct vigs_fenceman *fenceman)
{
    struct vigs_fence_ack *ack, *tmp;

    qemu_mutex_lock(&fenceman->mutex);

    QTAILQ_FOREACH_SAFE(ack, &fenceman->acks, entry, tmp) {
        QTAILQ_REMOVE(&fenceman->acks, ack, entry);
        g_free(ack);
    }

    qemu_mutex_unlock(&fenceman->mutex);
}

void vigs_fenceman_ack(struct vigs_fenceman *fenceman, uint32_t fence_seq)
{
    struct vigs_fence_ack *ack, *tmp;

    VIGS_LOG_TRACE("Fence ack %u", fence_seq);

    qemu_mutex_lock(&fenceman->mutex);

    QTAILQ_FOREACH(ack, &fenceman->acks, entry) {
        uint32_t upper_p1 = vigs_fenceman_seq_next(ack->upper);

        if (upper_p1 == fence_seq) {
            ack->upper = fence_seq;

            tmp = QTAILQ_NEXT(ack, entry);

            if (tmp && (tmp->lower == fence_seq)) {
                ack->upper = tmp->upper;
                QTAILQ_REMOVE(&fenceman->acks, tmp, entry);
                g_free(tmp);
            }

            goto out;
        } else if (vigs_fenceman_seq_before(fence_seq, upper_p1)) {
            uint32_t lower_m1 = vigs_fenceman_seq_prev(ack->lower);

            if (lower_m1 == fence_seq) {
                ack->lower = fence_seq;
            } else if (vigs_fenceman_seq_before(fence_seq, lower_m1)) {
                tmp = g_malloc(sizeof(*tmp));

                QTAILQ_INSERT_BEFORE(ack, tmp, entry);

                tmp->lower = tmp->upper = fence_seq;
            } else {
                VIGS_LOG_ERROR("Duplicate fence ack %u", fence_seq);
            }

            goto out;
        }
    }

    tmp = g_malloc(sizeof(*tmp));

    QTAILQ_INSERT_TAIL(&fenceman->acks, tmp, entry);

    tmp->lower = tmp->upper = fence_seq;

out:
    qemu_mutex_unlock(&fenceman->mutex);
}

uint32_t vigs_fenceman_get_lower(struct vigs_fenceman *fenceman)
{
    struct vigs_fence_ack *ack;
    uint32_t lower = 0;

    qemu_mutex_lock(&fenceman->mutex);

    ack = QTAILQ_FIRST(&fenceman->acks);

    if (ack) {
        lower = ack->lower;
        fenceman->last_upper = ack->upper;

        QTAILQ_REMOVE(&fenceman->acks, ack, entry);
        g_free(ack);
    }

    qemu_mutex_unlock(&fenceman->mutex);

    return lower;
}

uint32_t vigs_fenceman_get_upper(struct vigs_fenceman *fenceman)
{
    uint32_t upper = 0;

    qemu_mutex_lock(&fenceman->mutex);

    upper = fenceman->last_upper;

    qemu_mutex_unlock(&fenceman->mutex);

    return upper;
}

bool vigs_fenceman_pending(struct vigs_fenceman *fenceman)
{
    bool empty;

    qemu_mutex_lock(&fenceman->mutex);

    empty = QTAILQ_EMPTY(&fenceman->acks);

    qemu_mutex_unlock(&fenceman->mutex);

    return !empty;
}
