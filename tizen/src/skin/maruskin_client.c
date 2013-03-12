/*
 * communicate with java skin process
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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


#include "maru_common.h"
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
#include "emul_state.h"
#include "maruskin_operation.h"

#ifdef CONFIG_WIN32
#include "maru_err_table.h"
#include <windows.h>
#endif

MULTI_DEBUG_CHANNEL(qemu, skin_client);


#define SKIN_SERVER_READY_TIME 3 // second
#define SKIN_SERVER_SLEEP_TIME 10 // milli second

#define OPT_SVR_PORT "svr.port"
#define OPT_UID "uid"
#define OPT_VM_PATH "vm.path"
#define OPT_NET_BASE_PORT "net.baseport"
#define OPT_MAX_TOUCHPOINT "max.touchpoint"

extern char tizen_target_path[];

static int skin_argc;
static char** skin_argv;

static void* run_skin_client(void* arg)
{
    char cmd[JAVA_MAX_COMMAND_LENGTH] = { 0, };
    char argv[JAVA_MAX_COMMAND_LENGTH] = { 0, };

    INFO("run skin client\n");
    int i;
    for (i = 0; i < skin_argc; ++i) {
        strncat(argv, skin_argv[i], strlen(skin_argv[i]));
        strncat(argv, " ", 1);
        INFO("[skin args %d] %s\n", i, skin_argv[i]);
    }

    int skin_server_port = get_skin_server_port();

    //srand( time( NULL ) );
    int uid = 0; //rand();
    //INFO( "generated skin uid:%d\n", uid );

    char* vm_path = tizen_target_path;
    //INFO( "vm_path:%s\n", vm_path );
    char buf_skin_server_port[16];
    char buf_uid[16];
    char buf_tizen_base_port[16];
    sprintf(buf_skin_server_port, "%d", skin_server_port);
    sprintf(buf_uid, "%d", uid);
    sprintf(buf_tizen_base_port, "%d", tizen_base_port);

#ifdef CONFIG_WIN32
    // find java path in 64bit windows
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
    int len_maxtouchpoint;
    if(maxtouchpoint > 9) {
        len_maxtouchpoint = 2;
    }else {
        len_maxtouchpoint = 1;
    }

    int len = strlen(JAVA_EXEFILE_PATH) + strlen(JAVA_EXEOPTION) +
#ifdef CONFIG_WIN32
            strlen((char*)bin_dir_win) + strlen(bin_dir) + strlen(JAR_SKINFILE) +
#else
            strlen(bin_dir) + strlen(bin_dir) + strlen(JAR_SKINFILE) +
#endif
        strlen(OPT_SVR_PORT) + strlen(buf_skin_server_port) + strlen(OPT_UID) + strlen(buf_uid) +
        strlen(OPT_VM_PATH) + strlen(vm_path) + strlen(OPT_NET_BASE_PORT) + strlen(buf_tizen_base_port) +
        strlen(OPT_MAX_TOUCHPOINT) + len_maxtouchpoint + strlen(argv) + 46;
    if (len > JAVA_MAX_COMMAND_LENGTH) {
        INFO("swt command length is too long! (%d)\n", len);
        len = JAVA_MAX_COMMAND_LENGTH;
    }

    snprintf(cmd, len, "%s %s %s=\"%s\" \"%s%s\" %s=\"%d\" %s=\"%d\" %s=\"%s\" %s=\"%d\" %s=%d %s",
        JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH,
#ifdef CONFIG_WIN32
        bin_dir_win, bin_dir, JAR_SKINFILE,
#else
        bin_dir, bin_dir, JAR_SKINFILE,
#endif
        OPT_SVR_PORT, skin_server_port,
        OPT_UID, uid,
        OPT_VM_PATH, vm_path,
        OPT_NET_BASE_PORT, tizen_base_port,
        OPT_MAX_TOUCHPOINT, maxtouchpoint,
        argv );

    INFO("command for swt : %s\n", cmd);

#ifdef CONFIG_WIN32
    // for 64bit windows
    free(JAVA_EXEFILE_PATH);
    JAVA_EXEFILE_PATH=0;

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

        //retrieves the termination status of the specified process
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

#else //ifndef CONFIG_WIN32
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

        if (ret != 0) {
            /* child process is terminated with some problem.
            So qemu process will terminate, too. immediately. */
            shutdown_qemu_gracefully();
        }
    }

#endif

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
        ERR( "fail to create skin_client pthread.\n" );
        return -1;
    }

    return 1;
}

int start_simple_client(char* msg)
{
    int ret = 0;
    char cmd[JAVA_MAX_COMMAND_LENGTH] = { 0, };

    INFO("run simple client\n");

#ifdef CONFIG_WIN32
    // find java path in 64bit windows
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


    int len = strlen(JAVA_EXEFILE_PATH) + strlen(JAVA_EXEOPTION) + strlen(JAVA_LIBRARY_PATH) +
#ifdef CONFIG_WIN32
            strlen((char*)bin_dir_win) + strlen(bin_dir) + strlen(JAR_SKINFILE) +
#else
            strlen(bin_dir) + strlen(bin_dir) + strlen(JAR_SKINFILE) +
#endif
            strlen(bin_dir) + strlen(JAVA_SIMPLEMODE_OPTION) + strlen(msg) + 11;
    if (len > JAVA_MAX_COMMAND_LENGTH) {
        len = JAVA_MAX_COMMAND_LENGTH;
    }
    snprintf(cmd, len, "%s %s %s=\"%s\" %s%s %s=\"%s\"",
#ifdef CONFIG_WIN32
    JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH, bin_dir_win,
#else
    JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAVA_LIBRARY_PATH, bin_dir,
#endif
    bin_dir, JAR_SKINFILE, JAVA_SIMPLEMODE_OPTION, msg);
    INFO("command for swt : %s\n", cmd);

#ifdef CONFIG_WIN32
    // for 64bit windows
    free(JAVA_EXEFILE_PATH);
    JAVA_EXEFILE_PATH=0;

    ret = WinExec(cmd, SW_SHOW);
#else
    ret = system(cmd);
#endif

    INFO("child return value : %d\n", ret);

    return 1;
}

#ifdef CONFIG_WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

int is_wow64(void)
{
    int result = 0;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&result))
        {
            //handle error
            INFO("Can not find 'IsWow64Process'\n");
        }
    }
    return result;
}

int get_java_path(char** java_path)
{
    HKEY hKeyNew;
    HKEY hKey;
    //char strJavaRuntimePath[JAVA_MAX_COMMAND_LENGTH] = {0};
    char strChoosenName[JAVA_MAX_COMMAND_LENGTH] = {0};
    char strSubKeyName[JAVA_MAX_COMMAND_LENGTH] = {0};
    char strJavaHome[JAVA_MAX_COMMAND_LENGTH] = {0};
    int index;
    DWORD dwSubKeyNameMax = JAVA_MAX_COMMAND_LENGTH;
    DWORD dwBufLen = JAVA_MAX_COMMAND_LENGTH;

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Runtime Environment", 0,
                                     KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | MY_KEY_WOW64_64KEY, &hKey);
    RegEnumKeyEx(hKey, 0, (LPSTR)strSubKeyName, &dwSubKeyNameMax, NULL, NULL, NULL, NULL);
    strcpy(strChoosenName, strSubKeyName);

    index = 1;
    while (ERROR_SUCCESS == RegEnumKeyEx(hKey, index, (LPSTR)strSubKeyName, &dwSubKeyNameMax,
            NULL, NULL, NULL, NULL)) {
        if (strcmp(strChoosenName, strSubKeyName) < 0) {
            strcpy(strChoosenName, strSubKeyName);
        }
        index++;
    }

    RegOpenKeyEx(hKey, strChoosenName, 0, KEY_QUERY_VALUE | MY_KEY_WOW64_64KEY, &hKeyNew);
    RegQueryValueEx(hKeyNew, "JavaHome", NULL, NULL, (LPBYTE)strJavaHome, &dwBufLen);
    RegCloseKey(hKey);
    if (strJavaHome[0] != '\0') {
        sprintf(*java_path, "\"%s\\bin\\java\"", strJavaHome);
        //strcpy(*java_path, strJavaHome);
        //strcat(*java_path, "\\bin\\java");
    } else {
        return 0;
    }
    return 1;
}
#endif
