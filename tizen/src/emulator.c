/*
 * Emulator
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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
#include <string.h>

#include "qemu/config-file.h"
#include "qemu/sockets.h"

#include "build_info.h"
#include "emulator.h"
#include "emul_state.h"
#include "guest_debug.h"
#include "guest_server.h"
#include "hw/maru_camera_common.h"
#include "hw/gloffscreen_test.h"
#include "maru_common.h"
#include "maru_err_table.h"
#include "maru_display.h"
#include "mloop_event.h"
#include "osutil.h"
#include "sdb.h"
#include "skin/maruskin_server.h"
#include "skin/maruskin_client.h"
#include "debug_ch.h"
#include "ecs.h"

#ifdef CONFIG_SDL
#include <SDL.h>
#endif
#ifdef CONFIG_LINUX
#include <sys/ipc.h>
#include <sys/shm.h>
extern int g_shmid;
#endif

#ifdef CONFIG_DARWIN
#include "ns_event.h"
int thread_running = 1; /* Check if we need exit main */
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
gchar maru_kernel_cmdline[LEN_MARU_KERNEL_CMDLINE];

gchar bin_path[PATH_MAX] = { 0, };
gchar log_path[PATH_MAX] = { 0, };

int tizen_base_port;
int tizen_ecs_port;
char tizen_target_path[PATH_MAX];
char tizen_target_img_path[PATH_MAX];

int enable_gl = 0;
int enable_yagl = 0;
int is_webcam_enabled;

static int _skin_argc;
static char **_skin_argv;
static int _qemu_argc;
static char **_qemu_argv;

const gchar *get_log_path(void)
{
    return log_path;
}

void exit_emulator(void)
{
    INFO("exit emulator!\n");

    mloop_ev_stop();
    shutdown_skin_server();
    shutdown_guest_server();
	stop_ecs();

#ifdef CONFIG_LINUX
    /* clean up the vm lock memory by munkyu */
    if (shmctl(g_shmid, IPC_RMID, 0) == -1) {
        ERR("shmctl failed\n");
        perror("emulator.c: ");
    }
#endif

    maru_display_fini();
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

static void get_host_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    get_host_proxy_os(http_proxy, https_proxy, ftp_proxy, socks_proxy);
}

static void set_bin_path(gchar * exec_argv)
{
    set_bin_path_os(exec_argv);
}

gchar * get_bin_path(void)
{
    return bin_path;
}

static void check_vm_lock(void)
{
    check_vm_lock_os();
}

static void make_vm_lock(void)
{
    make_vm_lock_os();
}

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
        g_mkdir(log_path, 0755);
    }
#endif
    strcat(log_path, LOGFILE);
}

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

static void extract_qemu_info(int qemu_argc, char **qemu_argv)
{
    int i = 0;

    for (i = 0; i < qemu_argc; ++i) {
        if (strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            set_image_and_log_path(qemu_argv[i]);
        } else if (strstr(qemu_argv[i], INPUT_TOUCH_PARAMETER) != NULL) {
            set_emul_input_touch_enable(true);
        }
    }
}

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
            set_emul_lcd_size(w, h);
            break;
        }
    }
}


static void print_system_info(void)
{
#define DIV 1024

    char timeinfo[64] = {0, };
    struct tm *tm_time;
    struct timeval tval;

    INFO("* Board name : %s\n", build_version);
    INFO("* Package %s\n", pkginfo_version);
    INFO("* Package %s\n", pkginfo_maintainer);
    INFO("* Git Head : %s\n", pkginfo_githead);
    INFO("* %s\n", latest_gittag);
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

    print_system_info_os();
}

typedef struct {
    const char *device_name;
    int found;
} device_opt_finding_t;

static int find_device_opt (QemuOpts *opts, void *opaque)
{
    device_opt_finding_t *devp = (device_opt_finding_t *) opaque;
    if (devp->found == 1) {
        return 0;
    }

    const char *str = qemu_opt_get (opts, "driver");
    if (strcmp (str, devp->device_name) == 0) {
        devp->found = 1;
    }
    return 0;
}

#define DEFAULT_QEMU_DNS_IP "10.0.2.3"
static void prepare_basic_features(void)
{
    char http_proxy[MIDBUF] ={0}, https_proxy[MIDBUF] = {0,},
        ftp_proxy[MIDBUF] = {0,}, socks_proxy[MIDBUF] = {0,},
        dns[MIDBUF] = {0};

    tizen_base_port = get_sdb_base_port();

	tizen_ecs_port = get_ecs_port();

    qemu_add_opts(&qemu_ecs_opts);

	start_ecs(tizen_ecs_port);

    get_host_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
    /* using "DNS" provided by default QEMU */
    g_strlcpy(dns, DEFAULT_QEMU_DNS_IP, strlen(DEFAULT_QEMU_DNS_IP) + 1);

    check_vm_lock();
    make_vm_lock();

    sdb_setup(); /* determine the base port for emulator */
    set_emul_vm_base_port(tizen_base_port);

    set_emul_vm_ecs_port(tizen_ecs_port);

    gchar * const tmp_str = g_strdup_printf(" sdb_port=%d,"
        " http_proxy=%s https_proxy=%s ftp_proxy=%s socks_proxy=%s"
        " dns1=%s", get_emul_vm_base_port(),
        http_proxy, https_proxy, ftp_proxy, socks_proxy, dns);

    g_strlcat(maru_kernel_cmdline, tmp_str, LEN_MARU_KERNEL_CMDLINE);

    g_free(tmp_str);
}

#define VIRTIOGL_DEV_NAME "virtio-gl-pci"
#ifdef CONFIG_GL_BACKEND
static void prepare_opengl_acceleration(void)
{
    int capability_check_gl = 0;

    if (enable_gl && enable_yagl) {
        ERR("Error: only one openGL passthrough device can be used at one time!\n");
        exit(1);
    }


    if (enable_gl || enable_yagl) {
        capability_check_gl = gl_acceleration_capability_check();

        if (capability_check_gl != 0) {
            enable_gl = enable_yagl = 0;
            WARN("Warn: GL acceleration was disabled due to the fail of GL check!\n");
        }
    }
    if (enable_gl) {
        device_opt_finding_t devp = {VIRTIOGL_DEV_NAME, 0};
        qemu_opts_foreach(qemu_find_opts("device"), find_device_opt, &devp, 0);
        if (devp.found == 0) {
            if (!qemu_opts_parse(qemu_find_opts("device"), VIRTIOGL_DEV_NAME, 1)) {
                exit(1);
            }
        }
    }

    gchar * const tmp_str = g_strdup_printf(" gles=%d yagl=%d", (enable_gl || enable_yagl), enable_yagl);

    g_strlcat(maru_kernel_cmdline, tmp_str, LEN_MARU_KERNEL_CMDLINE);

    g_free(tmp_str);
}
#endif

#define MARUCAM_DEV_NAME "maru_camera_pci"
#define WEBCAM_INFO_IGNORE 0x00
#define WEBCAM_INFO_WRITE 0x04
static void prepare_host_webcam(void)
{
    is_webcam_enabled = marucam_device_check(WEBCAM_INFO_WRITE);

    if (!is_webcam_enabled) {
        INFO("[Webcam] <WARNING> Webcam support was disabled "
                         "due to the fail of webcam capability check!\n");
    }
    else {
        device_opt_finding_t devp = {MARUCAM_DEV_NAME, 0};
        qemu_opts_foreach(qemu_find_opts("device"), find_device_opt, &devp, 0);
        if (devp.found == 0) {
            if (!qemu_opts_parse(qemu_find_opts("device"), MARUCAM_DEV_NAME, 1)) {
                INFO("Failed to initialize the marucam device.\n");
                exit(1);
            }
        }
        INFO("[Webcam] Webcam support was enabled.\n");
    }

    gchar * const tmp_str = g_strdup_printf(" enable_cam=%d", is_webcam_enabled);

    g_strlcat(maru_kernel_cmdline, tmp_str, LEN_MARU_KERNEL_CMDLINE);

    g_free(tmp_str);
}

const gchar * prepare_maru_devices(const gchar *kernel_cmdline)
{
    INFO("Prepare maru specified kernel command line\n");

    g_strlcpy(maru_kernel_cmdline, kernel_cmdline, LEN_MARU_KERNEL_CMDLINE);

    // Prepare basic features
    prepare_basic_features();

    // Prepare GL acceleration
#ifdef CONFIG_GL_BACKEND
    prepare_opengl_acceleration();
#endif

    // Prepare host webcam
    prepare_host_webcam();

    INFO("kernel command : %s\n", maru_kernel_cmdline);

    return maru_kernel_cmdline;
}

int maru_device_check(QemuOpts *opts)
{
#if defined(CONFIG_GL_BACKEND)
    // virtio-gl pci device
    if (!enable_gl) {
        // ignore virtio-gl-pci device, even if users set it in option.
        const char *driver = qemu_opt_get(opts, "driver");
        if (driver && (strcmp (driver, VIRTIOGL_DEV_NAME) == 0)) {
            return -1;
        }
    }
#endif
    if (!is_webcam_enabled) {
        const char *driver = qemu_opt_get(opts, "driver");
        if (driver && (strcmp (driver, MARUCAM_DEV_NAME) == 0)) {
            return -1;
        }
    }

    return 0;
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
    set_bin_path(_qemu_argv[0]);
    extract_qemu_info(_qemu_argc, _qemu_argv);

    INFO("Emulator start !!!\n");
    atexit(maru_atexit);

    extract_skin_info(_skin_argc, _skin_argv);

    print_system_info();

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

    INFO("socket initialize\n");
    socket_init();

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

