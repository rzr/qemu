/*
 * Shared memory
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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


#include "maru_shm.h"
#include "emul_state.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_shm);


void qemu_ds_shm_update(DisplayState *ds, int x, int y, int w, int h)
{
    //TODO:
}

void qemu_ds_shm_resize(DisplayState *ds)
{
    //TODO:
}

void qemu_ds_shm_refresh(DisplayState *ds)
{
    //TODO:
}

void maruskin_shm_init(uint64 swt_handle, int lcd_size_width, int lcd_size_height, bool is_resize)
{
    INFO("maru shm initialization = %d\n", is_resize);

    if (is_resize == FALSE) { //once
        set_emul_lcd_size(lcd_size_width, lcd_size_height);
        set_emul_sdl_bpp(32);
    }
}

