/*
 * communicate with java skin process
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

#include "qemu-common.h"

#include "maru_common.h"
#include "maruskin_client.h"
#include "maruskin_server.h"
#include "emulator.h"
#include "sdb.h"
#include "debug_ch.h"
#include "emul_state.h"
#include "maruskin_operation.h"

#ifdef CONFIG_WIN32
#include <windows.h>
#include "maru_err_table.h"
#endif

MULTI_DEBUG_CHANNEL(qemu, skin_client);


#define SKIN_SERVER_READY_TIME 3 /* second */
#define SKIN_SERVER_SLEEP_TIME 10 /* milli second */

#define SPACE_LEN 1
#define QUOTATION_LEN 2
#define EQUAL_LEN 1

#define OPT_UID "uid"
#define OPT_VM_PATH "vm.path"
#define OPT_VM_SKIN_PORT "vm.skinport"
#define OPT_VM_BASE_PORT "vm.baseport"
#define OPT_DISPLAY_SHM "display.shm"
#define OPT_INPUT_MOUSE "input.mouse"
#define OPT_INPUT_TOUCH "input.touch"
#define OPT_MAX_TOUCHPOINT "input.touch.maxpoint"

#define OPT_BOOLEAN_TRUE "true"
#define OPT_BOOLEAN_FALSE "false"

extern char tizen_target_path[];

static int skin_argc;
static char** skin_argv;

#ifdef CONFIG_WIN32
static char* JAVA_EXEFILE_PATH = NULL;
#endif

static void *run_skin_client(void *arg)
{
    gchar *cmd = NULL;
    char argv[JAVA_MAX_COMMAND_LENGTH] = { 0, };

    INFO("run skin client\n");
    int i;
    for (i = 0; i < skin_argc; ++i) {
        strncat(argv, "\"", 1);
        strncat(argv, skin_argv[i], strlen(skin_argv[i]));
        strncat(argv, "\" ", 2);
        INFO("[skin args %d] %s\n", i, skin_argv[i]);
    }

    //srand( time( NULL ) );
    int uid = 0; //rand();
    //INFO( "generated skin uid:%d\n", uid );

    char* vm_path = tizen_target_path;
    //INFO( "vm_path:%s\n", vm_path );

    int skin_server_port = get_skin_server_port();
    int vm_base_port = get_emul_vm_base_port();

    char buf_skin_server_port[16];
    char buf_uid[16];
    char buf_vm_base_port[16];
    sprintf(buf_skin_server_port, "%d", skin_server_port);
    sprintf(buf_uid, "%d", uid);
    sprintf(buf_vm_base_port, "%d", vm_base_port);

    /* display */
    char buf_display_shm[8] = { 0, };
#ifdef CONFIG_USE_SHM
    strcpy(buf_display_shm, OPT_BOOLEAN_TRUE); /* maru_shm */
#else
    strcpy(buf_display_shm, OPT_BOOLEAN_FALSE); /* maru_sdl */
#endif

    /* input */
    char buf_input[12] = { 0, };
    if (is_emul_input_mouse_enable() == true)
        strcpy(buf_input, OPT_INPUT_MOUSE);
    else
        strcpy(buf_input, OPT_INPUT_TOUCH);

#ifdef CONFIG_WIN32
    /* find java path in 64bit windows */
    JAVA_EXEFILE_PATH = malloc(JAVA_MAX_COMMAND_LENGTH);
    memset(JAVA_EXEFILE_PATH, 0, JAVA_MAX_COMMAND_LENGTH);
    if (is_wow64()) {
        INFO("This process is running under WOW64.\n");
        if (!get_java_path(&JAVA_EXEFILE_PATH)) {
             strcpy(JAVA_EXEFILE_PATH, "java");
        }
    } else {
        strcpy(JAVA_EXEFILE_PATH, "java");
    }

    char* bin_dir = get_bin_path();
    int bin_len = strlen(bin_dir);
    char bin_dir_win[bin_len];
    strcpy(bin_dir_win, bin_dir);
    bin_dir_win[strlen(bin_dir_win) -1] = '\0';
#else
    char* bin_dir = get_bin_path();
#endif
    INFO("bin directory : %s\n", bin_dir);

    int maxtouchpoint = get_emul_max_touch_point();
    int len_maxtouchpoint = 0;
    if (maxtouchpoint > 9) {
        len_maxtouchpoint = 2;
    } else {
        len_maxtouchpoint = 1;
    }

    /* calculate buffer length */
    int cmd_len = strlen(JAVA_EXEFILE_PATH) + SPACE_LEN +
        strlen(JAVA_EXEOPTION) + SPACE_LEN +
        strlen(JAVA_LIBRARY_PATH) + EQUAL_LEN +
#ifdef CONFIG_WIN32
            QUOTATION_LEN + strlen((char*)bin_dir_win) + SPACE_LEN +
        QUOTATION_LEN + strlen(bin_dir) + strlen(JAR_SKINFILE) + SPACE_LEN +
#else
            QUOTATION_LEN + strlen(bin_dir) + SPACE_LEN +
        QUOTATION_LEN + strlen(bin_dir) + strlen(JAR_SKINFILE) + SPACE_LEN +
#endif

        strlen(OPT_VM_SKIN_PORT) + EQUAL_LEN +
            strlen(buf_skin_server_port) + SPACE_LEN +
        strlen(OPT_UID) + EQUAL_LEN +
            strlen(buf_uid) + SPACE_LEN +
        strlen(OPT_VM_PATH) + EQUAL_LEN +
            QUOTATION_LEN + strlen(vm_path) + SPACE_LEN +
        strlen(OPT_VM_BASE_PORT) + EQUAL_LEN +
            strlen(buf_vm_base_port) + SPACE_LEN +
        strlen(OPT_DISPLAY_SHM) + EQUAL_LEN +
            strlen(buf_display_shm) + SPACE_LEN +
        strlen(buf_input) + EQUAL_LEN +
            strlen(OPT_BOOLEAN_TRUE) + SPACE_LEN +
        strlen(OPT_MAX_TOUCHPOINT) + EQUAL_LEN +
            len_maxtouchpoint + SPACE_LEN + 1 +
        strlen(argv);

    INFO("skin command length : %d\n", cmd_len);
    cmd = g_malloc0(cmd_len);

    /*if (len > JAVA_MAX_COMMAND_LENGTH) {
        INFO("swt command length is too long! (%d)\n", len);
        len = JAVA_MAX_COMMAND_LENGTH;
    }*/

    snprintf(cmd, cmd_len, "%s %s %s=\"%s\" \
\"%s%s\" \
%s=%d \
%s=%d \
%s=\"%s\" \
%s=%d \
%s=%s \
%s=%s \
%s=%d \
%s",
        JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH,
#ifdef CONFIG_WIN32
        bin_dir_win, bin_dir, JAR_SKINFILE,
#else
        bin_dir, bin_dir, JAR_SKINFILE,
#endif
        OPT_VM_SKIN_PORT, skin_server_port,
        OPT_UID, uid,
        OPT_VM_PATH, vm_path,
        OPT_VM_BASE_PORT, vm_base_port,
        OPT_DISPLAY_SHM, buf_display_shm,
        buf_input, OPT_BOOLEAN_TRUE,
        OPT_MAX_TOUCHPOINT, maxtouchpoint,
        argv);

    INFO("command for swt : %s\n", cmd);

#ifdef CONFIG_WIN32
    /* for 64bit windows */
    free(JAVA_EXEFILE_PATH);
    JAVA_EXEFILE_PATH = NULL;

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
            ERR("Unable to generate process! error %u\n", GetLastError());
            maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                "CreateProcess function failed. Unable to generate process.");
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
            INFO("time-out interval elapsed,\
                and the child thread's state is nonsignaled.\n");
            break;
        case WAIT_FAILED:
            ERR("WaitForSingleObject() failed, error %u\n", GetLastError());
            break;
        }

        /* retrieves the termination status of the specified process */
        if (GetExitCodeProcess(pi.hProcess, &dwRet) != 0) {
            ERR("failed to GetExitCodeProcess, error %u\n", GetLastError());
        }
        INFO("child return value : %d\n", dwRet);

        if (dwRet != 0) {
            /* child process is terminated with some problem.
            So qemu process will terminate, too. immediately. */
            shutdown_qemu_gracefully();
        }

        if (CloseHandle(pi.hProcess) != 0) {
            INFO("child thread handle was closed successfully!\n");
        } else {
            ERR("failed to close child thread handle, error %u\n", GetLastError());
        }
    }

#else /* ifndef CONFIG_WIN32 */
    int ret = system(cmd);

    if (ret == 127) {
        INFO("can't execute /bin/sh!\n");
    } else if(ret == -1) {
        INFO("fork error!\n");
    } else {
        ret = WEXITSTATUS(ret);
        /* The high-order 8 bits are the exit code from exit() */
        /* The low-order 8 bits are zero if the process exited normally */
        INFO("child return value : %d\n", ret);

        if (ret != 0) {
            /* child process is terminated with some problem.
            So qemu process will terminate, too. immediately. */
            shutdown_qemu_gracefully();
        }
    }

#endif

    g_free(cmd);

    return NULL;
}

int start_skin_client(int argc, char* argv[])
{
    int count = 0;
    int skin_server_ready = 0;

    while(1) {
        if (100 * SKIN_SERVER_READY_TIME < count) {
            break;
        }

        if (is_ready_skin_server()) {
            skin_server_ready = 1;
            break;
        } else {
            count++;
            INFO("sleep for ready. count:%d\n", count);

#ifdef CONFIG_WIN32
            Sleep(SKIN_SERVER_SLEEP_TIME);
#else
            usleep(1000 * SKIN_SERVER_SLEEP_TIME);
#endif
        }

    }

    if (!skin_server_ready) {
        ERR("skin_server is not ready.\n");
        return -1;
    }

    skin_argc = argc;
    skin_argv = argv;

    pthread_t thread_id;

    if (0 != pthread_create(&thread_id, NULL, run_skin_client, NULL)) {
        ERR("fail to create skin_client pthread\n");
        return -1;
    }

    return 1;
}

int start_simple_client(char* msg)
{
    int ret = 0;
    gchar *cmd = NULL;

    INFO("run simple client\n");

#ifdef CONFIG_WIN32
    /* find java path in 64bit windows */
    JAVA_EXEFILE_PATH = malloc(JAVA_MAX_COMMAND_LENGTH);
    memset(JAVA_EXEFILE_PATH, 0, JAVA_MAX_COMMAND_LENGTH);
    if (is_wow64()) {
        INFO("This process is running under WOW64.\n");
        if (!get_java_path(&JAVA_EXEFILE_PATH)) {
             strcpy(JAVA_EXEFILE_PATH, "java");
        }
    } else {
        strcpy(JAVA_EXEFILE_PATH, "java");
    }

    char* bin_dir = get_bin_path();
    int bin_dir_len = strlen(bin_dir);
    char bin_dir_win[bin_dir_len];
    strcpy(bin_dir_win, bin_dir);
    bin_dir_win[strlen(bin_dir_win) -1] = '\0';
#else
    char* bin_dir = get_bin_path();
#endif
    INFO("bin directory : %s\n", bin_dir);

    /* calculate buffer length */
    int cmd_len = strlen(JAVA_EXEFILE_PATH) + SPACE_LEN +
        strlen(JAVA_EXEOPTION) + SPACE_LEN +
        strlen(JAVA_LIBRARY_PATH) + EQUAL_LEN +
#ifdef CONFIG_WIN32
            QUOTATION_LEN + strlen((char*)bin_dir_win) + SPACE_LEN +
        QUOTATION_LEN + strlen(bin_dir) + strlen(JAR_SKINFILE) + SPACE_LEN +
#else
            QUOTATION_LEN + strlen(bin_dir) + SPACE_LEN +
        QUOTATION_LEN + strlen(bin_dir) + strlen(JAR_SKINFILE) + SPACE_LEN +
#endif

        strlen(JAVA_SIMPLEMODE_OPTION) + EQUAL_LEN +
        QUOTATION_LEN + strlen(msg) + 1;

    INFO("skin command length : %d\n", cmd_len);
    cmd = g_malloc0(cmd_len);

    snprintf(cmd, cmd_len, "%s %s %s=\"%s\" \"%s%s\" %s=\"%s\"",
#ifdef CONFIG_WIN32
        JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH, bin_dir_win,
#else
        JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH, bin_dir,
#endif
        bin_dir, JAR_SKINFILE, JAVA_SIMPLEMODE_OPTION, msg);

    INFO("command for swt : %s\n", cmd);

#ifdef CONFIG_WIN32
    /* for 64bit windows */
    free(JAVA_EXEFILE_PATH);
    JAVA_EXEFILE_PATH=0;

    ret = WinExec(cmd, SW_SHOW);
#else
    ret = system(cmd);
#endif

    INFO("child return value : %d\n", ret);

    g_free(cmd);

    return 1;
}
