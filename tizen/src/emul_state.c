/*
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
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


#include "emul_state.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, emul_state);


static EmulatorConfigInfo _emul_info;
static EmulatorConfigState _emul_state;


/* current emulator condition */
int get_emulator_condition(void)
{
    return _emul_state.emulator_condition;
}

void set_emulator_condition(int state)
{
    _emul_state.emulator_condition = state;
}

/* lcd screen size */
void set_emul_lcd_size(int width, int height)
{
    _emul_info.lcd_size_w = width;
    _emul_info.lcd_size_h = height;

   INFO("emulator graphic resolution = %dx%d\n", _emul_info.lcd_size_w,  _emul_info.lcd_size_h);
}

int get_emul_lcd_width(void)
{
    return _emul_info.lcd_size_w;
}

int get_emul_lcd_height(void)
{
    return _emul_info.lcd_size_h;
}

/* emulator window scale */
void set_emul_win_scale(double scale_factor)
{
    if (scale_factor < 0.0 || scale_factor > 1.0) {
        INFO("scale_factor is out of range = %lf\n", scale_factor);
        scale_factor = 1.0;
    }

    _emul_state.scale_factor = scale_factor;
    INFO("emulator window scale_factor = %lf\n", _emul_state.scale_factor);
}

double get_emul_win_scale(void)
{
    return _emul_state.scale_factor;
}

/* emulator rotation */
void set_emul_rotation(short rotation_type)
{
    if (rotation_type < ROTATION_PORTRAIT || rotation_type > ROTATION_REVERSE_LANDSCAPE) {
        INFO("rotation type is out of range = %d\n", rotation_type);
        rotation_type = ROTATION_PORTRAIT;
    }

    _emul_state.rotation_type = rotation_type;
    INFO("emulator rotation type = %d\n", _emul_state.rotation_type);
}

short get_emul_rotation(void)
{
    return _emul_state.rotation_type;
}

/* emulator multi-touch */
MultiTouchState *get_emul_multi_touch_state(void)
{
    return &(_emul_state.qemu_mts);
}
