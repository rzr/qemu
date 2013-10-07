/*
 *
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JiHye Kim <jihye1128.kim@samsung.com>
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

#include "emulator.h"
#include "guest_server.h"
#include "mloop_event.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"
#include "sdb.h"
#include "maru_common.h"
#include "hw/maru_virtio_hwkey.h"

MULTI_DEBUG_CHANNEL(qemu, guest_server);

#define RECV_BUF_SIZE 32

static int svr_port = 0;
static int server_sock = 0;

/*
 * SDB server data
 */
typedef struct GS_Client {
    int port;
    char addr[RECV_BUF_SIZE];

    QTAILQ_ENTRY(GS_Client) next;
} GS_Client;

static QTAILQ_HEAD(GS_ClientHead, GS_Client)
clients = QTAILQ_HEAD_INITIALIZER(clients);

static pthread_mutex_t mutex_clilist = PTHREAD_MUTEX_INITIALIZER;

static void add_sdb_client(const char* addr, int port)
{
    GS_Client *client = g_malloc0(sizeof(GS_Client));
    if (NULL == client) {
        INFO("GS_Client allocation failed.\n");
        return;
    }

    if (addr == NULL || strlen(addr) <= 0) {
        INFO("GS_Client client's address is EMPTY.\n");
        return;
    } else if (strlen(addr) > RECV_BUF_SIZE) {
        INFO("GS_Client client's address is too long. %s\n", addr);
        return;
    }

    strcpy(client->addr, addr);
    client->port = port;

    pthread_mutex_lock(&mutex_clilist);

    QTAILQ_INSERT_TAIL(&clients, client, next);

    pthread_mutex_unlock(&mutex_clilist);

    INFO("Added new sdb client. ip: %s, port: %d\n", client->addr, client->port);
}

static void remove_sdb_client(GS_Client* client)
{
    pthread_mutex_lock(&mutex_clilist);

    QTAILQ_REMOVE(&clients, client, next);
    if (NULL != client) {
        g_free(client);
    }

    pthread_mutex_unlock(&mutex_clilist);
}

static void send_to_client(GS_Client* client, int state)
{
    struct sockaddr_in sock_addr;
    int s, slen = sizeof(sock_addr);
    char buf [32];

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
          INFO("socket error!\n");
          return;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(client->port);
    if (inet_aton(client->addr, &sock_addr.sin_addr) == 0) {
          INFO("inet_aton() failed\n");
    }

    memset(buf, 0, sizeof(buf));

    // send message "[4 digit message length]host:sync:emulator-26101:[0|1]"
    sprintf(buf, "0026host:sync:emulator-%d:%d", svr_port, state);

    if (sendto(s, buf, sizeof(buf), 0, (struct sockaddr*)&sock_addr, slen) == -1)
    {
        INFO("sendto error! remove this client.\n");
        remove_sdb_client(client);
    }

    close(s);
}

void notify_all_sdb_clients(int state)
{
    pthread_mutex_lock(&mutex_clilist);
    GS_Client *client;

    QTAILQ_FOREACH(client, &clients, next)
    {
        send_to_client(client, state);
    }
    pthread_mutex_unlock(&mutex_clilist);

}

static int parse_val(char* buff, unsigned char data, char* parsbuf)
{
    int count = 0;

    while (1) {
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

/*
 *  In case that SDK does not refer to sdk.info to get tizen-sdk-data path.
 *  When SDK is not installed by the latest SDK installer,
 *  SDK installed path is fixed and there is no sdk.info file.
 */
static gchar *get_old_tizen_sdk_data_path(void)
{
    gchar *tizen_sdk_data_path = NULL;

    INFO("try to search tizen-sdk-data path in another way.\n");

#ifndef CONFIG_WIN32
    gchar tizen_sdk_data[] = "/tizen-sdk-data";
    gint tizen_sdk_data_len = 0;
    gchar *home_dir;

    home_dir = (gchar *)g_getenv("HOME");
    if (!home_dir) {
        home_dir = (gchar *)g_get_home_dir();
    }

    tizen_sdk_data_len = strlen(home_dir) + sizeof(tizen_sdk_data) + 1;
    tizen_sdk_data_path = g_malloc(tizen_sdk_data_len);
    if (!tizen_sdk_data_path) {
        INFO("failed to allocate memory.\n");
        return NULL;
    }
    g_strlcpy(tizen_sdk_data_path, home_dir, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);

#else
    gchar tizen_sdk_data[] = "\\tizen-sdk-data\\";
    gint tizen_sdk_data_len = 0;
    HKEY hKey;
    char strLocalAppDataPath[1024] = { 0 };
    DWORD dwBufLen = 1024;

    RegOpenKeyEx(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        0, KEY_QUERY_VALUE, &hKey);

    RegQueryValueEx(hKey, "Local AppData", NULL,
                    NULL, (LPBYTE)strLocalAppDataPath, &dwBufLen);
    RegCloseKey(hKey);

    tizen_sdk_data_len = strlen(strLocalAppDataPath) + sizeof(tizen_sdk_data) + 1;
    tizen_sdk_data_path = g_malloc(tizen_sdk_data_len);
    if (!tizen_sdk_data_path) {
        INFO("failed to allocate memory.\n");
        return NULL;
    }

    g_strlcpy(tizen_sdk_data_path, strLocalAppDataPath, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);
#endif

    INFO("tizen-sdk-data path: %s\n", tizen_sdk_data_path);
    return tizen_sdk_data_path;
}

/*
 *  get tizen-sdk-data path from sdk.info.
 */
static gchar *get_tizen_sdk_data_path(void)
{
    gchar *emul_bin_path = NULL;
    gchar *sdk_info_file_path = NULL;
    gchar *tizen_sdk_data_path = NULL;
#ifndef CONFIG_WIN32
    const char *sdk_info = "../../../sdk.info";
#else
    const char *sdk_info = "..\\..\\..\\sdk.info";
#endif
    const char sdk_data_var[] = "TIZEN_SDK_DATA_PATH";

    FILE *sdk_info_fp = NULL;
    int sdk_info_path_len = 0;

    TRACE("%s\n", __func__);

    emul_bin_path = get_bin_path();
    if (!emul_bin_path) {
        INFO("failed to get emulator path.\n");
        return NULL;
    }

    sdk_info_path_len = strlen(emul_bin_path) + strlen(sdk_info) + 1;
    sdk_info_file_path = g_malloc(sdk_info_path_len);
    if (!sdk_info_file_path) {
        INFO("failed to allocate sdk-data buffer.\n");
        return NULL;
    }

    g_snprintf(sdk_info_file_path, sdk_info_path_len, "%s%s",
                emul_bin_path, sdk_info);
    INFO("sdk.info path: %s\n", sdk_info_file_path);

    sdk_info_fp = fopen(sdk_info_file_path, "r");
    g_free(sdk_info_file_path);

    if (sdk_info_fp) {
        INFO("Succeeded to open [sdk.info].\n");

        char tmp[256] = { '\0', };
        char *tmpline = NULL;
        while (fgets(tmp, sizeof(tmp), sdk_info_fp) != NULL) {
            if ((tmpline = g_strstr_len(tmp, sizeof(tmp), sdk_data_var))) {
                tmpline += strlen(sdk_data_var) + 1; // 1 for '='
                break;
            }
        }

        if (tmpline) {
            if (tmpline[strlen(tmpline) - 1] == '\n') {
                tmpline[strlen(tmpline) - 1] = '\0';
            }
            if (tmpline[strlen(tmpline) - 1] == '\r') {
                tmpline[strlen(tmpline) - 1] = '\0';
            }

            tizen_sdk_data_path = g_malloc(strlen(tmpline) + 1);
            g_strlcpy(tizen_sdk_data_path, tmpline, strlen(tmpline) + 1);

            INFO("tizen-sdk-data path: %s\n", tizen_sdk_data_path);

            return tizen_sdk_data_path;
        }

        fclose(sdk_info_fp);
    }

    // legacy mode
    INFO("Failed to open [sdk.info].\n");

    return get_old_tizen_sdk_data_path();
}

static char* get_emulator_sdcard_path(void)
{
    gchar *emulator_sdcard_path = NULL;
    gchar *tizen_sdk_data = NULL;
#ifndef CONFIG_WIN32
    char emulator_sdcard[] = "/emulator/sdcard/";
#else
    char emulator_sdcard[] = "\\emulator\\sdcard\\";
#endif

    TRACE("emulator_sdcard: %s, %d\n", emulator_sdcard, sizeof(emulator_sdcard));

    tizen_sdk_data = get_tizen_sdk_data_path();
    if (!tizen_sdk_data) {
        INFO("failed to get tizen-sdk-data path.\n");
        return NULL;
    }

    emulator_sdcard_path =
        g_malloc(strlen(tizen_sdk_data) + sizeof(emulator_sdcard) + 1);
    if (!emulator_sdcard_path) {
        INFO("failed to allocate memory.\n");
        return NULL;
    }

    g_snprintf(emulator_sdcard_path, strlen(tizen_sdk_data) + sizeof(emulator_sdcard),
             "%s%s", tizen_sdk_data, emulator_sdcard);

    g_free(tizen_sdk_data);

    TRACE("sdcard path: %s\n", emulator_sdcard_path);
    return emulator_sdcard_path;
}

static void handle_sdcard(char* readbuf)
{
    char token[] = "\n";
    char* ret = NULL;
    ret = strtok(readbuf, token);
    ret = strtok(NULL, token);

    if (atoi(ret) == 0) {
        /* umount sdcard */
        //mloop_evcmd_usbdisk(NULL);
        mloop_evcmd_sdcard(NULL);
    } else if (atoi(ret) == 1) {
        /* mount sdcard */
        char sdcard_img_path[256];
        char* sdcard_path = NULL;

        sdcard_path = get_emulator_sdcard_path();
        if (sdcard_path) {
            g_strlcpy(sdcard_img_path, sdcard_path, sizeof(sdcard_img_path));

            /* emulator_sdcard_img_path + sdcard img name */
            ret = strtok(NULL, token);

            g_strlcat(sdcard_img_path, ret, sizeof(sdcard_img_path));
            TRACE("sdcard path: %s\n", sdcard_img_path);

            //mloop_evcmd_usbdisk(sdcard_img_path);
            mloop_evcmd_sdcard(sdcard_img_path);

            g_free(sdcard_path);
        } else {
            INFO("failed to get sdcard path!!\n");
        }
    } else {
        INFO("!!! unknown command : %s\n", ret);
    }
}

static void register_sdb_server(char* readbuf)
{
    int port = 0;
    char token[] = "\n";
    char* ret = NULL;
    char* addr = NULL;
    ret = strtok(readbuf, token);
    addr = strtok(NULL, token);
    if (addr == NULL)
        return;
    ret = strtok(NULL, token);
    if (ret == NULL)
        return;
    port = atoi(ret);

    add_sdb_client(addr, port);
}

#define PRESS     1
#define RELEASE   2
#define POWER_KEY 116
static void wakeup_guest(void)
{
    // FIXME: Temporarily working model.
    // It must be fixed as the way it works.
    maru_hwkey_event(PRESS, POWER_KEY);
    maru_hwkey_event(RELEASE, POWER_KEY);
}

static void command_handler(char* readbuf)
{
    char command[RECV_BUF_SIZE];
    memset(command, '\0', sizeof(command));

    parse_val(readbuf, 0x0a, command);

    TRACE("----------------------------------------\n");
    TRACE("command:%s\n", command);
    if (strcmp(command, "2\n" ) == 0) {
        notify_sdb_daemon_start();
    } else if (strcmp(command, "3\n" ) == 0) {
        notify_sensor_daemon_start();
        notify_ecs_server_start();
    } else if (strcmp(command, "4\n") == 0) {
        handle_sdcard(readbuf);
    } else if (strcmp(command, "5\n") == 0) {
        register_sdb_server(readbuf);
    } else if (strcmp(command, "6\n") == 0) {
        wakeup_guest();
    } else {
        INFO("!!! unknown command : %s\n", command);
    }
    TRACE("========================================\n");
}

static void server_process(void)
{
    int read_cnt = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    char readbuf[RECV_BUF_SIZE];

    client_len = sizeof(client_addr);

    while (1) {
        memset(&readbuf, 0, RECV_BUF_SIZE);

        if (server_sock == 0) {
            INFO("server_sock is closed\n");
            return;
        }
        read_cnt = recvfrom(server_sock, readbuf, RECV_BUF_SIZE, 0,
                            (struct sockaddr*) &client_addr, &client_len);

        if (read_cnt < 0) {
            INFO("fail to recvfrom in guest_server.\n");
            perror("fail to recvfrom in guest_server.:");
            break;
        } else {

            if (read_cnt == 0) {
                INFO("read_cnt is 0.\n");
                break;
            }

            TRACE("================= recv =================\n");
            TRACE("read_cnt:%d\n", read_cnt);
            TRACE("readbuf:%s\n", readbuf);

            command_handler(readbuf);
        }
    }
}

static void close_clients(void)
{
    GS_Client * client;

    pthread_mutex_lock(&mutex_clilist);

    QTAILQ_FOREACH(client, &clients, next)
    {
        QTAILQ_REMOVE(&clients, client, next);

        if (NULL != client)
        {
            g_free(client);
        }
    }

    pthread_mutex_unlock(&mutex_clilist);
}

static void close_server(void)
{
    close_clients();
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

static void* run_guest_server(void* args)
{
    uint16_t port = svr_port;
    int opt = 1;
    struct sockaddr_in server_addr;

    INFO("start guest server thread.\n");

    if ((server_sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        INFO("create listen socket error\n");
        perror("create listen socket error\n");

        close_server();

        return NULL;
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        INFO("guest server bind error: ");
        perror("bind");

        close_server();

        return NULL;
    } else {
        INFO("success to bind port[127.0.0.1:%d/udp] for guest_server in host \n", port);
    }

    INFO("guest server start...port:%d\n", port);

    server_process();

    close_server();

    return NULL;
}

pthread_t start_guest_server(int server_port)
{
    svr_port = server_port;

    pthread_t thread_id;

    if (0 != pthread_create(&thread_id, NULL, run_guest_server, NULL)) {
        INFO("fail to create guest_server pthread.\n");
    } else {
        INFO("created guest server thread\n");
    }

    return thread_id;

}

void shutdown_guest_server(void)
{
    INFO("shutdown_guest_server.\n");

    close_server();
}
