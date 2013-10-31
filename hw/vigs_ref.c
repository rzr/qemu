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

#include "vigs_ref.h"

void vigs_ref_init(struct vigs_ref *ref, vigs_ref_destroy_func destroy)
{
    assert(ref);
    assert(destroy);

    memset(ref, 0, sizeof(*ref));

    ref->destroy = destroy;
    ref->count = 1;
}

void vigs_ref_cleanup(struct vigs_ref *ref)
{
    assert(ref);
    assert(!ref->count);
}

void vigs_ref_acquire(struct vigs_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    ++ref->count;
}

void vigs_ref_release(struct vigs_ref *ref)
{
    assert(ref);
    assert(ref->count > 0);

    if (--ref->count == 0) {
        assert(ref->destroy);
        ref->destroy(ref);
    }
}
