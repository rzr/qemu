/* 
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
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


#include <stdlib.h>
#include <SDL.h>
#include "maru_common.h"
#include "emulator.h"
#include "sdb.h"
#include "string.h"
#include "skin/maruskin_server.h"
#include "skin/maruskin_client.h"
#include "guest_server.h"
#include "debug_ch.h"
#include "option.h"
#include "emul_state.h"
#include "qemu_socket.h"
#include "build_info.h"
#include <glib.h>
#include <glib/gstdio.h>

#if defined( _WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <linux/version.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#endif

#include "mloop_event.h"

MULTI_DEBUG_CHANNEL(qemu, main);


#define IMAGE_PATH_PREFIX   "file="
#define IMAGE_PATH_SUFFIX   ",if=virtio"
#define SDB_PORT_PREFIX     "sdb_port="
#define LOGS_SUFFIX         "/logs/"
#define LOGFILE             "emulator.log"
#define MIDBUF  128
int tizen_base_port = 0;

char tizen_target_path[MAXLEN] = {0, };
char logpath[MAXLEN] = { 0, };

static int skin_argc = 0;
static char** skin_argv = NULL;
static int qemu_argc = 0;
static char** qemu_argv = NULL;

void exit_emulator(void)
{
    cleanup_multi_touch_state();

    mloop_ev_stop();
    shutdown_skin_server();
    shutdown_guest_server();

    SDL_Quit();
}

static void construct_main_window(int skin_argc, char* skin_argv[], int qemu_argc, char* qemu_argv[] )
{
    INFO("construct main window\n");

    start_skin_server( skin_argc, skin_argv, qemu_argc, qemu_argv );

    if (get_emul_skin_enable() == 1) { //this line is check for debugging, etc..
        if ( 0 > start_skin_client(skin_argc, skin_argv) ) {
            exit( -1 );
        }
    }

    set_emul_caps_lock_state(0);
    set_emul_num_lock_state(0);
}

static void parse_options(int argc, char* argv[], int* skin_argc, char*** skin_argv, int* qemu_argc, char*** qemu_argv)
{
    int i;
    int j;

// FIXME !!!
// TODO:
   
    for(i = 1; i < argc; ++i)
    {
        if(strncmp(argv[i], "--skin-args", 11) == 0)
        {
            *skin_argv = &(argv[i + 1]);
            break;
        }
    }
    for(j = i; j < argc; ++j)
    {
        if(strncmp(argv[j], "--qemu-args", 11) == 0)
        {
            *skin_argc = j - i - 1;

            *qemu_argc = argc - j - i + 1;
            *qemu_argv = &(argv[j]);

            argv[j] = argv[0];
        }
    }
}

static void get_bin_dir( char* exec_argv ) {

    if ( !exec_argv ) {
        return;
    }

    char* data = strdup( exec_argv );
    if ( !data ) {
        fprintf( stderr, "Fail to strdup for paring a binary directory.\n" );
        return;
    }

    char* p = NULL;
#ifdef _WIN32
    p = strrchr( data, '\\' );
    if ( !p ) {
        p = strrchr( data, '/' );
    }
#else
    p = strrchr( data, '/' );
#endif
    if ( !p ) {
        free( data );
        return;
    }

    strncpy( bin_dir, data, strlen( data ) - strlen( p ) );

    free( data );

}

void set_image_and_log_path(char* qemu_argv)
{
    int i;
    int j = 0;
    int name_len = 0;
    int prefix_len = 0;
    int suffix_len = 0;
    int max = 0;
    char *path = malloc(MAXLEN);
    name_len = strlen(qemu_argv);
    prefix_len = strlen(IMAGE_PATH_PREFIX);
    suffix_len = strlen(IMAGE_PATH_SUFFIX);
    max = name_len - suffix_len;
    for(i = prefix_len , j = 0; i < max; i++)
    {
        path[j++] = qemu_argv[i];
    }
    path[j] = '\0';
    if(!g_path_is_absolute(path))
        strcpy(tizen_target_path, g_get_current_dir());
    else
        strcpy(tizen_target_path, g_path_get_dirname(path));

    strcpy(logpath, tizen_target_path);
    strcat(logpath, LOGS_SUFFIX);
#ifdef _WIN32
    if(access(g_win32_locale_filename_from_utf8(logpath), R_OK) != 0) {
       g_mkdir(g_win32_locale_filename_from_utf8(logpath), 0755); 
    }
#else
    if(access(logpath, R_OK) != 0) {
       g_mkdir(logpath, 0755); 
    }
#endif
	strcat(logpath, LOGFILE);
    set_log_path(logpath);
}

void redir_output(void)
{
	FILE *fp;

	fp = freopen(logpath, "a+", stdout);
	if(fp ==NULL)
		fprintf(stderr, "log file open error\n");
	fp = freopen(logpath, "a+", stderr);
	if(fp ==NULL)
		fprintf(stderr, "log file open error\n");

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
}

void extract_info(int qemu_argc, char** qemu_argv)
{
    int i;

    for(i = 0; i < qemu_argc; ++i)
    {
        if(strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            set_image_and_log_path(qemu_argv[i]);
            break;
        }
    }
    
    tizen_base_port = get_sdb_base_port();
}

static void system_info(void)
{
#define DIV 1024

#ifdef __linux__
    char lscmd[MAXLEN] = "lspci >> ";
#endif
    char timeinfo[64] = {0, };
    struct tm *tm_time;
    struct timeval tval;

    INFO("* SDK Version : %s\n", build_version);
    INFO("* Package %s\n", pkginfo_version);
    INFO("* User name : %s\n", g_get_real_name());
#ifdef _WIN32
    INFO("* Host name : %s\n", g_get_host_name());
#endif

    /* timestamp */
    INFO("* Build date : %s\n", build_date);
    gettimeofday(&tval, NULL);
    tm_time = localtime(&(tval.tv_sec));
    strftime(timeinfo, sizeof(timeinfo), "%Y/%m/%d %H:%M:%S", tm_time);
    INFO("* Current time : %s\n", timeinfo);

    /* Gets the version of the dynamically linked SDL library */
    INFO("* Host sdl version : (%d, %d, %d)\n",
        SDL_Linked_Version()->major, SDL_Linked_Version()->minor, SDL_Linked_Version()->patch);

#if defined( _WIN32)
    /* Retrieves information about the current os */
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osvi)) {
        INFO("* MajorVersion : %d, MinorVersion : %d, BuildNumber : %d, PlatformId : %d, CSDVersion : %s\n",
            osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.dwPlatformId, osvi.szCSDVersion);
    }

    /* Retrieves information about the current system */
    SYSTEM_INFO sysi;
    ZeroMemory(&sysi, sizeof(SYSTEM_INFO));

    GetSystemInfo(&sysi);
    INFO("* Processor type : %d, Number of processors : %d\n", sysi.dwProcessorType,  sysi.dwNumberOfProcessors);

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    INFO("* Total Ram : %llu kB, Free: %lld kB\n",
        memInfo.ullTotalPhys / DIV, memInfo.ullAvailPhys / DIV);

#elif defined(__linux__)
    /* depends on building */
    INFO("* Qemu build machine linux kernel version : (%d, %d, %d)\n",
        LINUX_VERSION_CODE >> 16, (LINUX_VERSION_CODE >> 8) & 0xff , LINUX_VERSION_CODE & 0xff);

     /* depends on launching */
    struct utsname host_uname_buf;
    if (uname(&host_uname_buf) == 0) {
        INFO("* Host machine uname : %s %s %s %s %s\n", host_uname_buf.sysname, host_uname_buf.nodename,
            host_uname_buf.release, host_uname_buf.version, host_uname_buf.machine);
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        INFO("* Total Ram : %llu kB, Free: %llu kB\n",
            sys_info.totalram * (unsigned long long)sys_info.mem_unit / DIV,
            sys_info.freeram * (unsigned long long)sys_info.mem_unit / DIV);
    }

    /* pci device description */
    INFO("* Pci devices :\n");
    strcat(lscmd, logpath);
    int i = system(lscmd);
    INFO("system function command : %s, system function returned value : %d\n", lscmd, i);
#endif
}

void prepare_maru(void)
{
    INFO("Prepare maru specified feature\n");

    sdb_setup();

    INFO("call construct_main_window\n");

    construct_main_window(skin_argc, skin_argv, qemu_argc, qemu_argv );

    int guest_server_port = tizen_base_port + SDB_UDP_SENSOR_INDEX;
    start_guest_server( guest_server_port );

    mloop_ev_init();
}

int qemu_main(int argc, char** argv, char** envp);

int main(int argc, char* argv[])
{
    parse_options(argc, argv, &skin_argc, &skin_argv, &qemu_argc, &qemu_argv);
    get_bin_dir( qemu_argv[0] );
    socket_init();
    extract_info(qemu_argc, qemu_argv);

    INFO("Emulator start !!!\n");
    system_info();

    INFO("Prepare running...\n");
    redir_output(); // Redirect stdout, stderr after debug_ch is initialized...

    int i;

    fprintf(stdout, "qemu args : ==========================================\n");
    for(i = 0; i < qemu_argc; ++i)
    {
        fprintf(stdout, "%s ", qemu_argv[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "======================================================\n");

    fprintf(stdout, "skin args : ==========================================\n");
    for(i = 0; i < skin_argc; ++i)
    {
        fprintf(stdout, "%s ", skin_argv[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "======================================================\n");

    INFO("qemu main start!\n");
    qemu_main(qemu_argc, qemu_argv, NULL);

    exit_emulator();

    return 0;
}

