/*
 * communicate with java skin process
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
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "maruskin_client.h"
#include "maruskin_server.h"
#include "debug_ch.h"

#define SKIN_SERVER_READY_TIME 3 // second
#define SKIN_SERVER_SLEEP_TIME 10 // milli second

#define JAR_SKINFILE_PATH "EmulatorSkin.jar"
#define JAVA_EXEFILE_PATH "java"
#define JAVA_EXEOPTION "-jar"

#define OPT_SVR_PORT "svr.port"
#define OPT_UID "uid"

MULTI_DEBUG_CHANNEL( qemu, maruskin_client );

// function modified by caramis
// for delivery argv

static int skin_argc;
static char** skin_argv;

static void* run_skin_client(void* arg)
{
    char cmd[256];
    char argv[200];

    int i;
    for(i = 0; i < skin_argc; ++i) {
        strncat(argv, skin_argv[i], strlen(skin_argv[i]));
        strncat(argv, " ", 1);
    }

    int skin_server_port = get_skin_server_port();
    int uid = rand();

    INFO( "generated skin uid:%d\n", uid );

    sprintf( cmd, "%s %s %s %s=%d %s=%d %s", JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAR_SKINFILE_PATH,
        OPT_SVR_PORT, skin_server_port,
        OPT_UID, uid,
        argv );

    if( system(cmd) ) {

    }else {

    }

    return NULL;
}

int start_skin_client(int argc, char* argv[])
{

    int count = 0;
    int skin_server_ready = 0;

    while( 1 ) {

        if( 100 * SKIN_SERVER_READY_TIME < count ) {
            break;
        }

        if( is_ready_skin_server() ) {
            skin_server_ready = 1;
            break;
        }else {
            count++;
            INFO( "sleep for ready. count:%d\n", count );
#ifdef _WIN32
        Sleep( SKIN_SERVER_SLEEP_TIME );
#else
        usleep( 1000 * SKIN_SERVER_SLEEP_TIME );
#endif
        }

    }

    if( !skin_server_ready ) {
        ERR( "skin_server is not ready.\n" );
        return -1;
    }

    skin_argc = argc;
    skin_argv = argv;

    pthread_t thread_id;

    if (0 != pthread_create(&thread_id, NULL, run_skin_client, NULL)) {
        ERR( "fail to create skin_client pthread.\n" );
        return -1;
    }

    return 1;
}

