/*
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "maru_common.h"
#include <stdlib.h>
#ifdef CONFIG_SDL
#include <SDL.h>
#endif
#include "emulator.h"
#include "guest_debug.h"
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
#include "maru_err_table.h"
#include <glib.h>
#include <glib/gstdio.h>

#if defined(CONFIG_WIN32)
#include <windows.h>
#elif defined(CONFIG_LINUX)
#include <linux/version.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if defined(CONFIG_DARWIN)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "tizen/src/ns_event.h"
#endif

#include "mloop_event.h"

MULTI_DEBUG_CHANNEL(qemu, main);

#define QEMU_ARGS_PREFIX "--qemu-args"
#define SKIN_ARGS_PREFIX "--skin-args"
#define IMAGE_PATH_PREFIX   "file="
//#define IMAGE_PATH_SUFFIX   ",if=virtio"
#define IMAGE_PATH_SUFFIX   ",if=virtio,index=1"
#define SDB_PORT_PREFIX     "sdb_port="
#define LOGS_SUFFIX         "/logs/"
#define LOGFILE             "emulator.log"
#define LCD_WIDTH_PREFIX "width="
#define LCD_HEIGHT_PREFIX "height="
#define MIDBUF  128

int tizen_base_port;
char tizen_target_path[MAXLEN];
char tizen_target_img_path[MAXLEN];
char logpath[MAXLEN];

static int _skin_argc;
static char **_skin_argv;
static int _qemu_argc;
static char **_qemu_argv;

#ifdef CONFIG_DARWIN
int thread_running = 1; /* Check if we need exit main */
#endif

void maru_display_fini(void);

char *get_logpath(void)
{
    return logpath;
}

void exit_emulator(void)
{
    mloop_ev_stop();
    shutdown_skin_server();
    shutdown_guest_server();

    maru_display_fini();
}

void check_shdmem(void)
{
#if defined(CONFIG_LINUX)
    int shm_id;
    void *shm_addr;
    u_int port;
    int val;
    struct shmid_ds shm_info;

    for (port = 26100; port < 26200; port += 10) {
        shm_id = shmget((key_t)port, 0, 0);
        if (shm_id != -1) {
            shm_addr = shmat(shm_id, (void *)0, 0);
            if ((void *)-1 == shm_addr) {
                ERR("error occured at shmat()\n");
                break;
            }

            val = shmctl(shm_id, IPC_STAT, &shm_info);
            if (val != -1) {
                INFO("count of process that use shared memory : %d\n",
                    shm_info.shm_nattch);
                if ((shm_info.shm_nattch > 0) &&
                    strcmp(tizen_target_img_path, (char *)shm_addr) == 0) {
                    if (check_port_bind_listen(port + 1) > 0) {
                        shmdt(shm_addr);
                        continue;
                    }
                    shmdt(shm_addr);
                    maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                                        "Can not execute this VM.\n"
                                        "The same name is running now.");
                    exit(0);
                } else {
                    shmdt(shm_addr);
                }
            }
        }
    }

#elif defined(CONFIG_WIN32)
    u_int port;
    char *base_port = NULL;
    char *pBuf;
    HANDLE hMapFile;
    for (port = 26100; port < 26200; port += 10) {
        base_port = g_strdup_printf("%d", port);
        hMapFile = OpenFileMapping(FILE_MAP_READ, TRUE, base_port);
        if (hMapFile == NULL) {
            INFO("port %s is not used.\n", base_port);
            continue;
        } else {
             pBuf = (char *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 50);
            if (pBuf == NULL) {
                ERR("Could not map view of file (%d).\n", GetLastError());
                CloseHandle(hMapFile);
            }

            if (strcmp(pBuf, tizen_target_img_path) == 0) {
                maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                    "Can not execute this VM.\n"
                    "The same name is running now.");
                UnmapViewOfFile(pBuf);
                CloseHandle(hMapFile);
                free(base_port);
                exit(0);
            } else {
                UnmapViewOfFile(pBuf);
            }
        }

        CloseHandle(hMapFile);
        free(base_port);
    }
#elif defined(CONFIG_DARWIN)
    /* TODO: */
#endif
}

void make_shdmem(void)
{
#if defined(CONFIG_LINUX)
    int shmid;
    char *shared_memory;

    shmid = shmget((key_t)tizen_base_port, MAXLEN, 0666|IPC_CREAT);
    if (shmid == -1) {
        ERR("shmget failed\n");
        return;
    }

    shared_memory = shmat(shmid, (char *)0x00, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        return;
    }
    sprintf(shared_memory, "%s", tizen_target_img_path);
    INFO("shared memory key: %d value: %s\n",
        tizen_base_port, (char *)shared_memory);
#elif defined(CONFIG_WIN32)
    HANDLE hMapFile;
    char *pBuf;
    char *port_in_use;
    char *shared_memory;

    shared_memory = g_strdup_printf("%s", tizen_target_img_path);
    port_in_use =  g_strdup_printf("%d", tizen_base_port);
    hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE, /* use paging file */
                 NULL,                 /* default security */
                 PAGE_READWRITE,       /* read/write access */
                 0,                /* maximum object size (high-order DWORD) */
                 50,               /* maximum object size (low-order DWORD) */
                 port_in_use);         /* name of mapping object */
    if (hMapFile == NULL) {
        ERR("Could not create file mapping object (%d).\n", GetLastError());
        return;
    }
    pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 50);

    if (pBuf == NULL) {
        ERR("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        return;
    }

    CopyMemory((PVOID)pBuf, shared_memory, strlen(shared_memory));
    free(port_in_use);
    free(shared_memory);
#elif defined(CONFIG_DARWIN)
    /* TODO: */
    int shmid;
    char *shared_memory;

    shmid = shmget((key_t)SHMKEY, MAXLEN, 0666|IPC_CREAT);
    if (shmid == -1) {
        ERR("shmget failed\n");
        return;
    }
    shared_memory = shmat(shmid, (char *)0x00, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        return;
    }
    sprintf(shared_memory, "%d", get_sdb_base_port() + 2);
    INFO("shared memory key: %d, value: %s\n", SHMKEY, (char *)shared_memory);
    shmdt(shared_memory);
#endif
    return;
}

static void construct_main_window(int skin_argc, char *skin_argv[],
                                int qemu_argc, char *qemu_argv[])
{
    INFO("construct main window\n");

    start_skin_server(skin_argc, skin_argv, qemu_argc, qemu_argv);

    /* the next line checks for debugging and etc.. */
    if (get_emul_skin_enable() == 1) {
        if (0 > start_skin_client(skin_argc, skin_argv)) {
            maru_register_exit_msg(MARU_EXIT_SKIN_SERVER_FAILED, NULL);
            exit(-1);
        }
    }

    set_emul_caps_lock_state(0);
    set_emul_num_lock_state(0);
}

static void parse_options(int argc, char *argv[], int *skin_argc,
                        char ***skin_argv, int *qemu_argc, char ***qemu_argv)
{
    int i = 0;
    int skin_args_index = 0;

    if (argc <= 1) {
        fprintf(stderr, "Arguments are not enough to launch Emulator. "
                "Please try to use Emulator Manager.\n");
        exit(1);
    }

    /* classification */
    for (i = 1; i < argc; ++i) {
        if (strstr(argv[i], SKIN_ARGS_PREFIX)) {
            *skin_argv = &(argv[i + 1]);
            break;
        }
    }

    for (skin_args_index = i; skin_args_index < argc; ++skin_args_index) {
        if (strstr(argv[skin_args_index], QEMU_ARGS_PREFIX)) {
            *skin_argc = skin_args_index - i - 1;

            *qemu_argc = argc - skin_args_index - i + 1;
            *qemu_argv = &(argv[skin_args_index]);

            argv[skin_args_index] = argv[0];
        }
    }
}

static char *set_bin_dir(char *exec_argv)
{
#ifndef CONFIG_DARWIN
    char link_path[1024] = { 0, };
#endif
    char *file_name = NULL;

#if defined(CONFIG_WIN32)
    if (!GetModuleFileName(NULL, link_path, 1024)) {
        return NULL;
    }

    file_name = strrchr(link_path, '\\');
    strncpy(bin_dir, link_path, strlen(link_path) - strlen(file_name));

    strcat(bin_dir, "\\");

#elif defined(CONFIG_LINUX)
    ssize_t len = readlink("/proc/self/exe", link_path, sizeof(link_path) - 1);

    if (len < 0 || len > sizeof(link_path)) {
        perror("get_bin_dir error : ");
        return NULL;
    }

    link_path[len] = '\0';

    file_name = strrchr(link_path, '/');
    strncpy(bin_dir, link_path, strlen(link_path) - strlen(file_name));

    strcat(bin_dir, "/");

#else
    if (!exec_argv) {
        return NULL;
    }

    char *data = strdup(exec_argv);
    if (!data) {
        fprintf(stderr, "Fail to strdup for paring a binary directory.\n");
        return NULL;
    }

    file_name = strrchr(data, '/');
    if (!file_name) {
        free(data);
        return NULL;
    }

    strncpy(bin_dir, data, strlen(data) - strlen(file_name));

    strcat(bin_dir, "/");
    free(data);
#endif

    return bin_dir;
}

char *get_bin_path(void)
{
    return bin_dir;
}

void set_image_and_log_path(char *qemu_argv)
{
    int i, j = 0;
    int name_len = 0;
    int prefix_len = 0;
    int suffix_len = 0;
    int max = 0;
    char *path = malloc(MAXLEN);
    name_len = strlen(qemu_argv);
    prefix_len = strlen(IMAGE_PATH_PREFIX);
    suffix_len = strlen(IMAGE_PATH_SUFFIX);
    max = name_len - suffix_len;
    for (i = prefix_len , j = 0; i < max; i++) {
        path[j++] = qemu_argv[i];
    }
    path[j] = '\0';
    if (!g_path_is_absolute(path)) {
        strcpy(tizen_target_path, g_get_current_dir());
    } else {
        strcpy(tizen_target_path, g_path_get_dirname(path));
    }
    strcpy(tizen_target_img_path, path);
    free(path);
    strcpy(logpath, tizen_target_path);
    strcat(logpath, LOGS_SUFFIX);
#ifdef CONFIG_WIN32
    if (access(g_win32_locale_filename_from_utf8(logpath), R_OK) != 0) {
        g_mkdir(g_win32_locale_filename_from_utf8(logpath), 0755);
    }
#else
    if (access(logpath, R_OK) != 0) {
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
    if (fp == NULL) {
        fprintf(stderr, "log file open error\n");
    }

    fp = freopen(logpath, "a+", stderr);
    if (fp == NULL) {
        fprintf(stderr, "log file open error\n");
    }
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
}

void extract_qemu_info(int qemu_argc, char **qemu_argv)
{
    int i = 0;

    for (i = 0; i < qemu_argc; ++i) {
        if (strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            set_image_and_log_path(qemu_argv[i]);
            break;
        }
    }

}

void extract_skin_info(int skin_argc, char **skin_argv)
{
    int i = 0;
    int w = 0, h = 0;

    for (i = 0; i < skin_argc; ++i) {
        if (strstr(skin_argv[i], LCD_WIDTH_PREFIX) != NULL) {
            char *width_arg = skin_argv[i] + strlen(LCD_WIDTH_PREFIX);
            w = atoi(width_arg);

            INFO("lcd width option = %d\n", w);
        } else if (strstr(skin_argv[i], LCD_HEIGHT_PREFIX) != NULL) {
            char *height_arg = skin_argv[i] + strlen(LCD_HEIGHT_PREFIX);
            h = atoi(height_arg);

            INFO("lcd height option = %d\n", h);
        }

        if (w != 0 && h != 0) {
            set_emul_lcd_size(w, h);
            break;
        }
    }
}


static void system_info(void)
{
#define DIV 1024

    char timeinfo[64] = {0, };
    struct tm *tm_time;
    struct timeval tval;

    INFO("* SDK Version : %s\n", build_version);
    INFO("* Package %s\n", pkginfo_version);
    INFO("* Package %s\n", pkginfo_maintainer);
    INFO("* Git Head : %s\n", pkginfo_githead);
    INFO("* User name : %s\n", g_get_real_name());
    INFO("* Host name : %s\n", g_get_host_name());

    /* timestamp */
    INFO("* Build date : %s\n", build_date);
    gettimeofday(&tval, NULL);
    tm_time = localtime(&(tval.tv_sec));
    strftime(timeinfo, sizeof(timeinfo), "%Y/%m/%d %H:%M:%S", tm_time);
    INFO("* Current time : %s\n", timeinfo);

#ifdef CONFIG_SDL
    /* Gets the version of the dynamically linked SDL library */
    INFO("* Host sdl version : (%d, %d, %d)\n",
        SDL_Linked_Version()->major,
        SDL_Linked_Version()->minor,
        SDL_Linked_Version()->patch);
#endif

#if defined(CONFIG_WIN32)
    INFO("* Windows\n");

    /* Retrieves information about the current os */
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osvi)) {
        INFO("* MajorVersion : %d, MinorVersion : %d, BuildNumber : %d, \
            PlatformId : %d, CSDVersion : %s\n", osvi.dwMajorVersion,
            osvi.dwMinorVersion, osvi.dwBuildNumber,
            osvi.dwPlatformId, osvi.szCSDVersion);
    }

    /* Retrieves information about the current system */
    SYSTEM_INFO sysi;
    ZeroMemory(&sysi, sizeof(SYSTEM_INFO));

    GetSystemInfo(&sysi);
    INFO("* Processor type : %d, Number of processors : %d\n",
            sysi.dwProcessorType, sysi.dwNumberOfProcessors);

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    INFO("* Total Ram : %llu kB, Free: %lld kB\n",
            memInfo.ullTotalPhys / DIV, memInfo.ullAvailPhys / DIV);

#elif defined(CONFIG_LINUX)
    INFO("* Linux\n");

    /* depends on building */
    INFO("* QEMU build machine linux kernel version : (%d, %d, %d)\n",
            LINUX_VERSION_CODE >> 16,
            (LINUX_VERSION_CODE >> 8) & 0xff,
            LINUX_VERSION_CODE & 0xff);

     /* depends on launching */
    struct utsname host_uname_buf;
    if (uname(&host_uname_buf) == 0) {
        INFO("* Host machine uname : %s %s %s %s %s\n",
            host_uname_buf.sysname, host_uname_buf.nodename,
            host_uname_buf.release, host_uname_buf.version,
            host_uname_buf.machine);
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        INFO("* Total Ram : %llu kB, Free: %llu kB\n",
            sys_info.totalram * (unsigned long long)sys_info.mem_unit / DIV,
            sys_info.freeram * (unsigned long long)sys_info.mem_unit / DIV);
    }

    /* get linux distribution */
    INFO("* Linux distribution infomation :\n");
    char lsb_release_cmd[MAXLEN] = "lsb_release -d -r -c >> ";
    strcat(lsb_release_cmd, logpath);
    if(system(lsb_release_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lsb_release_cmd);
    }

    /* pci device description */
    INFO("* PCI devices :\n");
    char lspci_cmd[MAXLEN] = "lspci >> ";
    strcat(lspci_cmd, logpath);
    if(system(lspci_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lspci_cmd);
    }

#elif defined(CONFIG_DARWIN)
    INFO("* Mac\n");

    /* uname */
    INFO("* Host machine uname :\n");
    char uname_cmd[MAXLEN] = "uname -a >> ";
    strcat(uname_cmd, logpath);
    if(system(uname_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", uname_cmd);
    }

    /* hw information */
    int mib[2];
    size_t len;
    char *sys_info;
    int sys_num = 0;

    mib[0] = CTL_HW;
    mib[1] = HW_MODEL;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine model : %s\n", sys_info);
    }
    free(sys_info);

    mib[0] = CTL_HW;
    mib[1] = HW_MACHINE;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine class : %s\n", sys_info);
    }
    free(sys_info);

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Number of processors : %d\n", sys_num);
    }

    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Total memory : %llu bytes\n", sys_num);
    }

#endif

    INFO("\n");
}

void prepare_maru(void)
{
    INFO("Prepare maru specified feature\n");

    INFO("call construct_main_window\n");

    construct_main_window(_skin_argc, _skin_argv, _qemu_argc, _qemu_argv);

    int guest_server_port = tizen_base_port + SDB_UDP_SENSOR_INDEX;
    start_guest_server(guest_server_port);

    mloop_ev_init();
}

int qemu_main(int argc, char **argv, char **envp);

static int emulator_main(int argc, char *argv[])
{
    parse_options(argc, argv, &_skin_argc,
                &_skin_argv, &_qemu_argc, &_qemu_argv);
    set_bin_dir(_qemu_argv[0]);
    extract_qemu_info(_qemu_argc, _qemu_argv);

    INFO("Emulator start !!!\n");
    atexit(maru_atexit);

    extract_skin_info(_skin_argc, _skin_argv);

    system_info();

    INFO("Prepare running...\n");
    /* Redirect stdout and stderr after debug_ch is initialized. */
    redir_output();
    INFO("tizen_target_img_path: %s\n", tizen_target_img_path);
    int i;

    fprintf(stdout, "qemu args: =========================================\n");
    for (i = 0; i < _qemu_argc; ++i) {
        fprintf(stdout, "%s ", _qemu_argv[i]);
    }
    fprintf(stdout, "\nqemu args: =========================================\n");

    fprintf(stdout, "skin args: =========================================\n");
    for (i = 0; i < _skin_argc; ++i) {
        fprintf(stdout, "%s ", _skin_argv[i]);
    }
    fprintf(stdout, "\nskin args: =========================================\n");

    INFO("qemu main start!\n");
    qemu_main(_qemu_argc, _qemu_argv, NULL);

    exit_emulator();

    return 0;
}

#ifndef CONFIG_DARWIN
int main(int argc, char *argv[])
{
    return emulator_main(argc, argv);
}
#else
int g_argc;

static void* main_thread(void* args)
{
    char** argv;
    int argc = g_argc;
    argv = (char**) args;

    emulator_main(argc, argv);

    thread_running = 0;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    char** args;
    pthread_t main_thread_id;

    g_argc = argc;
    args = argv;

    if (0 != pthread_create(&main_thread_id, NULL, main_thread, args)) {
        INFO("Create main thread failed\n");
        return -1;
    }

    ns_event_loop(&thread_running);

    return 0;
}
#endif

