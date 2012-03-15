/*
 * operation for emulator skin
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

#ifndef MARUSKIN_OPERATION_H_
#define MARUSKIN_OPERATION_H_

void start_display( int handle_id, int lcd_size_width, int lcd_size_height, short scale, short direction );

void do_mouse_event( int event_type, int x, int y, int z );

void do_key_event( int event_type, int keycode );

void do_hardkey_event( int event_type, int keycode );

void do_rotation_event( int event_type );

void open_shell(void);

void onoff_usb_kbd( int on );

void request_close( void );

#endif /* MARUSKIN_OPERATION_H_ */
