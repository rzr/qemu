/*
 * Emulator
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
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
#include <getopt.h>
#include <libgen.h>

#include "qemu/config-file.h"
#include "qemu/sockets.h"

#include "build_info.h"
#include "emulator.h"
#include "emul_state.h"
#include "emulator_options.h"
#include "util/check_gl.h"
#include "util/maru_err_table.h"
#include "util/osutil.h"
#include "util/sdb.h"
#include "skin/maruskin_server.h"
#include "util/new_debug_ch.h"
#include "ecs/ecs.h"
#include "util/maru_device_hotplug.h"

#ifdef CONFIG_SDL
#include <SDL.h>
#endif

#ifdef CONFIG_DARWIN
#include "ns_event.h"
int thread_running = 1; /* Check if we need exit main */
#endif

DECLARE_DEBUG_CHANNEL(main);

#define ARGS_LIMIT  128
#define LEN_MARU_KERNEL_CMDLINE 512
char maru_kernel_cmdline[LEN_MARU_KERNEL_CMDLINE];

char tizen_target_path[PATH_MAX];
char tizen_target_img_path[PATH_MAX];

int enable_yagl = 0;
int enable_spice = 0;

int _skin_argc;
char **_skin_argv;
int _qemu_argc;
char **_qemu_argv;

void emulator_add_exit_notifier(Notifier *notify)
{
    qemu_add_exit_notifier(notify);
}

#if defined(CONFIG_SDL) || defined(CONFIG_USE_SHM)
static void construct_main_window(int skin_argc, char *skin_argv[],
                                int qemu_argc, char *qemu_argv[])
{
    LOG_INFO("construct main window\n");

    start_skin_server(skin_argc, skin_argv, qemu_argc, qemu_argv);

    set_emul_caps_lock_state(0);
    set_emul_num_lock_state(0);

    /* the next line checks for debugging and etc.. */
    if (get_emul_skin_enable()) {
        if (0 > start_skin_client(skin_argc, skin_argv)) {
            maru_register_exit_msg(MARU_EXIT_SKIN_SERVER_FAILED, NULL);
            exit(-1);
        }
    }
}
#endif

static void get_host_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    get_host_proxy_os(http_proxy, https_proxy, ftp_proxy, socks_proxy);
}

static void set_bin_path(char const *const exec_argv)
{
    set_bin_path_os(exec_argv);
}

static void check_vm_lock(void)
{
    check_vm_lock_os();
}

static void make_vm_lock(void)
{
    make_vm_lock_os();
}

static void remove_vm_lock(void)
{
    remove_vm_lock_os();
}

static void emulator_notify_exit(Notifier *notifier, void *data)
{
    remove_vm_lock();

    int i;
    for (i = 0; i < _qemu_argc; ++i) {
        g_free(_qemu_argv[i]);
    }
    for (i = 0; i < _skin_argc; ++i) {
        g_free(_skin_argv[i]);
    }
    reset_variables();

    LOG_INFO("Exit emulator...\n");
}

static Notifier emulator_exit = { .notify = emulator_notify_exit };

static void print_system_info(void)
{
    LOG_INFO("* Board name : %s\n", build_version);
    LOG_INFO("* Package %s\n", pkginfo_version);
    LOG_INFO("* Package %s\n", pkginfo_maintainer);
    LOG_INFO("* Git Head : %s\n", pkginfo_githead);
    LOG_INFO("* %s\n", latest_gittag);
    LOG_INFO("* User name : %s\n", g_get_real_name());
    LOG_INFO("* Host name : %s\n", g_get_host_name());

    /* time stamp */
    LOG_INFO("* Build date : %s\n", build_date);

    qemu_timeval tval = { 0, };
    if (qemu_gettimeofday(&tval) == 0) {
        char timeinfo[64] = {0, };

        time_t ti = tval.tv_sec;
        struct tm *tm_time = localtime(&ti);
        strftime(timeinfo, sizeof(timeinfo), "%Y-%m-%d %H:%M:%S", tm_time);
        LOG_INFO("* Current time : %s\n", timeinfo);
    }

#ifdef CONFIG_SDL
    /* Gets the version of the dynamically linked SDL library */
    LOG_INFO("* Host sdl version : (%d, %d, %d)\n",
            SDL_Linked_Version()->major,
            SDL_Linked_Version()->minor,
            SDL_Linked_Version()->patch);
#endif

    print_system_info_os();
}

static void print_options_info(void)
{
    int i;

    fprintf(stdout, "qemu args: =========================================\n");
    for (i = 0; i < _qemu_argc; ++i) {
        fprintf(stdout, "%s ", _qemu_argv[i]);
    }
    fprintf(stdout, "\n====================================================\n");

    fprintf(stdout, "skin args: =========================================\n");
    for (i = 0; i < _skin_argc; ++i) {
        fprintf(stdout, "%s ", _skin_argv[i]);
    }
    fprintf(stdout, "\n====================================================\n");
}

#define PROXY_BUFFER_LEN  128
#define DEFAULT_QEMU_DNS_IP "10.0.2.3"
static void prepare_basic_features(gchar * const kernel_cmdline)
{
    char http_proxy[PROXY_BUFFER_LEN] ={ 0, },
        https_proxy[PROXY_BUFFER_LEN] = { 0, },
        ftp_proxy[PROXY_BUFFER_LEN] = { 0, },
        socks_proxy[PROXY_BUFFER_LEN] = { 0, },
        dns[PROXY_BUFFER_LEN] = { 0, };

    set_base_port();

#if defined(CONFIG_SPICE) && defined(CONFIG_LINUX)
    clean_websocket_port(SIGKILL);
#endif

    check_vm_lock();
    make_vm_lock();

    maru_device_hotplug_init();

    qemu_add_opts(&qemu_ecs_opts);
    start_ecs();

    start_sdb_noti_server(get_device_serial_number() + SDB_UDP_SENSOR_INDEX);

    sdb_setup();

    get_host_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
    /* using "DNS" provided by default QEMU */
    g_strlcpy(dns, DEFAULT_QEMU_DNS_IP, strlen(DEFAULT_QEMU_DNS_IP) + 1);

    gchar * const tmp_str = g_strdup_printf(" sdb_port=%d,"
        " http_proxy=%s https_proxy=%s ftp_proxy=%s socks_proxy=%s"
        " dns1=%s vm_resolution=%dx%d", get_emul_vm_base_port(),
        http_proxy, https_proxy, ftp_proxy, socks_proxy, dns,
        get_emul_resolution_width(), get_emul_resolution_height());

    g_strlcat(kernel_cmdline, tmp_str, LEN_MARU_KERNEL_CMDLINE);

    g_free(tmp_str);
}

#ifdef CONFIG_YAGL
static void prepare_opengl_acceleration(gchar * const kernel_cmdline)
{
    int capability_check_gl = 0;

    if (enable_yagl) {
        capability_check_gl = check_gl();

        if (capability_check_gl != 0) {
            enable_yagl = 0;
            LOG_INFO("<WARNING> GL acceleration was disabled due to the fail of GL check!\n");
        }
    }

    gchar * const tmp_str = g_strdup_printf(" yagl=%d", enable_yagl);

    g_strlcat(kernel_cmdline, tmp_str, LEN_MARU_KERNEL_CMDLINE);

    g_free(tmp_str);
}
#endif

const char *prepare_maru(const gchar * const kernel_cmdline)
{
    LOG_INFO("Prepare maru specified feature\n");

    g_strlcpy(maru_kernel_cmdline, kernel_cmdline, LEN_MARU_KERNEL_CMDLINE);

    /* Prepare basic features */
    LOG_INFO("Prepare_basic_features\n");
    prepare_basic_features(maru_kernel_cmdline);

    /* Prepare GL acceleration */
#ifdef CONFIG_YAGL
    LOG_INFO("Prepare_opengl_acceleration\n");
    prepare_opengl_acceleration(maru_kernel_cmdline);
#endif

    LOG_INFO("kernel command : %s\n", maru_kernel_cmdline);

    return maru_kernel_cmdline;
}

#if defined(CONFIG_SDL) || defined(CONFIG_USE_SHM)
void start_skin(void)
{
    LOG_INFO("Start skin\n");

    construct_main_window(_skin_argc, _skin_argv, _qemu_argc, _qemu_argv);
}
#endif

int qemu_main(int argc, char **argv, char **envp);
int legacy_emulator_main(int argc, char **argv, char **envp);

static int emulator_main(int argc, char *argv[], char **envp)
{
#ifdef SUPPORT_LEGACY_ARGS
    // for compatibilities...
    if (argc > 2 && !g_strcmp0("--skin-args", argv[1])) {
        return legacy_emulator_main(argc, argv, envp);
    }
#endif

#ifdef CONFIG_WIN32
    // SDL_init() routes stdout and stderr to the respective files in win32.
    // So we revert it.
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
#endif

    gchar *profile = NULL;
    gchar *conf = NULL;

    int c = 0;

    _qemu_argv = g_malloc(sizeof(char*) * ARGS_LIMIT);
    _skin_argv = g_malloc(sizeof(char*) * ARGS_LIMIT);

    // parse arguments
    // prevent the error message for undefined options
    opterr = 0;

    while (c != -1) {
        static struct option long_options[] = {
            {"conf",        required_argument,  0,  'c' },
            {"profile",     required_argument,  0,  'p' },
            {"additional",  required_argument,  0,  'a' },
            {0,             0,                  0,  0   }
        };

        c = getopt_long(argc, argv, "c:p:a:", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
        case '?':
            set_variable(argv[optind - 1], argv[optind], true);
            break;
        case 'c':
            set_variable("conf", optarg, true);
            conf = g_strdup(optarg);
            break;
        case 'p':
            set_variable("profile", optarg, true);
            profile = g_strdup(optarg);
            break;
        case 'a':
            // TODO: additional options should be accepted
            set_variable("additional", optarg, true);
            c = -1;
            break;
        default:
            break;
        }
    }

    if (!profile && !conf) {
        fprintf(stderr, "Usage: %s {-c|--conf} conf_file ...\n"
                        "       %s {-p|--profile} profile ...\n",
                        basename(argv[0]), basename(argv[0]));

        return -1;
    }

    // load profile configurations
    _qemu_argc = 0;
    _qemu_argv[_qemu_argc++] = g_strdup(argv[0]);

    set_bin_path(_qemu_argv[0]);

    if (!load_profile_default(conf, profile)) {
        return -1;
    }

    // set emulator resolution
    {
        char *resolution = get_variable("resolution");
        if (!resolution) {
            fprintf(stderr, "[resolution] is required.\n");
            return -1;
        }
        char **splitted = g_strsplit(resolution, "x", 2);
        if (!splitted[0] || !splitted[1]) {
            fprintf(stderr, "resolution value [%s] is weird. Please use format \"WIDTHxHEIGHT\"\n", resolution);
            g_strfreev(splitted);
            return -1;
        }
        else {
            set_emul_resolution(g_ascii_strtoull(splitted[0], NULL, 0),
                            g_ascii_strtoull(splitted[1], NULL, 0));
        }
        g_strfreev(splitted);
    }

    // assemble arguments for qemu and skin
    if (!assemble_profile_args(&_qemu_argc, _qemu_argv,
                        &_skin_argc, _skin_argv)) {
        return -1;
    }


    LOG_INFO("Start emulator...\n");
    atexit(maru_atexit);
    emulator_add_exit_notifier(&emulator_exit);

    print_system_info();

    print_options_info();

    LOG_INFO("socket initialize...\n");
    socket_init();

    LOG_INFO("qemu main start...\n");
    qemu_main(_qemu_argc, _qemu_argv, envp);

    return 0;
}

#ifdef CONFIG_DARWIN
int g_argc;

static void* main_thread(void* args)
{
    char** argv;
    int argc = g_argc;
    argv = (char**) args;

    emulator_main(argc, argv, NULL);

    thread_running = 0;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    char** args;
    QemuThread main_thread_id;
    g_argc = argc;
    args = argv;
    qemu_thread_create(&main_thread_id, "main_thread", main_thread, (void *)args, QEMU_THREAD_DETACHED);
    ns_event_loop(&thread_running);

    return 0;
}
#elif defined (CONFIG_LINUX)
int main(int argc, char *argv[], char **envp)
{
    maru_register_exception_handler();
    return emulator_main(argc, argv, envp);
}
#else // WIN32
int main(int argc, char *argv[])
{
    maru_register_exception_handler();
    return emulator_main(argc, argv, NULL);
}
#endif
