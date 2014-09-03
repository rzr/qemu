/*
 * Emulator State Information
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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
#include "emulator_options.h"
#include "skin/maruskin_server.h"
#include "util/new_debug_ch.h"

#if defined(CONFIG_LINUX)
#include <X11/XKBlib.h>
#elif defined (CONFIG_WIN32)
#include <windows.h>
#endif

DECLARE_DEBUG_CHANNEL(emul_state);

static EmulatorConfigInfo _emul_info = {0,};
static EmulatorConfigState _emul_state;

/* misc */
char bin_path[PATH_MAX] = { 0, };
#ifdef SUPPORT_LEGACY_ARGS
// for compatibility
char log_path[PATH_MAX] = { 0, };
#endif


const char *get_bin_path(void)
{
    return bin_path;
}

const char *get_log_path(void)
{
#ifdef SUPPORT_LEGACY_ARGS
    if (log_path[0]) {
        return log_path;
    }
#endif
    char *log_path = get_variable("log_path");

    // if "log_path" is not exist, make it first
    if (!log_path) {
        char *vms_path = get_variable("vms_path");
        char *vm_name = get_variable("vm_name");
        if (!vms_path || !vm_name) {
            log_path = NULL;
        }
        else {
            log_path = g_strdup_printf("%s/%s/logs", vms_path, vm_name);
        }

        set_variable("log_path", log_path, false);
    }

    return log_path;
}


/* start_skin_client or not ? */
void set_emul_skin_enable(bool enable)
{
    _emul_info.skin_enable = enable;
}

bool get_emul_skin_enable(void)
{
    return _emul_info.skin_enable;
}

/* display screen resolution */
void set_emul_resolution(int width, int height)
{
    _emul_info.resolution_w = width;
    _emul_info.resolution_h = height;

    LOG_INFO("emulator graphic resolution : %dx%d\n",
        _emul_info.resolution_w, _emul_info.resolution_h);
}

int get_emul_resolution_width(void)
{
    return _emul_info.resolution_w;
}

int get_emul_resolution_height(void)
{
    return _emul_info.resolution_h;
}

/* sdl bits per pixel */
void set_emul_sdl_bpp(int bpp)
{
    _emul_info.sdl_bpp = bpp;

    if (_emul_info.sdl_bpp != 32) {
        LOG_INFO("sdl bpp : %d\n", _emul_info.sdl_bpp);
    }
}

int get_emul_sdl_bpp(void)
{
    return _emul_info.sdl_bpp;
}

/* mouse device */
void set_emul_input_mouse_enable(bool on)
{
    _emul_info.input_mouse_enable = on;

    if (_emul_info.input_mouse_enable == true) {
        LOG_INFO("set_emul_input_mouse_enable\n");
    }
}

bool is_emul_input_mouse_enable(void)
{
    return _emul_info.input_mouse_enable;
}

/* touchscreen device */
void set_emul_input_touch_enable(bool on)
{
    _emul_info.input_touch_enable = on;

    if (_emul_info.input_touch_enable == true) {
        LOG_INFO("set_emul_input_touch_enable\n");
    }
}

bool is_emul_input_touch_enable(void)
{
    return _emul_info.input_touch_enable;
}

/* maximum number of touch point */
void set_emul_max_touch_point(int cnt)
{
    if (cnt <= 0) {
        cnt = 1;
    }
    _emul_info.max_touch_point = cnt;

    LOG_INFO("set max touch point : %d\n", cnt);
}

int get_emul_max_touch_point(void)
{
    if (_emul_info.max_touch_point <= 0) {
        set_emul_max_touch_point(1);
    }
    return _emul_info.max_touch_point;
}

/* base port for emualtor vm */
void set_emul_vm_base_port(int port)
{
    _emul_info.vm_base_port = port;
    _emul_info.device_serial_number = port + 1;
    _emul_info.ecs_port = port + 3;
    _emul_info.serial_port = port + 4;
    _emul_info.spice_port = port + 5;
}

int get_emul_spice_port(void)
{
    return _emul_info.spice_port;
}

void set_emul_ecs_port(int port)
{
    _emul_info.ecs_port = port;
}

int get_emul_vm_base_port(void)
{
    return _emul_info.vm_base_port;
}

int get_device_serial_number(void)
{
    return _emul_info.device_serial_number;
}

int get_emul_ecs_port(void)
{
    return _emul_info.ecs_port;
}

int get_emul_serial_port(void)
{
    return _emul_info.serial_port;
}

/* current emulator condition */
int get_emulator_condition(void)
{
    return _emul_state.emulator_condition;
}

void set_emulator_condition(int state)
{
    if (state == BOOT_COMPLETED) {
        LOG_INFO("boot completed!\n");
        // TODO:
    } else if (state == RESET) {
        LOG_INFO("reset emulator!\n");

        notify_emul_reset();
    }

    _emul_state.emulator_condition = state;
}

/* emulator window scale */
void set_emul_win_scale(double scale_factor)
{
    if (scale_factor < 0.0 || scale_factor > 2.0) {
        LOG_INFO("scale_factor is out of range : %f\n", scale_factor);
        scale_factor = 1.0;
    }

    _emul_state.scale_factor = scale_factor;
    LOG_INFO("emulator window scale_factor : %f\n", _emul_state.scale_factor);
}

double get_emul_win_scale(void)
{
    return _emul_state.scale_factor;
}

/* emulator rotation */
void set_emul_rotation(short rotation_type)
{
    if (rotation_type < ROTATION_PORTRAIT ||
            rotation_type > ROTATION_REVERSE_LANDSCAPE) {
        LOG_INFO("rotation type is out of range : %d\n", rotation_type);
        rotation_type = ROTATION_PORTRAIT;
    }

    _emul_state.rotation_type = rotation_type;
    LOG_INFO("emulator rotation type : %d\n", _emul_state.rotation_type);
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

#elif defined(CONFIG_WIN32)
    if (key == HOST_CAPSLOCK_KEY) {
        return (GetKeyState(VK_CAPITAL) & 1) != 0;
    } else if (key == HOST_NUMLOCK_KEY) {
        return (GetKeyState(VK_NUMLOCK) & 1) != 0;
    }
#elif defined(CONFIG_DARWIN)
    return get_host_lock_key_state_darwin(key);
#endif

    return -1;
}

/* manage CapsLock key state for host keyboard input */
void set_emul_caps_lock_state(int state)
{
    _emul_state.qemu_caps_lock = state;
}

int get_emul_caps_lock_state(void)
{
    return _emul_state.qemu_caps_lock;
}

/* manage NumLock key state for host keyboard input */
void set_emul_num_lock_state(int state)
{
    _emul_state.qemu_num_lock = state;
}

int get_emul_num_lock_state(void)
{
    return _emul_state.qemu_num_lock;
}

/* emualtor vm name */
void set_emul_vm_name(char *vm_name)
{
    _emul_info.vm_name = vm_name;
}

char* get_emul_vm_name(void)
{
    return _emul_info.vm_name;
}

