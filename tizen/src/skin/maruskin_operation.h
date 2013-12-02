/*
 * operation for emulator skin
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * Hyunjun Son
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

#ifndef MARUSKIN_OPERATION_H_
#define MARUSKIN_OPERATION_H_

#include "qemu-common.h"


extern int ret_hax_init;

struct QemuSurfaceInfo {
    unsigned char* pixel_data;
    int pixel_data_length;
};
typedef struct QemuSurfaceInfo QemuSurfaceInfo;

struct DetailInfo {
    char* data;
    int data_length;
};
typedef struct DetailInfo DetailInfo;

void start_display(uint64 handle_id,
    unsigned int display_width, unsigned int display_height,
    double scale_factor, short rotation_type, bool blank_guide);

void do_mouse_event(int button_type, int event_type,
    int origin_x, int origin_y, int x, int y, int z);
void do_keyboard_key_event(int event_type,
    int keycode, int state_mask, int key_location);
void do_hw_key_event(int event_type, int keycode);
void do_scale_event(double scale_factor);
void do_rotation_event(int rotation_type);

QemuSurfaceInfo *get_screenshot_info(void);
DetailInfo *get_detail_info(int qemu_argc, char **qemu_argv);
void free_detail_info(DetailInfo *detail_info);
void free_screenshot_info(QemuSurfaceInfo *);

void do_open_shell(void);
void do_host_kbd_enable(bool on);
void do_interpolation_enable(bool on);
void do_ram_dump(void);
void do_guestmemory_dump(void);

void request_close(void);
void shutdown_qemu_gracefully(void);
int is_requested_shutdown_qemu_gracefully(void);

#endif /* MARUSKIN_OPERATION_H_ */
