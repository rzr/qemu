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

void set_emul_lcd_size(int width, int height)
{
    _emul_info.resolution_w = width;
    _emul_info.resolution_h = height;

   INFO("emulator graphic resolution %dx%d\n", _emul_info.resolution_w,  _emul_info.resolution_h);
}

int get_emul_lcd_width(void)
{
    return _emul_info.resolution_w;
}

int get_emul_lcd_height(void)
{
    return _emul_info.resolution_h;
}
