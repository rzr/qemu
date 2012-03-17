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


static emulator_config_info _emul_info;
static emulator_config_state _emul_state;

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
void set_emul_win_scale(int scale)
{
    _emul_state.scale = scale;
    INFO("emulator window scale = %d\n", _emul_state.scale);
}

int get_emul_win_scale(void)
{
    return _emul_state.scale;
}

/* emulator rotation */
void set_emul_rotation(short rotation)
{
    _emul_state.rotation = rotation;
    INFO("emulator rotation = %d\n", _emul_state.rotation);
}

short get_emul_rotation(void)
{
    return _emul_state.rotation;
}
