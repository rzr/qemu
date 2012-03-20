/*
 * Multi-touch processing
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * HyunJun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
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


#include <glib.h>
#include "maru_finger.h"
#include "emul_state.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, maru_finger);


void init_multi_touch_state(void)
{
    int i;
    MultiTouchState* mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;
    INFO("multi-touch state initailize\n");

    mts->multitouch_enable = 0;
    mts->finger_cnt_max = MAX_FINGER_CNT; //temp
    mts->finger_cnt = 0;

    if (mts->finger_slot != NULL) {
        g_free(mts->finger_slot);
        mts->finger_slot = NULL;
    }
    mts->finger_slot = (FingerPoint *)g_malloc0(sizeof(FingerPoint) * mts->finger_cnt_max);
    for (i = 0; i < mts->finger_cnt; i++) {
        finger = get_finger_point_from_slot(i);
        //finger->id = 0;
        finger->x = finger->y = -1;
    }

    mts->finger_point_size = DEFAULT_FINGER_POINT_SIZE; //temp
    mts->finger_point_color = DEFAULT_FINGER_POINT_COLOR; //temp
    mts->finger_point_outline_color = DEFAULT_FINGER_POINT_OUTLINE_COLOR; //temp
}

void set_multi_touch_enable(int enable)
{
    get_emul_multi_touch_state()->multitouch_enable = enable;
}

int get_multi_touch_enable(void)
{
    return get_emul_multi_touch_state()->multitouch_enable;
}

int add_finger_point(int x, int y)
{
    MultiTouchState *mts = get_emul_multi_touch_state();
    
    if (mts->finger_cnt == mts->finger_cnt_max) {
        return -1;
    }

    mts->finger_cnt++;

    mts->finger_slot[mts->finger_cnt - 1].id = mts->finger_cnt;
    mts->finger_slot[mts->finger_cnt - 1].x = x;
    mts->finger_slot[mts->finger_cnt - 1].y = y;
    INFO("%d finger touching\n", mts->finger_cnt);

    return mts->finger_cnt;
}

FingerPoint *get_finger_point_from_slot(int index)
{
    MultiTouchState *mts = get_emul_multi_touch_state();

    if (index < 0 || index > mts->finger_cnt_max) {
        return NULL;
    }

    return &(mts->finger_slot[index]);
}

void clear_finger_slot(void)
{
    int i;
    MultiTouchState *mts = get_emul_multi_touch_state();
    FingerPoint *finger = NULL;

    for (i = 0; i < mts->finger_cnt; i++) {
        finger = get_finger_point_from_slot(i);
        finger->id = 0;
        finger->x = finger->y = -1;
    }
}
