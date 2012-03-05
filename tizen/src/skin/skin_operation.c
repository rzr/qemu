/*
 * operation for emulator skin
 *
 * Copyright (C) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <stdio.h>
#include "skin_operation.h"

void start_display( int handle_id ) {
    printf( "start_display handle_id:%d\n", handle_id );
    //TODO sdl init
}

void do_mouse_event( int event_type, int x, int y ) {
    printf( "mouse_event event_type:%d, x:%d, y:%d\n", event_type, x, y );
    //TODO send event to qemu
}

void do_key_event( int event_type, int keycode ) {
    printf( "key_event event_type:%d, keycode:%d\n", event_type, keycode );
    //TODO send event to qemu
}

void change_lcd_state( short direction, short scale ) {
    printf( "change_lcd_state direction:%d, scale:%d\n", direction, scale );
    //TODO send request to emuld
}

void request_close( void ) {
    printf( "request_close\n" );
    //TODO send power key event to qemu
}
