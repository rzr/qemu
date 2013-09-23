/*
 * Emulator
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


#ifndef __EMUL_STATE_H__
#define __EMUL_STATE_H__


#include "maru_common.h"
#include "maru_finger.h"

/* keep it consistent with emulator-skin definition */
enum {
    HARD_KEY_HOME = 139,
    HARD_KEY_POWER = 116,
    HARD_KEY_VOL_UP = 115,
    HARD_KEY_VOL_DOWN = 114,
};

/* keep it consistent with emulator-skin definition */
enum {
    MOUSE_DOWN = 1,
    MOUSE_UP = 2,
    MOUSE_DRAG = 3,
    MOUSE_WHEELUP = 4,
    MOUSE_WHEELDOWN = 5,
    MOUSE_MOVE = 6,
};

/* keep it consistent with emulator-skin definition */
enum {
    KEY_PRESSED = 1,
    KEY_RELEASED = 2,
};

/* keep it consistent with emulator-skin definition */
enum {
    ROTATION_PORTRAIT = 0,
    ROTATION_LANDSCAPE = 1,
    ROTATION_REVERSE_PORTRAIT = 2,
    ROTATION_REVERSE_LANDSCAPE = 3,
};

enum {
    HOST_CAPSLOCK_KEY = 1,
    HOST_NUMLOCK_KEY = 2,
};


typedef  struct EmulatorConfigInfo {
    int skin_enable;
    int lcd_size_w;
    int lcd_size_h;
    int sdl_bpp;
    bool input_mouse_enable;
    bool input_touch_enable;
    int max_touch_point;
    int vm_base_port;
    int device_serial_number;
    char *vm_name;
    /* add here */
} EmulatorConfigInfo;

typedef struct EmulatorConfigState {
    int emulator_condition; //TODO : enum
    double scale_factor;
    short rotation_type;
    MultiTouchState qemu_mts;
    int qemu_caps_lock;
    int qemu_num_lock;
    /* add here */
} EmulatorConfigState;


/* setter */
void set_emul_skin_enable(int enable);
void set_emul_lcd_size(int width, int height);
void set_emul_win_scale(double scale);
void set_emul_sdl_bpp(int bpp);
void set_emul_input_mouse_enable(bool on);
void set_emul_input_touch_enable(bool on);
void set_emul_max_touch_point(int cnt);
void set_emul_vm_base_port(int port);

void set_emulator_condition(int state);
void set_emul_rotation(short rotation_type);
void set_emul_caps_lock_state(int state);
void set_emul_num_lock_state(int state);
void set_emul_vm_name(char *vm_name);

/* getter */
int get_emul_skin_enable(void);
int get_emul_lcd_width(void);
int get_emul_lcd_height(void);
double get_emul_win_scale(void);
int get_emul_sdl_bpp(void);
bool is_emul_input_mouse_enable(void);
bool is_emul_input_touch_enable(void);
int get_emul_max_touch_point(void);
int get_emul_vm_base_port(void);
int get_device_serial_number(void);

int get_emulator_condition(void);
short get_emul_rotation(void);
MultiTouchState *get_emul_multi_touch_state(void);
int get_host_lock_key_state(int key);
int get_host_lock_key_state_darwin(int key);
int get_emul_caps_lock_state(void);
int get_emul_num_lock_state(void);
char* get_emul_vm_name(void);

#endif /* __EMUL_STATE_H__ */
