/*
 * socket server for emulator skin
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "maruskin_server.h"
#include "maruskin_operation.h"
#include "qemu-thread.h"
#include "emul_state.h"
#include "maru_sdl.h"
#include "maruskin_client.h"
#include "emulator.h"
#include "debug_ch.h"
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

MULTI_DEBUG_CHANNEL( qemu, skin_server );

#define MAX_REQ_ID 0x7fffffff
#define RECV_BUF_SIZE 32
#define RECV_HEADER_SIZE 12

#define SEND_HEADER_SIZE 10
#define SEND_BIG_BUF_SIZE 1024 * 1024 * 2
#define SEND_BUF_SIZE 512

#define HEART_BEAT_INTERVAL 1
#define HEART_BEAT_FAIL_COUNT 5
#define HEART_BEAT_EXPIRE_COUNT 5
#define RESTART_CLIENT_MAX_COUNT 3

#define PORT_RETRY_COUNT 50

#define TEST_HB_IGNORE "test.hb.ignore"
#define SKIN_CONFIG_PROP ".skinconfig.properties"

enum {
    RECV_START = 1,
    RECV_MOUSE_EVENT = 10,
    RECV_KEY_EVENT = 11,
    RECV_HARD_KEY_EVENT = 12,
    RECV_CHANGE_LCD_STATE = 13,
    RECV_OPEN_SHELL = 14,
    RECV_USB_KBD = 15,
    RECV_SCREEN_SHOT = 16,
    RECV_DETAIL_INFO = 17,
    RECV_RESPONSE_HEART_BEAT = 900,
    RECV_CLOSE = 998,
    RECV_RESPONSE_SHUTDOWN = 999,
};

enum {
    SEND_HEART_BEAT = 1,
    SEND_SCREEN_SHOT = 2,
    SEND_DETAIL_INFO = 3,
    SEND_SENSOR_DAEMON_START = 800,
    SEND_SHUTDOWN = 999,
};

static int seq_req_id = 0;

static uint16_t svr_port = 0;
static int server_sock = 0;
static int client_sock = 0;
static int stop_server = 0;
static int is_sensord_initialized = 0;
static int ready_server = 0;
static int ignore_heartbeat = 0;
static int is_force_close_client = 0;

static int is_started_heartbeat = 0;
static int stop_heartbeat = 0;
static int recv_heartbeat_count = 0;
static pthread_t thread_id_heartbeat;
static pthread_mutex_t mutex_heartbeat = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_heartbeat = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_recv_heartbeat_count = PTHREAD_MUTEX_INITIALIZER;

static int skin_argc = 0;
static char** skin_argv = NULL;
static int qmu_argc = 0;
static char** qmu_argv = NULL;

static void parse_skin_args( void );
static void parse_skinconfig_prop( void );
static void* run_skin_server( void* args );
static int recv_n( int client_sock, char* read_buf, int recv_len );
static void make_header( int client_sock, short send_cmd, int data_length, char* sendbuf, int print_log );
static int send_skin_header_only( int client_sock, short send_cmd, int print_log );
static int send_skin_data( int client_sock, short send_cmd, unsigned char* data, int length, int big_data );
static int send_n( int client_sock, unsigned char* data, int length, int big_data );

static void* do_heart_beat( void* args );
static int start_heart_beat( void );
static void stop_heart_beat( void );

int start_skin_server( int argc, char** argv, int qemu_argc, char** qemu_argv ) {

    skin_argc = argc;
    skin_argv = argv;

    parse_skinconfig_prop();

    // arguments have higher priority than '.skinconfig.properties'
    parse_skin_args();

    INFO( "ignore_heartbeat:%d\n", ignore_heartbeat );

    qmu_argc = qemu_argc;
    qmu_argv = qemu_argv;

    QemuThread qemu_thread;

    qemu_thread_create( &qemu_thread, run_skin_server, NULL );

    return 1;

}

void shutdown_skin_server( void ) {

    INFO( "shutdown_skin_server\n" );

    if ( client_sock ) {

        INFO( "send shutdown to skin.\n" );

        if ( 0 > send_skin_header_only( client_sock, SEND_SHUTDOWN, 1 ) ) {

            ERR( "fail to send SEND_SHUTDOWN to skin.\n" );
            // force close
            is_force_close_client = 1;
#ifdef _WIN32
            closesocket( client_sock );
#else
            close( client_sock );
#endif

        } else {
            // skin sent RECV_RESPONSE_SHUTDOWN.
        }
    }

    if ( client_sock ) {
#ifdef _WIN32
        closesocket( client_sock );
#else
        close( client_sock );
#endif
    }

    int close_server_socket = 0;
    int count = 0;
    int max_sleep_count = 10;

    while( 1 ) {

        if( max_sleep_count < count ) {
            close_server_socket = 1;
            break;
        }

        if( stop_server ) {
            INFO( "skin client sent normal shutdown response.\n" );
            break;
        }else {
#ifdef _WIN32
            Sleep( 1 ); // 1ms
#else
            usleep( 1000 ); // 1ms
#endif
            count++;
        }
    }

    if ( close_server_socket ) {
        WARN( "skin client did not send normal shutdown response.\n" );
        stop_server = 1;
        if ( server_sock ) {
#ifdef _WIN32
            closesocket( server_sock );
#else
            close( server_sock );
#endif
        }
    }

}

void notify_sensor_daemon_start( void ) {
    INFO( "notify_sensor_daemon_start\n" );
    is_sensord_initialized = 1;
    if ( client_sock ) {
        if ( 0 > send_skin_header_only( client_sock, SEND_SENSOR_DAEMON_START, 1 ) ) {
            ERR( "fail to send SEND_SENSOR_DAEMON_START to skin.\n" );
        }
    }
}

int is_ready_skin_server( void ) {
    return ready_server;
}

int get_skin_server_port( void ) {
    return svr_port;
}

static void parse_skinconfig_prop( void ) {

    int target_path_len = strlen( tizen_target_path );
    char skin_config_path[target_path_len + 32];

    memset( skin_config_path, 0, target_path_len + 32 );
    strcpy( skin_config_path, tizen_target_path );
#ifdef _WIN32
    strcat( skin_config_path, "\\" );
#else
    strcat( skin_config_path, "/" );
#endif
    strcat( skin_config_path, SKIN_CONFIG_PROP );

    FILE* fp = fopen( skin_config_path, "r" );

    if ( !fp ) {
        INFO( "There is no %s. skin_config_path:%s\n", SKIN_CONFIG_PROP, skin_config_path );
        return;
    }

    fseek( fp, 0L, SEEK_END );
    int buf_size = ftell( fp );
    rewind( fp );

    if ( 0 >= buf_size ) {
        WARN( "%s contents is empty.\n", SKIN_CONFIG_PROP );
        fclose( fp );
        return;
    }

    char* buf = g_malloc0( buf_size );
    if ( !buf ) {
        ERR( "Fail to malloc for %s\n", SKIN_CONFIG_PROP );
        fclose( fp );
        return;
    }

    int read_cnt = 0;
    int total_cnt = 0;

    while ( 1 ) {

        if ( total_cnt == buf_size ) {
            break;
        }

        read_cnt = fread( (void*) ( buf + read_cnt ), 1, buf_size - total_cnt, fp );
        if ( 0 > read_cnt ) {
            break;
        } else {
            total_cnt += read_cnt;
        }

    }

    fclose( fp );

    INFO( "====== %s ======\n%s\n====================================\n", SKIN_CONFIG_PROP, buf );

    char hb_ignore_prop[32];
    memset( hb_ignore_prop, 0, 32 );
    strcat( hb_ignore_prop, TEST_HB_IGNORE );
    strcat( hb_ignore_prop, "=true" );

    char* line_str = strtok( buf, "\n" );

    while ( 1 ) {

        if ( line_str ) {

            TRACE( "prop line_str:%s\n", line_str );

            if ( 0 == strcmp( line_str, hb_ignore_prop ) ) {
                ignore_heartbeat = 1;
                INFO( "ignore heartbeat by %s\n", SKIN_CONFIG_PROP );
            }

        } else {
            break;
        }

        line_str = strtok( NULL, "\n" );

    }

    g_free( buf );

}

static void parse_skin_args( void ) {

    int i;
    for( i = 0; i < skin_argc; i++ ) {

        char* arg = NULL;
        arg = strdup( skin_argv[i] );

        if( arg ) {

            char* key = strtok( arg, "=" );
            char* value = strtok( NULL, "=" );

            INFO( "skin params key:%s, value:%s\n", key, value );

            if( 0 == strcmp( TEST_HB_IGNORE, key ) ) {
                if( 0 == strcmp( "true", value ) ) {
                    ignore_heartbeat = 1;
                }else if( 0 == strcmp( "false", value ) ) {
                    ignore_heartbeat = 0;
                }
            }

            free( arg );

        }else {
            ERR( "fail to strdup." );
        }

    }

}

static void* run_skin_server( void* args ) {

    struct sockaddr server_addr, client_addr;
    socklen_t server_len, client_len;
    int shutdown_qmu = 0;

    INFO("run skin server\n");

    if ( 0 > ( server_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) ) {
        ERR( "create listen socket error\n" );
        perror( "create listen socket error : " );
        shutdown_qmu = 1;
        goto cleanup;
    }

    memset( &server_addr, '\0', sizeof( server_addr ) );
    ( (struct sockaddr_in *) &server_addr )->sin_family = AF_INET;
    memcpy( &( (struct sockaddr_in *) &server_addr )->sin_addr, "\177\000\000\001", 4 ); // 127.0.0.1
    ( (struct sockaddr_in *) &server_addr )->sin_port = htons( 0 );

    server_len = sizeof( server_addr );

    if ( 0 != bind( server_sock, &server_addr, server_len ) ) {
        ERR( "skin server bind error\n" );
        perror( "skin server bind error : " );
        shutdown_qmu = 1;
        goto cleanup;
    }

    memset( &server_addr, '\0', sizeof( server_addr ) );
    getsockname( server_sock, (struct sockaddr *) &server_addr, &server_len );
    svr_port = ntohs( ( (struct sockaddr_in *) &server_addr )->sin_port );

    INFO( "success to bind port[127.0.0.1:%d/tcp] for skin_server in host \n", svr_port );

    if ( 0 > listen( server_sock, 4 ) ) {
        ERR( "skin_server listen error\n" );
        perror( "skin_server listen error : " );
        shutdown_qmu = 1;
        goto cleanup;
    }

    client_len = sizeof( client_addr );

    char recvbuf[RECV_BUF_SIZE];

    INFO( "skin server start...port:%d\n", svr_port );

    while ( 1 ) {

        if ( stop_server ) {
            INFO( "close server socket normally.\n" );
            break;
        }

        ready_server = 1;

        if( !is_started_heartbeat ) {
            if ( !start_heart_beat() ) {
                ERR( "Fail to start heartbeat thread.\n" );
                shutdown_qmu = 1;
                break;
            }
        }

        INFO( "start accepting socket...\n" );

        if ( 0 > ( client_sock = accept( server_sock, (struct sockaddr *) &client_addr, &client_len ) ) ) {
            ERR( "skin_servier accept error\n" );
            perror( "skin_servier accept error : " );
            continue;
        }

        INFO( "accept client : client_sock:%d\n", client_sock );

        while ( 1 ) {

            if ( stop_server ) {
                INFO( "stop receiving current client socket.\n" );
                break;
            }

            stop_heartbeat = 0;
            memset( &recvbuf, 0, RECV_BUF_SIZE );

            int read_cnt = recv_n( client_sock, recvbuf, RECV_HEADER_SIZE );

            if ( 0 > read_cnt ) {

                if( is_force_close_client ) {
                    WARN( "force close client socket.\n" );
                    is_force_close_client = 0;
                }else {
                    ERR( "skin_server read error:%d\n", read_cnt );
                    perror( "skin_server read error : " );
                }
                break;

            } else {

                if ( 0 == read_cnt ) {
                    ERR( "read_cnt is 0.\n" );
                    break;
                }

                int log_cnt;
                char log_buf[512];
                memset( log_buf, 0, 512 );

                log_cnt = sprintf( log_buf, "== RECV read_cnt:%d ", read_cnt );

                int uid = 0;
                int req_id = 0;
                short cmd = 0;
                short length = 0;

                char* p = recvbuf;

                memcpy( &uid, p, sizeof( uid ) );
                p += sizeof( uid );
                memcpy( &req_id, p, sizeof( req_id ) );
                p += sizeof( req_id );
                memcpy( &cmd, p, sizeof( cmd ) );
                p += sizeof( cmd );
                memcpy( &length, p, sizeof( length ) );

                uid = ntohl( uid );
                req_id = ntohl( req_id );
                cmd = ntohs( cmd );
                length = ntohs( length );

                log_cnt += sprintf( log_buf + log_cnt, "uid:%d, req_id:%d, cmd:%d, length:%d ", uid, req_id, cmd, length );

                if ( 0 < length ) {

                    if ( RECV_BUF_SIZE < length ) {
                        ERR( "length is bigger than RECV_BUF_SIZE\n" );
                        continue;
                    }

                    memset( &recvbuf, 0, length );

                    int recv_cnt = recv_n( client_sock, recvbuf, length );

                    log_cnt += sprintf( log_buf + log_cnt, "data read_cnt:%d ", recv_cnt );

                    if ( 0 > recv_cnt ) {
                        ERR( "skin_server read data\n" );
                        perror( "skin_server read data : " );
                        break;
                    } else if ( 0 == recv_cnt ) {
                        ERR( "data read_cnt is 0.\n" );
                        break;
                    } else if ( recv_cnt != length ) {
                        ERR( "read_cnt is not equal to length.\n" );
                        break;
                    }

                }

                switch ( cmd ) {
                case RECV_START: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_START ==\n" );
                    INFO( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

                    /* keep it consistent with emulator-skin definition */
                    int handle_id = 0;
                    int lcd_size_width = 0;
                    int lcd_size_height = 0;
                    int scale = 0;
                    double scale_ratio = 0.0;
                    short rotation = 0;

                    char* p = recvbuf;
                    memcpy( &handle_id, p, sizeof( handle_id ) );
                    p += sizeof( handle_id );
                    memcpy( &lcd_size_width, p, sizeof( lcd_size_width ) );
                    p += sizeof( lcd_size_width );
                    memcpy( &lcd_size_height, p, sizeof( lcd_size_height ) );
                    p += sizeof( lcd_size_height );
                    memcpy( &scale, p, sizeof( scale ) );
                    p += sizeof( scale );
                    memcpy( &rotation, p, sizeof( rotation ) );

                    handle_id = ntohl( handle_id );
                    lcd_size_width = ntohl( lcd_size_width );
                    lcd_size_height = ntohl( lcd_size_height );
                    scale = ntohl( scale );
                    scale_ratio = ( (double) scale ) / 100;
                    rotation = ntohs( rotation );

                    set_emul_win_scale( scale_ratio );

                    start_display( handle_id, lcd_size_width, lcd_size_height, scale_ratio, rotation );

                    break;
                }
                case RECV_MOUSE_EVENT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_MOUSE_EVENT ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

                    /* keep it consistent with emulator-skin definition */
                    int event_type = 0;
                    int x = 0;
                    int y = 0;
                    int z = 0;

                    char* p = recvbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &x, p, sizeof( x ) );
                    p += sizeof( x );
                    memcpy( &y, p, sizeof( y ) );
                    p += sizeof( y );
                    memcpy( &z, p, sizeof( z ) );

                    event_type = ntohl( event_type );
                    x = ntohl( x );
                    y = ntohl( y );
                    z = ntohl( z );

                    do_mouse_event( event_type, x, y, z );
                    break;
                }
                case RECV_KEY_EVENT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_KEY_EVENT ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

                    /* keep it consistent with emulator-skin definition */
                    int event_type = 0;
                    int keycode = 0;
                    int key_location = 0;

                    char* p = recvbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &keycode, p, sizeof( keycode ) );
                    p += sizeof( keycode );
                    memcpy( &key_location, p, sizeof( key_location ) );

                    event_type = ntohl( event_type );
                    keycode = ntohl( keycode );
                    key_location = ntohl( key_location );

                    do_key_event( event_type, keycode, key_location );
                    break;
                }
                case RECV_HARD_KEY_EVENT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_HARD_KEY_EVENT ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

                    /* keep it consistent with emulator-skin definition */
                    int event_type = 0;
                    int keycode = 0;

                    char* p = recvbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &keycode, p, sizeof( keycode ) );

                    event_type = ntohl( event_type );
                    keycode = ntohl( keycode );

                    do_hardkey_event( event_type, keycode );
                    break;
                }
                case RECV_CHANGE_LCD_STATE: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_CHANGE_LCD_STATE ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

                    /* keep it consistent with emulator-skin definition */
                    int scale = 0;
                    double scale_ratio = 0.0;
                    short rotation_type = 0;

                    char* p = recvbuf;
                    memcpy( &scale, p, sizeof( scale ) );
                    p += sizeof( scale );
                    memcpy( &rotation_type, p, sizeof( rotation_type ) );

                    scale = ntohl( scale );
                    scale_ratio = ( (double) scale ) / 100;
                    rotation_type = ntohs( rotation_type );

                    if ( get_emul_win_scale() != scale_ratio ) {
                        do_scale_event( scale_ratio );
                    }

                    if ( is_sensord_initialized == 1 && get_emul_rotation() != rotation_type ) {
                        do_rotation_event( rotation_type );
                    }

                    maruskin_sdl_resize(); //send sdl event
                    break;
                }
                case RECV_SCREEN_SHOT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_SCREEN_SHOT ==\n" );
                    TRACE( log_buf );

                    QemuSurfaceInfo* info = get_screenshot_info();

                    if ( info ) {
                        send_skin_data( client_sock, SEND_SCREEN_SHOT, info->pixel_data, info->pixel_data_length, 1 );
                        free_screenshot_info( info );
                    } else {
                        ERR( "Fail to get screenshot data.\n" );
                    }

                    break;
                }
                case RECV_DETAIL_INFO: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_DETAIL_INFO ==\n" );
                    TRACE( log_buf );

                    DetailInfo* detail_info = get_detail_info( qmu_argc, qmu_argv );

                    if ( detail_info ) {
                        send_skin_data( client_sock, SEND_DETAIL_INFO, (unsigned char*) detail_info->data,
                            detail_info->data_length, 0 );
                        free_detail_info( detail_info );
                    } else {
                        ERR( "Fail to get detail info.\n" );
                    }

                    break;
                }
                case RECV_RESPONSE_HEART_BEAT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_RESPONSE_HEART_BEAT ==\n" );
//                    TRACE( log_buf );
                    TRACE( "recv HB req_id:%d\n", req_id );

                    pthread_mutex_lock( &mutex_recv_heartbeat_count );
                    recv_heartbeat_count = 0;
                    pthread_mutex_unlock( &mutex_recv_heartbeat_count );

                    break;
                }
                case RECV_OPEN_SHELL: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_OPEN_SHELL ==\n" );
                    TRACE( log_buf );

                    open_shell();
                    break;
                }
                case RECV_USB_KBD: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_USB_KBD ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        INFO( "there is no data looking at 0 length." );
                        continue;
                    }

                    char on = 0;
                    memcpy( &on, recvbuf, sizeof( on ) );
                    onoff_usb_kbd( on );
                    break;
                }
                case RECV_CLOSE: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_CLOSE ==\n" );
                    TRACE( log_buf );

                    request_close();
                    break;
                }
                case RECV_RESPONSE_SHUTDOWN: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_RESPONSE_SHUTDOWN ==\n" );
                    INFO( log_buf );

                    stop_server = 1;
                    break;
                }
                default: {
                    log_cnt += sprintf( log_buf + log_cnt, "!!! unknown command : %d\n", cmd );
                    TRACE( log_buf );

                    ERR( "!!! unknown command : %d\n", cmd );
                    break;
                }
                }

            }

        }

    }

    stop_heart_beat();

cleanup:
    if ( server_sock ) {
#ifdef _WIN32
        closesocket( server_sock );
#else
        close( server_sock );
#endif
    }

    if( shutdown_qmu ) {
        ERR( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        ERR( "!!! Fail to initialize for skin server operation. Shutdown QEMU !!!\n" );
        ERR( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        shutdown_qemu_gracefully();
    }

    return NULL;
}

static int recv_n( int client_sock, char* read_buf, int recv_len ) {

    int total_cnt = 0;
    int recv_cnt = 0;

    while ( 1 ) {

        recv_cnt = recv( client_sock, (void*) ( read_buf + recv_cnt ), ( recv_len - recv_cnt ), 0 );

        if ( 0 > recv_cnt ) {

            return recv_cnt;

        } else if ( 0 == recv_cnt ) {

            if ( total_cnt == recv_len ) {
                return total_cnt;
            } else {
                continue;
            }

        } else {

            total_cnt += recv_cnt;

            if ( total_cnt == recv_len ) {
                return total_cnt;
            } else {
                continue;
            }

        }

    }

    return 0;

}

static void make_header( int client_sock, short send_cmd, int data_length, char* sendbuf, int print_log ) {

    memset( sendbuf, 0, SEND_HEADER_SIZE );

    int request_id = ( MAX_REQ_ID == seq_req_id ) ? 0 : ++seq_req_id;
    if( print_log ) {
        TRACE( "== SEND skin request_id:%d, send_cmd:%d ==\n", request_id, send_cmd );
    }
    request_id = htonl( request_id );

    short cmd = send_cmd;
    cmd = htons( cmd );

    int length = data_length;
    length = htonl( length );

    char* p = sendbuf;
    memcpy( p, &request_id, sizeof( request_id ) );
    p += sizeof( request_id );
    memcpy( p, &cmd, sizeof( cmd ) );
    p += sizeof( cmd );
    memcpy( p, &length, sizeof( length ) );

}

static int send_n( int client_sock, unsigned char* data, int length, int big_data ) {

    int send_cnt = 0;
    int total_cnt = 0;

    int buf_size = big_data ? SEND_BIG_BUF_SIZE : SEND_BUF_SIZE;

    // use malloc instead of general array definition to avoid seg fault in 'alloca' in MinGW env, only using big buf size.
    char* databuf = (char*)g_malloc0( buf_size );

    INFO( "send_n start. length:%d\n", length );

    while ( 1 ) {

        if ( total_cnt == length ) {
            break;
        }

        if ( buf_size < ( length - total_cnt ) ) {
            send_cnt = buf_size;
        } else {
            send_cnt = ( length - total_cnt );
        }

        memset( databuf, 0, send_cnt );
        memcpy( databuf, (char*) ( data + total_cnt ), send_cnt );

        send_cnt = send( client_sock, databuf, send_cnt, 0 );

        if ( 0 > send_cnt ) {
            ERR( "send_n error. error code:%d\n", send_cnt );
            return send_cnt;
        } else {
            total_cnt += send_cnt;
        }

    }

    g_free( databuf );

    INFO( "send_n finished.\n" );

    return total_cnt;

}

static int send_skin_header_only( int client_sock, short send_cmd, int print_log ) {

    char headerbuf[SEND_HEADER_SIZE];
    make_header( client_sock, send_cmd, 0, headerbuf, print_log );

    int send_count = send( client_sock, headerbuf, SEND_HEADER_SIZE, 0 );
    return send_count;

}

static int send_skin_data( int client_sock, short send_cmd, unsigned char* data, int length, int big_data ) {

    char headerbuf[SEND_HEADER_SIZE];
    make_header( client_sock, send_cmd, length, headerbuf, 1 );

    int header_cnt = send( client_sock, headerbuf, SEND_HEADER_SIZE, 0 );

    if ( 0 > header_cnt ) {
        ERR( "send header for data is NULL.\n" );
        return header_cnt;
    }

    if ( !data ) {
        ERR( "send data is NULL.\n" );
        return -1;
    }

    int send_cnt = send_n( client_sock, data, length, big_data );
    INFO( "send_n result:%d\n", send_cnt );

    return send_cnt;

}

static void* do_heart_beat( void* args ) {

    is_started_heartbeat = 1;

    int send_fail_count = 0;
    int restart_client_count = 0;
    int need_restart_skin_client = 0;
    int shutdown = 0;

    while ( 1 ) {

        struct timeval current;
        gettimeofday( &current, NULL );

        struct timespec ts_heartbeat;
        ts_heartbeat.tv_sec = current.tv_sec + HEART_BEAT_INTERVAL;
        ts_heartbeat.tv_nsec = current.tv_usec * 1000;

        pthread_mutex_lock( &mutex_heartbeat );
        pthread_cond_timedwait( &cond_heartbeat, &mutex_heartbeat, &ts_heartbeat );
        pthread_mutex_unlock( &mutex_heartbeat );

        if ( stop_heartbeat ) {
            INFO( "[HB] stop heart beat.\n" );
            break;
        }

        if( client_sock ) {
            TRACE( "send HB\n" );
            if ( 0 > send_skin_header_only( client_sock, SEND_HEART_BEAT, 0 ) ) {
                send_fail_count++;
            } else {
                send_fail_count = 0;
            }
        }else {
            // fail to get socket in accepting or client is not yet accepted.
            send_fail_count++;
            TRACE( "[HB] client socket is NULL yet.\n" );
        }

        if ( HEART_BEAT_FAIL_COUNT < send_fail_count ) {
            ERR( "[HB] fail to send heart beat to skin. fail count:%d\n", HEART_BEAT_FAIL_COUNT );
            need_restart_skin_client = 1;
        }

        pthread_mutex_lock( &mutex_recv_heartbeat_count );
        recv_heartbeat_count++;
        if( 1 < recv_heartbeat_count ) {
            TRACE( "[HB] recv_heartbeat_count:%d\n", recv_heartbeat_count );
        }
        pthread_mutex_unlock( &mutex_recv_heartbeat_count );

        if ( HEART_BEAT_EXPIRE_COUNT < recv_heartbeat_count ) {
            ERR( "received heartbeat count is expired.\n" );
            need_restart_skin_client = 1;
        }

        if( need_restart_skin_client ) {

            if ( RESTART_CLIENT_MAX_COUNT <= restart_client_count ) {
                shutdown = 1;
                break;
            } else {

                if ( is_requested_shutdown_qemu_gracefully() ) {
                    INFO( "requested shutdown_qemu_gracefully, do not retry starting skin client process.\n" );
                    break;
                }else {

                    send_fail_count = 0;
                    recv_heartbeat_count = 0;
                    need_restart_skin_client = 0;
                    restart_client_count++;

                    WARN( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
                    WARN( "!!! restart skin client process !!!\n" );
                    WARN( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );

                    is_force_close_client = 1;
                    if ( client_sock ) {
                        if ( client_sock ) {
#ifdef _WIN32
                            closesocket( client_sock );
#else
                            close( client_sock );
#endif
                        }
                    }

                    start_skin_client( skin_argc, skin_argv );

                }

            }

        }

    }

    if ( shutdown ) {

        INFO( "[HB] shutdown skin_server by heartbeat thread.\n" );

        is_force_close_client = 1;
        if ( client_sock ) {
            if ( client_sock ) {
#ifdef _WIN32
                closesocket( client_sock );
#else
                close( client_sock );
#endif
            }
        }

        stop_server = 1;

        if ( server_sock ) {
#ifdef _WIN32
            closesocket( server_sock );
#else
            close( server_sock );
#endif
        }

        ERR( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        ERR( "!!! Fail to operate with heartbeat from skin client. Shutdown QEMU !!!\n" );
        ERR( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );

        shutdown_qemu_gracefully();

    }

    return NULL;

}

static int start_heart_beat( void ) {

    if( is_started_heartbeat ) {
        return 1;
    }

    if( ignore_heartbeat ) {
        return 1;
    }else {
        if ( 0 != pthread_create( &thread_id_heartbeat, NULL, do_heart_beat, NULL ) ) {
            ERR( "[HB] fail to create heartbean pthread.\n" );
            return 0;
        } else {
            return 1;
        }
    }

}

static void stop_heart_beat( void ) {
    pthread_mutex_lock( &mutex_heartbeat );
    stop_heartbeat = 1;
    pthread_cond_signal( &cond_heartbeat );
    pthread_mutex_unlock( &mutex_heartbeat );
}
