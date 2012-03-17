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
#include "../hw/maru_pm.h"

#include "console.h"
#include "sdb.h"
#include "nbd.h"
#include "../mloop_event.h"
#include "emul_state.h"
#include "sdl_rotate.h"

#ifndef _WIN32
#include "maruskin_keymap.h"
#endif

MULTI_DEBUG_CHANNEL(qemu, skin_operation);

enum {
    HARD_KEY_HOME = 101,
    HARD_KEY_POWER = 103,
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

void start_display( int handle_id, int lcd_size_width, int lcd_size_height, int scale, short rotation ) {
    INFO( "start_display handle_id:%d, lcd size:%dx%d, scale:%d, rotation:%d\n",
        handle_id, lcd_size_width, lcd_size_height, scale, rotation );

    maruskin_sdl_init(handle_id, lcd_size_width, lcd_size_height);
}

void do_mouse_event( int event_type, int x, int y, int z ) {
    TRACE( "mouse_event event_type:%d, x:%d, y:%d, z:%d\n", event_type, x, y, z );

    if ( MOUSE_DOWN == event_type || MOUSE_DRAG == event_type) {
        kbd_mouse_event(x, y, z, 1);
    } else if (MOUSE_UP == event_type) {
        kbd_mouse_event(x, y, z, 0);
    } else {
        ERR( "undefined mouse event type:%d\n", event_type );
    }

    usleep(100);
}

void do_key_event( int event_type, int keycode ) {
    TRACE( "key_event event_type:%d, keycode:%d\n", event_type, keycode );

#ifndef _WIN32
    if (KEY_PRESSED == event_type) {
        kbd_put_keycode(curses2keycode[keycode]);
    } else if (KEY_RELEASED == event_type) {
        kbd_put_keycode(curses2keycode[keycode] | 0x80);
    }
#endif
}

void do_hardkey_event( int event_type, int keycode ) {
    TRACE( "do_hardkey_event event_type:%d, keycode:%d\n", event_type, keycode );

    // press
    if ( KEY_PRESSED == event_type ) {

        if ( kbd_mouse_is_absolute() ) {

            // home key or power key is used for resume.
            if ( ( HARD_KEY_HOME == keycode ) || ( HARD_KEY_POWER == keycode ) ) {
                if ( is_suspended_state() ) {
                    INFO( "user requests system resume.\n" );
                    resume();
                    usleep( 500 * 1000 );
                }
            }

            ps2kbd_put_keycode( keycode & 0x7f );

        }

    } else if ( KEY_RELEASED == event_type ) {

        if ( kbd_mouse_is_absolute() ) {
            ps2kbd_put_keycode( keycode | 0x80 );
        }

    }

}

void do_scale_event( int event_type) {
    INFO( "do_scale_event event_type:%d", event_type);

    //qemu refresh
    vga_hw_invalidate();
    vga_hw_update();

    set_emul_win_scale(event_type);
}

void do_rotation_event( int event_type) {

    INFO( "do_rotation_event event_type:%d", event_type);

    int buf_size = 32;
    char send_buf[32] = { 0 };

    switch ( event_type ) {
    case ROTATION_PORTRAIT:
        sprintf( send_buf, "1\n3\n0\n-9.80665\n0\n" );
        break;
    case ROTATION_LANDSCAPE:
        sprintf( send_buf, "1\n3\n0\n9.80665\n0\n" );
        break;
    case ROTATION_REVERSE_PORTRAIT:
        sprintf( send_buf, "1\n3\n-9.80665\n0\n0\n" );
        break;
    case ROTATION_REVERSE_LANDSCAPE:
        sprintf(send_buf, "1\n3\n9.80665\n0\n0\n");
        break;
    }

    // send_to_sensor_daemon
    int s;

    s = tcp_socket_outgoing( "127.0.0.1", (uint16_t) ( tizen_base_port + SDB_TCP_EMULD_INDEX ) );
    if ( s < 0 ) {
        ERR( "can't create socket to talk to the sdb forwarding session \n");
        ERR( "[127.0.0.1:%d/tcp] connect fail (%d:%s)\n" , tizen_base_port + SDB_TCP_EMULD_INDEX , errno, strerror(errno));
        return;
    }

    socket_send( s, "sensor\n\n\n\n", 10 );
    socket_send( s, &buf_size, 4 );
    socket_send( s, send_buf, buf_size );

    INFO( "send to sendord(size: %d) 127.0.0.1:%d/tcp \n", buf_size, tizen_base_port + SDB_TCP_EMULD_INDEX);

    set_emul_rotation(event_type);

#ifdef _WIN32
    closesocket( s );
#else
    close( s );
#endif

}

void open_shell(void) {
    //TODO
}

void onoff_usb_kbd( int on ) {
    INFO( "usb kbd on/off:%d\n", on );
    //TODO
    if(on) {
        mloop_evcmd_usbkbd_on();
    }
    else {
        mloop_evcmd_usbkbd_off();
    }
}


void request_close( void ) {
    INFO( "request_close\n" );

    ps2kbd_put_keycode( 103 & 0x7f );
#ifdef _WIN32
    Sleep( 1.6 * 1000 ); // 1.6 seconds
#else
    usleep( 1.6 * 1000 * 1000 ); // 1.6 seconds
#endif
    kbd_put_keycode( 103 | 0x80 );

}
