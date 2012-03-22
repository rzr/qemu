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

MULTI_DEBUG_CHANNEL( qemu, maruskin_server );

#define RECV_BUF_SIZE 32
#define RECV_HEADER_SIZE 12
#define SEND_HEADER_SIZE 6

#define HEART_BEAT_INTERVAL 1
#define HEART_BEAT_FAIL_COUNT 5
#define HEART_BEAT_EXPIRE_COUNT 5

#define PORT_RETRY_COUNT 50

#define TEST_HB_IGNORE "test.hb.ignore"

enum {
    RECV_START = 1,
    RECV_MOUSE_EVENT = 10,
    RECV_KEY_EVENT = 11,
    RECV_HARD_KEY_EVENT = 12,
    RECV_CHANGE_LCD_STATE = 13,
    RECV_OPEN_SHELL = 14,
    RECV_USB_KBD = 15,
    RECV_RESPONSE_HEART_BEAT = 900,
    RECV_CLOSE = 998,
    RECV_RESPONSE_SHUTDOWN = 999,
};

enum {
    SEND_HEART_BEAT = 1,
    SEND_HEART_BEAT_RESPONSE = 2,
    SEND_SENSOR_DAEMON_START = 800,
    SEND_SHUTDOWN = 999,
};

static uint16_t svr_port = 0;
static int server_sock = 0;
static int client_sock = 0;
static int stop_server = 0;
static int is_sensord_initialized = 0;
static int ready_server = 0;
static int ignore_heartbeat = 0;

static int stop_heartbeat = 0;
static int recv_heartbeat_count = 0;
static pthread_t thread_id_heartbeat;
static pthread_mutex_t mutex_heartbeat = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_heartbeat = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_recv_heartbeat_count = PTHREAD_MUTEX_INITIALIZER;

static void* run_skin_server( void* args );
static int recv_n( int client_sock, char* read_buf, int recv_len );
static int send_skin( int client_sock, short send_cmd );
static void* do_heart_beat( void* args );
static int start_heart_beat( int client_sock );
static void stop_heart_beat( void );

int start_skin_server( int argc, char** argv ) {

    int i;
    for( i = 0; i < argc; i++ ) {

        char* arg = NULL;
        arg = strdup( argv[i] );

        if( arg ) {

            char* key = strtok( arg, "=" );
            char* value = strtok( NULL, "=" );

            INFO( "skin params key:%s, value:%s\n", key, value );

            if( 0 == strcmp( TEST_HB_IGNORE, key ) ) {
                if( 0 == strcmp( "true", value ) ) {
                    ignore_heartbeat = 1;
                    break;
                }
            }

            free( arg );

        }else {
            ERR( "fail to strdup." );
        }

    }

    INFO( "ignore_heartbeat:%d\n", ignore_heartbeat );

    QemuThread qemu_thread;

    qemu_thread_create( &qemu_thread, run_skin_server, NULL );

    return 1;

}

void shutdown_skin_server( void ) {
    if ( client_sock ) {
        INFO( "send shutdown to skin.\n" );
        if ( 0 > send_skin( client_sock, SEND_SHUTDOWN ) ) {
            ERR( "fail to send SEND_SHUTDOWN to skin.\n" );
            stop_server = 1;
            // force close
#ifdef _WIN32
            closesocket( client_sock );
            if ( server_sock ) {
                closesocket( server_sock );
            }
#else
            close( client_sock );
            if ( server_sock ) {
                close( server_sock );
            }
#endif
        } else {
            // skin sent RECV_RESPONSE_SHUTDOWN.
        }
    }
}

void notify_sensor_daemon_start( void ) {
    INFO( "notify_sensor_daemon_start\n" );
    is_sensord_initialized = 1;
    if ( client_sock ) {
        if ( 0 > send_skin( client_sock, SEND_SENSOR_DAEMON_START ) ) {
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

static void* run_skin_server( void* args ) {

    uint16_t port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    INFO("run skin server\n");

    // min:10000 ~ max:(20000 + 10000)
    port = rand() % 20001;
    port += 10000;

    if ( 0 > ( server_sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) ) {
        ERR( "create listen socket error\n" );
        perror( "create listen socket error : " );
        goto cleanup;
    }

    int port_fail_count = 0;

    while ( 1 ) {

        memset( &server_addr, '\0', sizeof( server_addr ) );

        server_addr.sin_family = PF_INET;
        server_addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
        server_addr.sin_port = htons( port );

        int opt = 1;
        setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );

        if ( 0 > bind( server_sock, (struct sockaddr *) &server_addr, sizeof( server_addr ) ) ) {

            ERR( "skin server bind error\n" );
            perror( "skin server bind error : " );
            port++;

            if ( PORT_RETRY_COUNT < port_fail_count ) {
                goto cleanup;
            } else {
                port_fail_count++;
                INFO( "Retry bind\n" );
                continue;
            }

        } else {
            svr_port = port;
            INFO( "success to bind port[127.0.0.1:%d/tcp] for skin_server in host \n", svr_port );
            break;
        }

    }

    if ( 0 > listen( server_sock, 4 ) ) {
        ERR( "skin_server listen error\n" );
        perror( "skin_server listen error : " );
        goto cleanup;
    }

    char recvbuf[RECV_BUF_SIZE];

    INFO( "skin server start...port:%d\n", port );

    while ( 1 ) {

        if ( stop_server ) {
            break;
        }

        //TODO receive client retry ?

        INFO( "start accepting socket...\n" );

        client_len = sizeof( client_addr );

        ready_server = 1;

        if ( 0 > ( client_sock = accept( server_sock, (struct sockaddr *) &client_addr, &client_len ) ) ) {
            ERR( "skin_servier accept error\n" );
            perror( "skin_servier accept error : " );
            continue;
        }

        INFO( "accept client : client_sock:%d\n", client_sock );

        while ( 1 ) {

            if ( stop_server ) {
                INFO( "stop receiving this socket.\n" );
                break;
            }

            stop_heartbeat = 0;
            memset( &recvbuf, 0, RECV_BUF_SIZE );

            int read_cnt = recv_n( client_sock, recvbuf, RECV_HEADER_SIZE );

            if ( 0 > read_cnt ) {
                ERR( "skin_server read error:%d\n", read_cnt );
                perror( "skin_server read error : " );
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

                //TODO check identification with uid

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
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

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

                    if ( start_heart_beat( client_sock ) ) {
                        start_display( handle_id, lcd_size_width, lcd_size_height, scale_ratio, rotation );
                    } else {
                        stop_server = 1;
                    }

                    break;
                }
                case RECV_MOUSE_EVENT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_MOUSE_EVENT ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

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

                    int event_type = 0;
                    int keycode = 0;

                    char* p = recvbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &keycode, p, sizeof( keycode ) );

                    event_type = ntohl( event_type );
                    keycode = ntohl( keycode );

                    do_key_event( event_type, keycode );
                    break;
                }
                case RECV_HARD_KEY_EVENT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_HARD_KEY_EVENT ==\n" );
                    TRACE( log_buf );

                    if ( 0 >= length ) {
                        ERR( "there is no data looking at 0 length." );
                        continue;
                    }

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
                case RECV_RESPONSE_HEART_BEAT: {
                    log_cnt += sprintf( log_buf + log_cnt, "RECV_RESPONSE_HEART_BEAT ==\n" );
                    TRACE( log_buf );

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
                    TRACE( log_buf );

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
#ifdef _WIN32
    if(server_sock) {
        closesocket( server_sock );
    }
#else
    if ( server_sock ) {
        close( server_sock );
    }
#endif

    return NULL;
}

static int recv_n( int client_sock, char* read_buf, int recv_len ) {

    int total_cnt = 0;
    int recv_cnt = 0;

    while ( 1 ) {

        recv_cnt = recv( client_sock, (void*) read_buf, ( recv_len - recv_cnt ), 0 );

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

static int send_skin( int client_sock, short send_cmd ) {

    char sendbuf[SEND_HEADER_SIZE];
    memset( &sendbuf, 0, SEND_HEADER_SIZE );

    int request_id = rand();
    TRACE( "== SEND skin request_id:%d, send_cmd:%d ==\n", request_id, send_cmd );
    request_id = htonl( request_id );

    short data = send_cmd;
    data = htons( data );

    char* p = sendbuf;
    memcpy( p, &request_id, sizeof( request_id ) );
    p += sizeof( request_id );
    memcpy( p, &data, sizeof( data ) );

    ssize_t send_count = send( client_sock, sendbuf, SEND_HEADER_SIZE, 0 );

    return send_count;

}

static void* do_heart_beat( void* args ) {

    int client_sock = *(int*) args;

    int fail_count = 0;
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

        TRACE( "[HB] send heartbeat to skin.\n" );
        if ( 0 > send_skin( client_sock, SEND_HEART_BEAT ) ) {
            fail_count++;
        } else {
            fail_count = 0;
        }

        if ( HEART_BEAT_FAIL_COUNT < fail_count ) {
            ERR( "[HB] fail to write heart beat to skin. fail count:%d\n", HEART_BEAT_FAIL_COUNT );
            shutdown = 1;
            break;
        }

        int count = 0;
        pthread_mutex_lock( &mutex_recv_heartbeat_count );
        recv_heartbeat_count++;
        count = recv_heartbeat_count;
        TRACE( "[HB] recv_heartbeat_count:%d\n", recv_heartbeat_count );
        pthread_mutex_unlock( &mutex_recv_heartbeat_count );

        if ( HEART_BEAT_EXPIRE_COUNT < count ) {
            ERR( "received heartbeat count is expired.\n" );
            shutdown = 1;
            break;
        }

    }

    if ( shutdown ) {
        INFO( "[HB] shutdown skin_server by heartbeat thread.\n" );
        shutdown_skin_server();
    }

    return NULL;

}

static int start_heart_beat( int client_sock ) {

    if( ignore_heartbeat ) {
        return 1;
    }else {
        if ( 0 != pthread_create( &thread_id_heartbeat, NULL, do_heart_beat, (void*) &client_sock ) ) {
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
