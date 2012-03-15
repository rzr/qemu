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
#include "option.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "mloop_event.h"

MULTI_DEBUG_CHANNEL(qemu, main);

#define IMAGE_PATH_PREFIX   "file="
#define IMAGE_PATH_SUFFIX   ",if=virtio"
#define SDB_PORT_PREFIX     "sdb_port="
#define MAXLEN  512
#define MIDBUF  128
int tizen_base_port = 0;

int _emulator_condition = 0; //TODO:
char tizen_target_path[MAXLEN] = {0, };
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
    mloop_ev_stop();
    shutdown_skin_server();
    shutdown_guest_server();
    SDL_Quit();
    remove_portfile();
}

static void construct_main_window(int skin_argc, char* skin_argv[])
{
    INFO("construct main window\n");
    start_skin_server(0, 0);
#if 1
    if ( 0 > start_skin_client(skin_argc, skin_argv) ) {
        exit( -1 );
    }
#endif

}

static void parse_options(int argc, char* argv[], char* proxy, char* dns1, char* dns2, int* skin_argc, char*** skin_argv, int* qemu_argc, char*** qemu_argv)
{
    int i;
    int j;
    char *point = NULL;
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
        if((point = strstr(argv[j], "console")) != NULL)
        {
            argv[j] = g_strdup_printf("%s proxy=%s dns1=%s dns2=%s", argv[j], proxy, dns1, dns2);
            break;
        }
    }
}

void get_image_path(char* qemu_argv)
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
    strcpy(tizen_target_path, path);
}

void get_tizen_port(char* option)
{
    int i;
    int j = 0;
    int max_len = 0;
    int prefix_len = 0;
    char *ptr;
    char *path = malloc(MAXLEN);
    prefix_len = strlen(SDB_PORT_PREFIX);;
    max_len = prefix_len + 5;
    for(i = prefix_len , j = 0; i < max_len; i++)
    {
        path[j++] = option[i];
    }
    path[j] = '\0';
    tizen_base_port = strtol(path, &ptr, 0);
    INFO( "tizen_base_port: %d\n", tizen_base_port);
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
    int skin_argc = 0;
    char** skin_argv = NULL;

    int qemu_argc = 0;
    char** qemu_argv = NULL;
    char proxy[MIDBUF] ={0}, dns1[MIDBUF] = {0}, dns2[MIDBUF] = {0};
	
    gethostproxy(proxy);
	gethostDNS(dns1, dns2);

    parse_options(argc, argv, proxy, dns1, dns2, &skin_argc, &skin_argv, &qemu_argc, &qemu_argv);

    int i;

/*
    printf("%d\n", skin_argc);
    for(i = 0; i < skin_argc; ++i)
    {
        printf("%s\n", skin_argv[i]);
    }
*/

//  printf("%d\n", qemu_argc);
    char *option = NULL;

    INFO("qemu args : =====================================\n");
    for(i = 0; i < qemu_argc; ++i)
    {
        INFO("%s ", qemu_argv[i]);
        if(strstr(qemu_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            get_image_path(qemu_argv[i]);
        }
        if((option = strstr(qemu_argv[i], SDB_PORT_PREFIX)) != NULL) {
            get_tizen_port(option);
            write_portfile(tizen_target_path);
        }

    }

    INFO("\n");
    INFO("======================================================\n");

    INFO("skin args : =====================================\n");
    for(i = 0; i < skin_argc; ++i)
    {
        INFO("%s ", skin_argv[i]);
        if(strstr(skin_argv[i], IMAGE_PATH_PREFIX) != NULL) {
            get_image_path(skin_argv[i]);
        }
        if((option = strstr(skin_argv[i], SDB_PORT_PREFIX)) != NULL) {
            get_tizen_port(option);
        }

    }
    INFO("\n");
    INFO("======================================================\n");

    sdb_setup();

    INFO("call construct_main_window\n");

    construct_main_window(skin_argc, skin_argv);

    //TODO get port number by args from emulator manager
    int guest_server_port = tizen_base_port + SDB_UDP_SENSOR_INDEX;
    start_guest_server( guest_server_port );

    mloop_ev_init();

    INFO("qemu main start!\n");
    qemu_main(qemu_argc, qemu_argv, NULL);

    exit_emulator();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

