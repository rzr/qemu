/*
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JiHye Kim <jihye1128.kim@samsung.com>
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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "guest_server.h"
#include "sdb.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL( qemu, guest_server );

#define RECV_BUF_SIZE 32

static void* run_guest_server( void* args );

static int svr_port = 0;
static int server_sock = 0;

static int parse_val( char *buff, unsigned char data, char *parsbuf );

pthread_t start_guest_server( void ) {

    svr_port = get_sdb_base_port() + SDB_UDP_SENSOR_INDEX;

    pthread_t thread_id = -1;

    if ( 0 != pthread_create( &thread_id, NULL, run_guest_server, NULL ) ) {
        ERR( "fail to create guest_server pthread.\n" );
    }

    return thread_id;

}

static void* run_guest_server( void* args ) {

    uint16_t port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    port = svr_port;

    if ( ( server_sock = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
        ERR( "create listen socket error\n" );
        perror( "create listen socket error\n" );
        goto cleanup;
    }
    memset( &server_addr, '\0', sizeof( server_addr ) );
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    server_addr.sin_port = htons( port );

    int opt = 1;
    setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );

    if ( 0 > bind( server_sock, (struct sockaddr*) &server_addr, sizeof( server_addr ) ) ) {
        ERR( "guest server bind error: " );
        perror( "bind" );
        goto cleanup;
    } else {
        INFO( "success to bind port[127.0.0.1:%d/udp] for guest_server in host \n", port );
    }

    client_len = sizeof( client_addr );

    char readbuf[RECV_BUF_SIZE];

    INFO( "guest server start...port:%d\n", port );

    while ( 1 ) {

        memset( &readbuf, 0, RECV_BUF_SIZE );

        int read_cnt = recvfrom( server_sock, readbuf, RECV_BUF_SIZE, 0, (struct sockaddr*) &client_addr, &client_len );

        if ( 0 > read_cnt ) {

            perror( "error : guest_server read" );
            break;

        } else {

            if ( 0 == read_cnt ) {
                ERR( "read_cnt is 0.\n" );
                break;
            }

            TRACE( "================= recv =================\n" );
            TRACE( "read_cnt:%d\n", read_cnt );
            TRACE( "readbuf:%s\n", readbuf );

            char command[RECV_BUF_SIZE];
            memset( command, '\0', sizeof( command ) );

            parse_val( readbuf, 0x0a, command );

            TRACE( "----------------------------------------\n" );
            if ( strcmp( command, "3\n" ) == 0 ) {
                TRACE( "command:%s\n", command );
                notify_sensor_daemon_start();
            } else {
                ERR( "!!! unknown command : %s\n", command );
            }
            TRACE( "========================================\n" );

        }

    }

    cleanup:
#ifdef __MINGW32__
    if( server_sock ) {
        closesocket(server_sock);
    }
#else
    if ( server_sock ) {
        close( server_sock );
    }
#endif

    return NULL;
}

static int parse_val( char* buff, unsigned char data, char* parsbuf ) {

    int count = 0;

    while ( 1 ) {

        if ( count > 12 ) {
            return -1;
        }

        if ( buff[count] == data ) {
            count++;
            strncpy( parsbuf, buff, count );
            return count;
        }

        count++;

    }

    return 0;

}

void shutdown_guest_server( void ) {
    INFO( "shutdown_guest_server.\n" );
#ifdef __MINGW32__
    if( server_sock ) {
        closesocket( server_sock );
    }
#else
    if ( server_sock ) {
        close( server_sock );
    }
#endif
}
