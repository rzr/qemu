/*
 * socket server for emulator skin
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

#ifndef MARUSKIN_SERVER_H_
#define MARUSKIN_SERVER_H_

#include <stdbool.h>

int start_skin_server(int argc, char** argv, int qemu_argc, char** qemu_argv);
void shutdown_skin_server(void);

void notify_draw_frame(void);
void notify_sensor_daemon_start(void);
void notify_sdb_daemon_start(void);
void notify_ramdump_completed(void);
void notify_booting_progress(int progress_value);
void notify_brightness(bool on);

int is_ready_skin_server(void);
int get_skin_server_port(void);

#endif /* MARUSKIN_SERVER_H_ */
