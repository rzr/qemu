/*
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JiHye Kim <jihye1128.kim@samsung.com>
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
#include <stdint.h>
#include <assert.h>
#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "guest_server.h"
#include "mloop_event.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"
#include "sdb.h"

MULTI_DEBUG_CHANNEL(qemu, guest_server);


#define RECV_BUF_SIZE 32

static void* run_guest_server(void* args);

static int svr_port = 0;
static int server_sock = 0;

static int parse_val(char *buff, unsigned char data, char *parsbuf);


pthread_t start_guest_server(int server_port)
{
    svr_port = server_port;

    pthread_t thread_id;

    if (0 != pthread_create(&thread_id, NULL, run_guest_server, NULL)) {
        ERR("fail to create guest_server pthread.\n");
    } else {
        INFO("created guest server thread\n");
    }

    return thread_id;

}

/* get_emulator_vms_sdcard_path = "/home/{USER}/tizen-sdk-data/emulator-vms/sdcard" */
char* get_emulator_vms_sdcard_path(void)
{
    char *emulator_vms_sdcard_path = NULL;

#ifndef _WIN32
    char emulator_vms[] = "/tizen-sdk-data/emulator-vms/sdcard/";
    char *homedir = (char*)g_getenv("HOME");

    if (!homedir) {
        homedir = (char*)g_get_home_dir();
    }

    emulator_vms_sdcard_path = malloc(strlen(homedir) + sizeof emulator_vms + 1);
    assert(emulator_vms_sdcard_path != NULL);
    strcpy(emulator_vms_sdcard_path, homedir);
    strcat(emulator_vms_sdcard_path, emulator_vms);
#else
    char emulator_vms[] = "\\tizen-sdk-data\\emulator-vms\\sdcard\\";
    HKEY hKey;
    char strLocalAppDataPath[1024] = { 0 };
    DWORD dwBufLen = 1024;
    RegOpenKeyEx(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        0, KEY_QUERY_VALUE, &hKey);

    RegQueryValueEx(hKey, "Local AppData", NULL, NULL, (LPBYTE)strLocalAppDataPath, &dwBufLen);
    RegCloseKey(hKey);

    emulator_vms_sdcard_path = malloc(strlen(strLocalAppDataPath) + sizeof emulator_vms + 1);
    strcpy(emulator_vms_sdcard_path, strLocalAppDataPath);
    strcat(emulator_vms_sdcard_path, emulator_vms);
#endif

    return emulator_vms_sdcard_path;
}

static void* run_guest_server(void* args)
{
    INFO("start guest server thread.\n");

    uint16_t port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    port = svr_port;

    if ((server_sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        ERR( "create listen socket error\n" );
        perror( "create listen socket error\n" );
        goto cleanup;
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    int opt = 1;
    setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );

    if ( 0 > bind( server_sock, (struct sockaddr*) &server_addr, sizeof( server_addr ) ) ) {
        ERR( "guest server bind error: " );
        perror( "bind" );
        goto cleanup;
    } else {
        INFO( "success to bind port[127.0.0.1:%d/udp] for guest_server in host \n", port );
    }

    client_len = sizeof( client_addr );

    char readbuf[RECV_BUF_SIZE];

    INFO( "guest server start...port:%d\n", port );

    while ( 1 ) {
        memset(&readbuf, 0, RECV_BUF_SIZE);

        if (server_sock == 0) {
            INFO("server_sock is closed\n");
            return NULL;
        }
        int read_cnt = recvfrom(server_sock, readbuf,
            RECV_BUF_SIZE, 0, (struct sockaddr*) &client_addr, &client_len);

        if (0 > read_cnt) {
            ERR("fail to recvfrom in guest_server.\n");
            perror("fail to recvfrom in guest_server.:");
            break;
        } else {

            if (0 == read_cnt) {
                ERR("read_cnt is 0.\n");
                break;
            }

            TRACE("================= recv =================\n");
            TRACE("read_cnt:%d\n", read_cnt);
            TRACE("readbuf:%s\n", readbuf);

            char command[RECV_BUF_SIZE];
            memset(command, '\0', sizeof(command));

            parse_val(readbuf, 0x0a, command);

            TRACE("----------------------------------------\n");
            if (strcmp(command, "3\n" ) == 0) {
                TRACE("command:%s\n", command);
                notify_sdb_daemon_start();
                notify_sensor_daemon_start();
            } 
            else if (strcmp(command, "4\n") == 0) {
                /* sdcard mount/umount msg recv from emuld */
                INFO("command:%s\n", command);
                char token[] = "\n";
                char* ret = NULL;
                ret = strtok(readbuf, token);
                INFO("%s\n", ret);

                ret = strtok(NULL, token);
                INFO("%s\n", ret);

                if (atoi(ret) == 0) {
                    /* umount sdcard */
                    mloop_evcmd_usbdisk(NULL);

                } else if (atoi(ret) == 1) {
                    /* mount sdcard */
                    char sdcard_path[256];
                    char* vms_path = get_emulator_vms_sdcard_path();
                    memset(sdcard_path, '\0', sizeof(sdcard_path));

                    strcpy(sdcard_path, vms_path);

                    /* emulator_vms_sdcard_path + sdcard img name */
                    ret = strtok(NULL, token);
                    strcat(sdcard_path, ret);
                    INFO("%s\n", sdcard_path);

                    mloop_evcmd_usbdisk(sdcard_path);
                    free(vms_path);
                } else {
                    ERR("!!! unknown command : %s\n", ret);
                }
            } else {
                ERR("!!! unknown command : %s\n", command);
            }

            TRACE("========================================\n");
        }
    }

cleanup:

#ifdef _WIN32
    if (server_sock) {
        closesocket(server_sock);
    }
#else
    if (server_sock) {
        close(server_sock);
    }
#endif

    server_sock = 0;

    return NULL;
}

static int parse_val(char* buff, unsigned char data, char* parsbuf)
{
    int count = 0;

    while ( 1 ) {
        if (count > 12) {
            return -1;
        }

        if (buff[count] == data) {
            count++;
            strncpy(parsbuf, buff, count);
            return count;
        }

        count++;
    }

    return 0;
}

void shutdown_guest_server(void)
{
    INFO("shutdown_guest_server.\n");

#ifdef _WIN32
    if (server_sock) {
        closesocket(server_sock);
    }
#else
    if (server_sock) {
        close(server_sock);
    }
#endif

    server_sock = 0;
}

