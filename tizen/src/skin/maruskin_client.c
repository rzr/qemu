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
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "maruskin_client.h"
#include "maruskin_server.h"
#include "emulator.h"
#include "sdb.h"
#include "debug_ch.h"

#ifdef _WIN32
#include <windows.h>
#endif

MULTI_DEBUG_CHANNEL(qemu, maruskin_client);



#define SKIN_SERVER_READY_TIME 3 // second
#define SKIN_SERVER_SLEEP_TIME 10 // milli second

#define JAR_SKINFILE_PATH "emulator-skin.jar"
#define JAVA_EXEFILE_PATH "java"
#define JAVA_EXEOPTION "-jar"

#define OPT_SVR_PORT "svr.port"
#define OPT_UID "uid"
#define OPT_VM_PATH "vm.path"
#define OPT_NET_BASE_PORT "net.baseport"

#define MAX_COMMAND 512

static int skin_argc;
static char** skin_argv;

static void* run_skin_client(void* arg)
{
    char cmd[MAX_COMMAND] = {0};
    char argv[256] = {0};

    INFO("run skin client\n");
    int i;
    for (i = 0; i < skin_argc; ++i) {
        strncat(argv, skin_argv[i], strlen(skin_argv[i]));
        strncat(argv, " ", 1);
        INFO( "[skin args %d] %s\n", i, skin_argv[i] );
    }

    int skin_server_port = get_skin_server_port();

    //srand( time( NULL ) );
    int uid = 0; //rand();
    //INFO( "generated skin uid:%d\n", uid );

    char* vm_path = tizen_target_path;
    //INFO( "vm_path:%s\n", vm_path );

    sprintf( cmd, "%s %s %s %s=\"%d\" %s=\"%d\" %s=\"%s\" %s=\"%d\" %s", JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAR_SKINFILE_PATH,
        OPT_SVR_PORT, skin_server_port,
        OPT_UID, uid,
        OPT_VM_PATH, vm_path,
        OPT_NET_BASE_PORT, tizen_base_port,
        argv );
    INFO( "command for swt : %s\n", cmd );

#ifdef _WIN32
    //WinExec( cmd, SW_SHOW );
    {
        STARTUPINFO sti = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        if (!CreateProcess(NULL,
                          cmd,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &sti,
                          &pi))
        {
            ERR("Unable to generate process!error %u\n", GetLastError());
            exit(1);
        }

        INFO("wait for single object..\n");
        DWORD dwRet = WaitForSingleObject(
                          pi.hProcess, // process handle
                          INFINITE);

        switch(dwRet) {
        case WAIT_OBJECT_0:
            INFO("the child thread state was signaled!\n");
            break;
        case WAIT_TIMEOUT:
            INFO("time-out interval elapsed, and the child thread's state is nonsignaled.\n");
            break;
        case WAIT_FAILED:
            ERR("WaitForSingleObject() failed, error %u\n", GetLastError());
            break;
        }

        //retrieves the termination status of the specified process
        GetExitCodeProcess(pi.hProcess, &dwRet);
        INFO("child return value : %d\n", dwRet);

        if (dwRet == -1) {
            //TODO:
            exit(1);
        }

        if (CloseHandle(pi.hProcess) != 0) {
            INFO("child thread handle was closed successfully!\n");
        } else {
            ERR("failed to close child thread handle, error %u\n", GetLastError());
        }
    }

#else //ifndef _WIN32
    int ret = system(cmd);

    if (ret == 127) {
        INFO("can't execute /bin/sh!\n");
    } else if(ret == -1) {
        INFO("fork error!\n");
    } else {
        ret = WEXITSTATUS(ret);
        //The high-order 8 bits are the exit code from exit().
        //The low-order 8 bits are zero if the process exited normally.
        INFO("child return value : %d\n", ret);

        if (ret == 0xff) {
            //TODO:
            exit(1);
        }
    }

#endif

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

        if ( is_ready_skin_server() ) {
            skin_server_ready = 1;
            break;
        } else {
            count++;
            INFO( "sleep for ready. count:%d\n", count );
#ifdef _WIN32
        Sleep( SKIN_SERVER_SLEEP_TIME );
#else
        usleep( 1000 * SKIN_SERVER_SLEEP_TIME );
#endif
        }

    }

    if ( !skin_server_ready ) {
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


int start_simple_client(char* msg) {
    int ret = 0;
    char cmd[MAX_COMMAND] = {0};

    INFO("run simple client\n");

    sprintf(cmd, "%s %s %s %s=\"%s\"", JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAR_SKINFILE_PATH, "simple.msg", msg);
    INFO("command for swt : %s\n", cmd);

#ifdef __WIN32
    ret = WinExec(cmd, SW_SHOW);
#else
    ret = system(cmd);
#endif

    INFO("child return value : %d\n", ret);

    return 1;
}

