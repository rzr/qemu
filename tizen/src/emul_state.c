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


#include "maru_common.h"
#include "emul_state.h"
#include "debug_ch.h"

#if defined( __linux__)
#include <X11/XKBlib.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

MULTI_DEBUG_CHANNEL(qemu, emul_state);


static EmulatorConfigInfo _emul_info;
static EmulatorConfigState _emul_state;

/* start_skin_client or not ? */
void set_emul_skin_enable(int enable)
{
    _emul_info.skin_enable = enable;
}

int get_emul_skin_enable(void)
{
    return _emul_info.skin_enable;
}

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

   //INFO("emulator graphic resolution = %dx%d\n", _emul_info.lcd_size_w,  _emul_info.lcd_size_h);
}

int get_emul_lcd_width(void)
{
    return _emul_info.lcd_size_w;
}

int get_emul_lcd_height(void)
{
    return _emul_info.lcd_size_h;
}

/* sdl bits per pixel */
void set_emul_sdl_bpp(int bpp)
{
    _emul_info.sdl_bpp = bpp;

    if (_emul_info.sdl_bpp != 32) {
        INFO("?? sdl bpp = %d\n", _emul_info.sdl_bpp);
    }
}

int get_emul_sdl_bpp(void)
{
    return _emul_info.sdl_bpp;
}

/* maximum number of touch point */
void set_emul_max_touch_point(int cnt)
{
    if (cnt <= 0) {
        cnt = 1;
    }
    _emul_info.max_touch_point = cnt;
}

int get_emul_max_touch_point(void)
{
    if (_emul_info.max_touch_point <= 0) {
        set_emul_max_touch_point(1);
    }
    return _emul_info.max_touch_point;
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

/* retrieves the status of the host lock key */
int get_host_lock_key_state(int key)
{
    /* support only capslock, numlock */

#if defined(CONFIG_LINUX)
    unsigned state = 0;
    Display *display = XOpenDisplay((char*)0);
    if (display) {
        XkbGetIndicatorState(display, XkbUseCoreKbd, &state);
    }
    XCloseDisplay(display);

    if (key == HOST_CAPSLOCK_KEY) {
        return (state & 0x01) != 0;
    } else if (key == HOST_NUMLOCK_KEY) {
        return (state & 0x02) != 0;
    }

    return -1;

#elif defined(CONFIG_WIN32)
    if (key == HOST_CAPSLOCK_KEY) {
        return (GetKeyState(VK_CAPITAL) & 1) != 0;
    } else if (key == HOST_NUMLOCK_KEY) {
        return (GetKeyState(VK_NUMLOCK) & 1) != 0;
    }

    return -1;

#elif defined(CONFIG_DARWIN)
    //TODO:
#endif

    return 0;
}

/* manage CapsLock key state for usb keyboard input */
void set_emul_caps_lock_state(int state)
{
    _emul_state.qemu_caps_lock = state;
}

int get_emul_caps_lock_state(void)
{
    return  _emul_state.qemu_caps_lock;
}

/* manage NumLock key state for usb keyboard input */
void set_emul_num_lock_state(int state)
{
    _emul_state.qemu_num_lock = state;
}

int get_emul_num_lock_state(void)
{
    return  _emul_state.qemu_num_lock;
}


