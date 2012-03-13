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

MULTI_DEBUG_CHANNEL(qemu, guest_server);

//TODO change size
#define RECV_BUF_SIZE 4

static void* run_guest_server( void* args );

static int stop_svr = 0;
static int svr_port = 0;
static int server_sock = 0;
static int client_sock = 0;

enum {
    SENSOR_DAEMON_START = 3,
};

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

    if ( ( server_sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
        INFO( "create listen socket error: " );
        perror( "socket" );
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
        INFO( "success to bind port[127.0.0.1:%d/tcp] for guest_server in host \n", port );
    }

    if ( listen( server_sock, 1 ) < 0 ) {
        INFO( "guest_server listen error: " );
        perror( "listen" );
        goto cleanup;
    }

    char readbuf[RECV_BUF_SIZE];

    INFO( "guest server start...port:%d\n", port );

    while ( 1 ) {
        if ( stop_svr ) {
            break;
        }

        INFO( "start accepting socket...\n" );

        client_len = sizeof( client_addr );
        if ( 0 > ( client_sock = accept( server_sock, (struct sockaddr*) &client_addr, &client_len ) ) ) {
            ERR( "guest_servier accept error: " );
            perror( "accept" );
            continue;
        }

        while ( 1 ) {

            if ( stop_svr ) {
                INFO( "stop reading this socket.\n" );
                break;
            }

            memset( &readbuf, 0, RECV_BUF_SIZE );

            int read_cnt = read( client_sock, readbuf, RECV_BUF_SIZE );

            if ( 0 > read_cnt ) {

                perror( "error : guest_server read" );
                break;

            } else {

                if ( 0 == read_cnt ) {
                    INFO( "read_cnt is 0.\n" );
                    break;
                }

                INFO( "================= recv =================\n" );
                INFO( "read_cnt:%d\n", read_cnt );

                //TODO parse data
                char cmd = 0;
                memcpy( &cmd, readbuf, sizeof( cmd ) );
                INFO( "cmd:%s\n", cmd );

                INFO( "----------------------------------------\n" );
                switch ( cmd ) {
                case SENSOR_DAEMON_START: {
                    notify_sensor_daemon_start();
                    break;
                }
                default: {
                    ERR( "!!! unknown command : %d\n", cmd );
                    break;
                }
                }

                INFO( "========================================\n" );

            }

        }

        if ( client_sock ) {
            close( client_sock );
        }
    }

    cleanup: if ( server_sock ) {
        close( server_sock );
    }

    return NULL;
}

void shutdown_guest_server( void ) {
    stop_svr = 1;
    if ( client_sock ) {
        close( client_sock );
    }
    if ( server_sock ) {
        close( server_sock );
    }
}
