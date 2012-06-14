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
#include <pthread.h>
#include "maruskin_operation.h"
#include "hw/maru_brightness.h"
#include "maru_sdl.h"
#include "debug_ch.h"
#include "sdb.h"
#include "nbd.h"
#include "mloop_event.h"
#include "emul_state.h"
#include "maruskin_keymap.h"
#include "emul_state.h"
#include "hw/maru_pm.h"
#include "sysemu.h"

#ifdef _WIN32
#include "target-i386/hax-i386.h"
#endif

MULTI_DEBUG_CHANNEL(qemu, skin_operation);


#define RESUME_KEY_SEND_INTERVAL 500 // milli-seconds
#define CLOSE_POWER_KEY_INTERVAL 1200 // milli-seconds
#define DATA_DELIMITER "#" // in detail info data
#define TIMEOUT_FOR_SHUTDOWN 10 // seconds

static int requested_shutdown_qemu_gracefully = 0;

static void* run_timed_shutdown_thread( void* args );
static void send_to_emuld( const char* request_type, int request_size, const char* send_buf, int buf_size );

void start_display(uint64 handle_id, int lcd_size_width, int lcd_size_height, double scale_factor, short rotation_type)
{
    INFO( "start_display handle_id:%ld, lcd size:%dx%d, scale_factor:%lf, rotation_type:%d\n",
        handle_id, lcd_size_width, lcd_size_height, scale_factor, rotation_type );

    set_emul_win_scale(scale_factor);
    maruskin_sdl_init(handle_id, lcd_size_width, lcd_size_height);
}

void do_mouse_event( int event_type, int x, int y, int z )
{
    if( brightness_off ) {
        TRACE( "reject mouse touch in lcd off : %d, x:%d, y:%d, z:%d\n", event_type, x, y, z );
        return;
    }

    TRACE( "mouse_event event_type:%d, x:%d, y:%d, z:%d\n", event_type, x, y, z );

    if (get_emul_multi_touch_state()->multitouch_enable == 1) {
        maru_finger_processing(x, y, event_type);
    }
    else if ( MOUSE_DOWN == event_type || MOUSE_DRAG == event_type) { //single touch
        kbd_mouse_event(x, y, z, 1);
    } else if (MOUSE_UP == event_type) {
        kbd_mouse_event(x, y, z, 0);
    } else {
        ERR( "undefined mouse event type:%d\n", event_type );
    }

#if 0
#ifdef _WIN32
    Sleep(1);
#else
    usleep(1000);
#endif
#endif
}

void do_key_event( int event_type, int keycode, int key_location )
{
    TRACE( "key_event event_type:%d, keycode:%d, key_location:%d\n", event_type, keycode, key_location );

    //is multi-touch mode ?
    if (get_emul_max_touch_point() > 1) {
        if (keycode == JAVA_KEYCODE_BIT_CTRL) {
            if (KEY_PRESSED == event_type) {
                get_emul_multi_touch_state()->multitouch_enable = 1;
                INFO("multi-touch enabled\n");
            } else if (KEY_RELEASED == event_type) {
                get_emul_multi_touch_state()->multitouch_enable = 0;
                clear_finger_slot();
                INFO("multi-touch disabled\n");
            }
        } else if (keycode == (JAVA_KEYCODE_NO_FOCUS | JAVA_KEYCODE_BIT_CTRL)) {
            //release ctrl key when dragging
            if (KEY_RELEASED == event_type) {
                get_emul_multi_touch_state()->multitouch_enable = 0;
                clear_finger_slot();
                INFO("multi-touch disabled\n");
            }
        }
    }

    if (!mloop_evcmd_get_usbkbd_status()) {
    	return;
    }

    int scancode = javakeycode_to_scancode(keycode, event_type, key_location);
    TRACE("javakeycode_to_scancode : %d\n", scancode);

    if (scancode == -1) {
        INFO("cannot find scancode\n");
        return;
    }

    if (KEY_PRESSED == event_type) {
        kbd_put_keycode(scancode);
    } else if (KEY_RELEASED == event_type) {
        kbd_put_keycode(scancode | 0x80);
    }
}

void do_hardkey_event( int event_type, int keycode )
{
    TRACE( "do_hardkey_event event_type:%d, keycode:%d\n", event_type, keycode );

    if ( is_suspended_state() ) {
        if ( KEY_PRESSED == event_type ) {
            if ( kbd_mouse_is_absolute() ) {
                // home key or power key is used for resume.
                if ( ( HARD_KEY_HOME == keycode ) || ( HARD_KEY_POWER == keycode ) ) {
                    INFO( "user requests system resume.\n" );
                    resume();
#ifdef _WIN32
                    Sleep( RESUME_KEY_SEND_INTERVAL );
#else
                    usleep( RESUME_KEY_SEND_INTERVAL * 1000 );
#endif
                }
            }
        }
    }

    SDL_Event event;
    memset( &event, 0, sizeof(SDL_Event) );

    event.type = SDL_USEREVENT;
    event.user.code = SDL_USER_EVENT_CODE_HARDKEY;

    // use pointer as integer
    event.user.data1 = (void*) event_type;
    event.user.data2 = (void*) keycode;

    // see qemu_ds_refresh in maru_sdl.c
    SDL_PushEvent( &event );

}

void do_scale_event( double scale_factor )
{
    INFO( "do_scale_event scale_factor:%lf", scale_factor);

    set_emul_win_scale(scale_factor);

    //TODO: thread safe
    //qemu refresh
    //vga_hw_invalidate();
    //vga_hw_update();
}

void do_rotation_event( int rotation_type)
{

    INFO( "do_rotation_event rotation_type:%d", rotation_type);

    char send_buf[32] = { 0 };

    switch ( rotation_type ) {
        case ROTATION_PORTRAIT:
            sprintf( send_buf, "1\n3\n0\n-9.80665\n0\n" );
            break;
        case ROTATION_LANDSCAPE:
            sprintf( send_buf, "1\n3\n-9.80665\n0\n0\n" );
            break;
        case ROTATION_REVERSE_PORTRAIT:
            sprintf( send_buf, "1\n3\n0\n9.80665\n0\n" );
            break;
        case ROTATION_REVERSE_LANDSCAPE:
            sprintf(send_buf, "1\n3\n9.80665\n0\n0\n");
            break;

        default:
            break;
    }

    send_to_emuld( "sensor\n\n\n\n", 10, send_buf, 32 );

    set_emul_rotation( rotation_type );

}

QemuSurfaceInfo* get_screenshot_info( void ) {

    DisplaySurface* qemu_display_surface = get_qemu_display_surface();

    if ( !qemu_display_surface ) {
        ERR( "qemu surface is NULL.\n" );
        return NULL;
    }

    QemuSurfaceInfo* info = (QemuSurfaceInfo*) g_malloc0( sizeof(QemuSurfaceInfo) );
    if ( !info ) {
        ERR( "Fail to malloc for QemuSurfaceInfo.\n");
        return NULL;
    }

    int length = qemu_display_surface->linesize * qemu_display_surface->height;
    INFO( "screenshot data length:%d\n", length );

    if ( 0 >= length ) {
        g_free( info );
        ERR( "screenshot data ( 0 >=length ). length:%d\n", length );
        return NULL;
    }

    info->pixel_data = (unsigned char*) g_malloc0( length );
    if ( !info->pixel_data ) {
        g_free( info );
        ERR( "Fail to malloc for pixel data.\n");
        return NULL;
    }

    memcpy( info->pixel_data, qemu_display_surface->data, length );
    info->pixel_data_length = length;

    return info;

}

void free_screenshot_info( QemuSurfaceInfo* info ) {
    if( info ) {
        if( info->pixel_data ) {
            g_free( info->pixel_data );
        }
        g_free( info );
    }
}

DetailInfo* get_detail_info( int qemu_argc, char** qemu_argv ) {

    DetailInfo* detail_info = g_malloc0( sizeof(DetailInfo) );
    if ( !detail_info ) {
        ERR( "Fail to malloc for DetailInfo.\n" );
        return NULL;
    }

    int i = 0;
    int total_len = 0;
    int delimiter_len = strlen( DATA_DELIMITER );

    for ( i = 0; i < qemu_argc; i++ ) {
        total_len += strlen( qemu_argv[i] );
        total_len += delimiter_len;
    }

#ifdef _WIN32
    const int HAX_LEN = 32;
    char hax_error[HAX_LEN];
    memset( hax_error, 0, HAX_LEN );

    int hax_err_len = 0;
    hax_err_len = sprintf( hax_error + hax_err_len, "%s", "hax_error=" );

    int error = 0;
    if ( !ret_hax_init ) {
        if ( -ENOSPC == ret_hax_init ) {
            error = 1;
        }
    }
    hax_err_len += sprintf( hax_error + hax_err_len, "%s", error ? "true" : "false" );
    total_len += hax_err_len;
#endif

    char* info_data = g_malloc0( total_len + 1 );
    if ( !info_data ) {
        g_free( detail_info );
        ERR( "Fail to malloc for info data.\n" );
        return NULL;
    }

    int len = 0;
    total_len = 0;

    for ( i = 0; i < qemu_argc; i++ ) {
        len = strlen( qemu_argv[i] );
        sprintf( info_data + total_len, "%s%s", qemu_argv[i], DATA_DELIMITER );
        total_len += len + delimiter_len;
    }

#ifdef _WIN32
    snprintf( info_data + total_len, total_len + 1, "%s", hax_error );
    total_len += hax_err_len;
#endif

    INFO( "################## detail info data ####################\n" );
    INFO( "%s\n", info_data );

    detail_info->data = info_data;
    detail_info->data_length = total_len;

    return detail_info;

}

void free_detail_info( DetailInfo* detail_info ) {
    if ( detail_info ) {
        if ( detail_info->data ) {
            g_free( detail_info->data );
        }
        g_free( detail_info );
    }
}

void open_shell( void ) {
}

void onoff_usb_kbd( int on )
{
    INFO( "usb kbd on/off:%d\n", on );
    //TODO
    mloop_evcmd_usbkbd(on);
}


void request_close( void )
{
    INFO( "request_close\n" );

    do_hardkey_event( KEY_PRESSED, HARD_KEY_POWER );

#ifdef _WIN32
        Sleep( CLOSE_POWER_KEY_INTERVAL );
#else
        usleep( CLOSE_POWER_KEY_INTERVAL * 1000 );
#endif

    do_hardkey_event( KEY_RELEASED, HARD_KEY_POWER );

}

void shutdown_qemu_gracefully( void ) {

    requested_shutdown_qemu_gracefully = 1;

    pthread_t thread_id;
    if( 0 > pthread_create( &thread_id, NULL, run_timed_shutdown_thread, NULL ) ) {
        ERR( "!!! Fail to create run_timed_shutdown_thread. shutdown qemu right now !!!\n"  );
        qemu_system_shutdown_request();
    }

}

int is_requested_shutdown_qemu_gracefully( void ) {
    return requested_shutdown_qemu_gracefully;
}

static void* run_timed_shutdown_thread( void* args ) {

    send_to_emuld( "system\n\n\n\n", 10, "shutdown", 8 );

    int sleep_interval_time = 1000; // milli-seconds

    int i;
    for ( i = 0; i < TIMEOUT_FOR_SHUTDOWN; i++ ) {
#ifdef _WIN32
        Sleep( sleep_interval_time );
#else
        usleep( sleep_interval_time * 1000 );
#endif
        // do not use logger to help user see log in console
        fprintf( stdout, "Wait for shutdown qemu...%d\n", ( i + 1 ) );
    }

    INFO( "Shutdown qemu !!!\n" );
    qemu_system_shutdown_request();

    return NULL;

}

static void send_to_emuld( const char* request_type, int request_size, const char* send_buf, int buf_size ) {

    int s = tcp_socket_outgoing( "127.0.0.1", (uint16_t) ( tizen_base_port + SDB_TCP_EMULD_INDEX ) );

    if ( s < 0 ) {
        ERR( "can't create socket to talk to the sdb forwarding session \n" );
        ERR( "[127.0.0.1:%d/tcp] connect fail (%d:%s)\n" , tizen_base_port + SDB_TCP_EMULD_INDEX , errno, strerror(errno) );
        return;
    }

    socket_send( s, (char*)request_type, request_size );
    socket_send( s, &buf_size, 4 );
    socket_send( s, (char*)send_buf, buf_size );

    INFO( "send to emuld [req_type:%s, send_data:%s, send_size:%d] 127.0.0.1:%d/tcp \n",
        request_type, send_buf, buf_size, tizen_base_port + SDB_TCP_EMULD_INDEX );

#ifdef _WIN32
    closesocket( s );
#else
    close( s );
#endif

}
