/*
 * operation for emulator skin
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
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

#include <unistd.h>
#include <stdio.h>
#include "maruskin_operation.h"
#include "maru_sdl.h"
#include "debug_ch.h"
#include "console.h"
//FIXME uncomment
//#include "maru_pm.h"

MULTI_DEBUG_CHANNEL(qemu, skin_operation);


enum {
    DIRECTION_PORTRAIT = 1,
    DIRECTION_LANDSCAPE = 2,
    DIRECTION_REVERSE_PORTRAIT = 3,
    DIRECTION_REVERSE_LANDSCAPE = 4,
};

enum {
    SCALE_ONE = 1,
    SCALE_THREE_QUARTERS = 2,
    SCALE_HALF = 3,
    SCALE_ONE_QUARTER = 4,
};

enum {
    MOUSE_DOWN = 1,
    MOUSE_UP = 2,
    MOUSE_DRAG = 3,
};

enum {
    KEY_PRESSED = 1,
    KEY_RELEASED = 2,
};

void start_display( int handle_id, short scale, short direction ) {
    INFO( "start_display handle_id:%d, scale:%d, direction:%d\n", handle_id, scale, direction );
    maruskin_sdl_init(handle_id);
}

void do_mouse_event( int event_type, int x, int y, int z ) {
    INFO( "mouse_event event_type:%d, x:%d, y:%d, z:%d\n", event_type, x, y, z );


    if ( MOUSE_DOWN == event_type || MOUSE_DRAG == event_type) {
        kbd_mouse_event(x, y, z, 1);
    } else if (MOUSE_UP == event_type) {
        kbd_mouse_event(x, y, z, 0);
    } else {
        INFO( "undefined mouse event type:%d\n", event_type );
    }

}

void do_key_event( int event_type, int keycode ) {
    INFO( "key_event event_type:%d, keycode:%d\n", event_type, keycode );
    //TODO
}

void do_hardkey_event( int event_type, int keycode ) {
    INFO( "do_hardkey_event event_type:%d, keycode:%d\n", event_type, keycode );

    //TODO convert keycode ?

//FIXME uncomment
//    // press
//    if ( KEY_PRESSED ) {
//
//        if ( kbd_mouse_is_absolute() ) {
//
//            // home key or power key is used for resume.
//            if ( ( 101 == keycode ) || ( 103 == keycode ) ) {
//                if ( is_suspended_state() ) {
//                    INFO( "user requests system resume.\n" );
//                    resume();
//                    usleep( 500 * 1000 );
//                }
//            }
//
//            ps2kbd_put_keycode( keycode & 0x7f );
//
//        }
//
//    } else if ( KEY_RELEASED ) {
//
//        if ( kbd_mouse_is_absolute() ) {
//            TRACE( "release parsing keycode = %d, result = %d\n", keycode, keycode | 0x80 );
//            ps2kbd_put_keycode( keycode | 0x80 );
//        }
//
//    }

}

void change_lcd_state( short scale, short direction ) {
    INFO( "change_lcd_state scale:%d, scale:%d\n", scale, direction );
    //TODO send request to emuld
}

void open_shell(void) {
    //TODO
}

void request_close( void ) {
    INFO( "request_close\n" );

//FIXME uncomment
//    ps2kbd_put_keycode( 103 & 0x7f );
//#ifdef _WIN32
//    Sleep( 1.6 * 1000 ); // 1.6 seconds
//#else
//    usleep( 1.6 * 1000 * 1000 ); // 1.6 seconds
//#endif
//    ps2kbd_put_keycode( 103 | 0x80 );

}
