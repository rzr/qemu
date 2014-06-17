/*
 * Emulator
 *
 * Copyright (C) 2011 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <stdlib.h>

#include "qemu/config-file.h"
#include "qemu/sockets.h"

#include "build_info.h"
#include "emulator.h"
#include "emul_state.h"
#include "guest_debug.h"
#include "guest_server.h"
#include "hw/maru_virtio_touchscreen.h"
#include "check_gl.h"
#include "maru_err_table.h"
#include "maru_display.h"
#include "mloop_event.h"
#include "osutil.h"
#include "sdb.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"
#include "ecs/ecs.h"

#ifdef CONFIG_SDL
#include <SDL.h>
#endif

MULTI_DEBUG_CHANNEL(qemu, main);

#define QEMU_ARGS_PREFIX "--qemu-args"
#define SKIN_ARGS_PREFIX "--skin-args"
#define IMAGE_PATH_PREFIX   "file="
//#define IMAGE_PATH_SUFFIX   ",if=virtio"
#define IMAGE_PATH_SUFFIX   ",if=virtio,index=1"
#define SDB_PORT_PREFIX     "sdb_port="
#define LOGS_SUFFIX         "/logs/"
#define LOGFILE             "emulator.log"
#define DISPLAY_WIDTH_PREFIX "width="
#define DISPLAY_HEIGHT_PREFIX "height="
#define INPUT_TOUCH_PARAMETER "virtio-touchscreen-pci"

#define MIDBUF  128
#define LEN_MARU_KERNEL_CMDLINE 512

// for compatibility
gchar log_path[PATH_MAX] = { 0, };

extern gchar maru_kernel_cmdline[LEN_MARU_KERNEL_CMDLINE];

extern char tizen_target_path[PATH_MAX];
extern char tizen_target_img_path[PATH_MAX];

extern int enable_yagl;
extern int enable_spice;

extern int _skin_argc;
extern char **_skin_argv;
extern int _qemu_argc;
extern char **_qemu_argv;

static void set_bin_path(gchar * exec_argv)
{
    set_bin_path_os(exec_argv);
}

static void print_system_info(void)
{
#define DIV 1024

    INFO("* Board name : %s\n", build_version);
    INFO("* Package %s\n", pkginfo_version);
    INFO("* Package %s\n", pkginfo_maintainer);
    INFO("* Git Head : %s\n", pkginfo_githead);
    INFO("* %s\n", latest_gittag);
    INFO("* User name : %s\n", g_get_real_name());
    INFO("* Host name : %s\n", g_get_host_name());

    /* time stamp */
    INFO("* Build date : %s\n", build_date);

    qemu_timeval tval = { 0, };
    if (qemu_gettimeofday(&tval) == 0) {
        char timeinfo[64] = {0, };

        time_t ti = tval.tv_sec;
        struct tm *tm_time = localtime(&ti);
        strftime(timeinfo, sizeof(timeinfo), "%Y-%m-%d %H:%M:%S", tm_time);
        INFO("* Current time : %s\n", timeinfo);
    }

#ifdef CONFIG_SDL
    /* Gets the version of the dynamically linked SDL library */
    INFO("* Host sdl version : (%d, %d, %d)\n",
            SDL_Linked_Version()->major,
            SDL_Linked_Version()->minor,
            SDL_Linked_Version()->patch);
#endif

    print_system_info_os();
}

int qemu_main(int argc, char **argv, char **envp);

static void set_image_and_log_path(char *qemu_argv)
{
    int i, j = 0;
    int name_len = 0;
    int prefix_len = 0;
    int suffix_len = 0;
    int max = 0;
    char *path = malloc(PATH_MAX);
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

    set_emul_vm_name(g_path_get_basename(tizen_target_path));
    strcpy(tizen_target_img_path, path);
    free(path);

    strcpy(log_path, tizen_target_path);
    strcat(log_path, LOGS_SUFFIX);
#ifdef CONFIG_WIN32
    if (access(g_win32_locale_filename_from_utf8(log_path), R_OK) != 0) {
        g_mkdir(g_win32_locale_filename_from_utf8(log_path), 0755);
    }
#else
    if (access(log_path, R_OK) != 0) {
        if (g_mkdir(log_path, 0755) < 0) {
            fprintf(stderr, "failed to create log directory %s\n", log_path);
        }
    }
#endif
    strcat(log_path, LOGFILE);
}

static void remove_vm_lock(void)
{
    remove_vm_lock_os();
}

static void emulator_notify_exit(Notifier *notifier, void *data)
{
    remove_vm_lock();

    INFO("Exit emulator...\n");
}

static Notifier emulator_exit = { .notify = emulator_notify_exit };

static void redir_output(void)
{
    FILE *fp;

    fp = freopen(log_path, "a+", stdout);
    if (fp == NULL) {
        fprintf(stderr, "log file open error\n");
    }

    fp = freopen(log_path, "a+", stderr);
    if (fp == NULL) {
        fprintf(stderr, "log file open error\n");
    }
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
}

// deprecated
static void extract_qemu_info(int qemu_argc, char **qemu_argv)
{
    int i = 0;

    for (i = 0; i < qemu_argc; ++i) {
        if (strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            set_image_and_log_path(qemu_argv[i]);
        } else if (strstr(qemu_argv[i], INPUT_TOUCH_PARAMETER) != NULL) {
            /* touchscreen */
            set_emul_input_touch_enable(true);

            char *option = strstr(qemu_argv[i] + strlen(INPUT_TOUCH_PARAMETER), TOUCHSCREEN_OPTION_NAME);
            if (option != NULL) {
                option += strlen(TOUCHSCREEN_OPTION_NAME) + 1;

                set_emul_max_touch_point(atoi(option));
            }
        }
    }

    if (is_emul_input_touch_enable() != true) {
        set_emul_input_mouse_enable(true);
    }
}

// deprecated
static void extract_skin_info(int skin_argc, char **skin_argv)
{
    int i = 0;
    int w = 0, h = 0;

    for (i = 0; i < skin_argc; ++i) {
        if (strstr(skin_argv[i], DISPLAY_WIDTH_PREFIX) != NULL) {
            char *width_arg = skin_argv[i] + strlen(DISPLAY_WIDTH_PREFIX);
            w = atoi(width_arg);

            INFO("display width option : %d\n", w);
        } else if (strstr(skin_argv[i], DISPLAY_HEIGHT_PREFIX) != NULL) {
            char *height_arg = skin_argv[i] + strlen(DISPLAY_HEIGHT_PREFIX);
            h = atoi(height_arg);

            INFO("display height option : %d\n", h);
        }

        if (w != 0 && h != 0) {
            set_emul_resolution(w, h);
            break;
        }
    }
}

// deprecated
static void legacy_parse_options(int argc, char *argv[], int *skin_argc,
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

// deprecated
int legacy_emulator_main(int argc, char **argv, char **envp);

int legacy_emulator_main(int argc, char * argv[], char **envp)
{
    legacy_parse_options(argc, argv, &_skin_argc,
                &_skin_argv, &_qemu_argc, &_qemu_argv);
    set_bin_path(_qemu_argv[0]);
    extract_qemu_info(_qemu_argc, _qemu_argv);

    INFO("Emulator start !!!\n");
    atexit(maru_atexit);
    emulator_add_exit_notifier(&emulator_exit);

    extract_skin_info(_skin_argc, _skin_argv);

    /* Redirect stdout and stderr after debug_ch is initialized. */
    redir_output();

    print_system_info();

    INFO("Prepare running...\n");
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

    INFO("socket initialize\n");
    socket_init();

    INFO("qemu main start!\n");
    qemu_main(_qemu_argc, _qemu_argv, envp);

    return 0;
}
