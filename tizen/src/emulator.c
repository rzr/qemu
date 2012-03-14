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
#include "process.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

MULTI_DEBUG_CHANNEL(qemu, main);

#define IMAGE_PATH_PREFIX "file="
#define IMAGE_PATH_SUFFIX ",if=virtio"
#define MAXPATH  512

int tizen_base_port = 0;

int _emulator_condition = 0; //TODO:
extern char tizen_vms_path[256];

int get_emulator_condition(void)
{
    return _emulator_condition;
}

void set_emulator_condition(int state)
{
    _emulator_condition = state;
}

void exit_emulator(void)
{
    shutdown_skin_server();
    shutdown_guest_server();
    SDL_Quit();
    remove_portfile();
}

static void construct_main_window(int skin_argc, char* skin_argv[])
{
    INFO("construct main window\n");
    start_skin_server(11111, 0, 0);
#if 1
    if ( 0 > start_skin_client(skin_argc, skin_argv) ) {
        exit( -1 );
    }
#endif

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

            break;
        }
    }
}

void get_image_path(int qemu_argc, char* qemu_argv)
{
    int i;
    int j = 0;
    int name_len = 0;
    int prefix_len = 0;
    int suffix_len = 0;
    int max = 0;
    char *path = malloc(MAXPATH);
    name_len = strlen(qemu_argv);
    prefix_len = strlen(IMAGE_PATH_PREFIX);
    suffix_len = strlen(IMAGE_PATH_SUFFIX);
    max = name_len - suffix_len;
    for(i = prefix_len , j = 0; i < max; i++)
    {
        path[j++] = qemu_argv[i];
    }
    path[j] = '\0';

    write_portfile(path);
}

int qemu_main(int argc, char** argv, char** envp);

int main(int argc, char* argv[])
{
#ifdef _WIN32
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,0), &wsadata) == SOCKET_ERROR) {
        ERR("Error creating socket.\n");
        return NULL;
    }
#endif

    tizen_base_port = get_sdb_base_port();

    int skin_argc = 0;
    char** skin_argv = NULL;

    int qemu_argc = 0;
    char** qemu_argv = NULL;

    parse_options(argc, argv, &skin_argc, &skin_argv, &qemu_argc, &qemu_argv);

    int i;

/*
    printf("%d\n", skin_argc);
    for(i = 0; i < skin_argc; ++i)
    {
        printf("%s\n", skin_argv[i]);
    }
*/

//  printf("%d\n", qemu_argc);
    INFO("Start emulator : =====================================\n");
    for(i = 0; i < qemu_argc; ++i)
    {
        INFO("%s ", qemu_argv[i]);
        if(strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            get_image_path(qemu_argc, qemu_argv[i]);
        }
    }
    INFO("\n");
    INFO("======================================================\n");

    sdb_setup();

    construct_main_window(skin_argc, skin_argv);

    //TODO get port number by args from emulator manager
    int guest_server_port = get_sdb_base_port() + SDB_UDP_SENSOR_INDEX;
    start_guest_server( guest_server_port );

    INFO("qemu main start!\n");
    qemu_main(qemu_argc, qemu_argv, NULL);

    exit_emulator();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

