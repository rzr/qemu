/*
 * Emulator SDB Notification Server
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  MunKyu Im       <munkyu.im@samsung.com>
 *  Sangho Park     <sangho1206.park@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
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


#include "net/slirp.h"
#include "qemu/sockets.h"
#include "sdb.h"
#include "block/nbd.h"

#include "emulator.h"
#include "emulator_common.h"
#include "debug_ch.h"
#include "emul_state.h"

#include "hw/virtio/maru_virtio_hwkey.h"
#include "skin/maruskin_server.h"
#include "hw/maru_pm.h"
#include "ecs/ecs.h"

MULTI_DEBUG_CHANNEL(qemu, sdb);

#ifdef _WIN32
#include "qemu/main-loop.h"

static void socket_close_handler( void*  _fd )
{
    int   fd = (int)_fd;
    int   ret;
    char  buff[64];

    /* we want to drain the read side of the socket before closing it */
    do {
        ret = recv( fd, buff, sizeof(buff), 0 );
    } while (ret < 0 && WSAGetLastError() == WSAEINTR);

    if (ret < 0 && WSAGetLastError() == EWOULDBLOCK)
        return;

    qemu_set_fd_handler( fd, NULL, NULL, NULL );
    closesocket( fd );
}

void socket_close( int  fd )
{
    int  old_errno = errno;

    shutdown( fd, SD_BOTH );
    /* we want to drain the socket before closing it */
    qemu_set_fd_handler( fd, socket_close_handler, NULL, (void*)fd );

    errno = old_errno;
}

#else /* !_WIN32 */

#include <unistd.h>

void socket_close( int  fd )
{
    int  old_errno = errno;

    shutdown( fd, SHUT_RDWR );
    close( fd );

    errno = old_errno;
}

#endif /* !_WIN32 */

int inet_strtoip(const char*  str, uint32_t  *ip)
{
    int  comp[4];

    if (sscanf(str, "%d.%d.%d.%d", &comp[0], &comp[1], &comp[2], &comp[3]) != 4)
        return -1;

    if ((unsigned)comp[0] >= 256 ||
            (unsigned)comp[1] >= 256 ||
            (unsigned)comp[2] >= 256 ||
            (unsigned)comp[3] >= 256)
        return -1;

    *ip = (uint32_t)((comp[0] << 24) | (comp[1] << 16) |
            (comp[2] << 8)  |  comp[3]);
    return 0;
}

int check_port_bind_listen(uint32_t port)
{
    struct sockaddr_in addr;
    int s = 0;
    int ret = -1;
    socklen_t addrlen = sizeof(addr);
    memset(&addr, 0, addrlen);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    s = qemu_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        INFO("failed to create a socket\n");
        return -1;
    }

#ifndef _WIN32
    int opt = 1;
    ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&opt, sizeof(int));
    if (ret < 0) {
        INFO("setsockopt failure\n");
        close(s);
        return -1;
    }
#endif

    if ((bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) ||
        (listen(s, 1) < 0)) {
        /* failure */
        ret = -1;
        INFO("port(%d) listen failure\n", port);
    } else {
        /* success */
        ret = 1;
        INFO("port(%d) listen success\n", port);
    }

#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif

    return ret;
}

void set_base_port(void)
{
    int   tries     = 10;
    int   success   = 0;
    uint32_t port = 26100;
    int base_port;

    base_port = get_emul_vm_base_port();

    if(base_port == 0){

        for ( ; tries > 0; tries--, port += 10 ) {
            if(check_port_bind_listen(port + 1) < 0 )
                continue;

            success = 1;
            break;
        }

        if (!success) {
            INFO( "it seems too many emulator instances are running on this machine. Aborting\n" );
            exit(1);
        }

        base_port = port;
        INFO( "sdb port is %d\n", base_port);
    }

    set_emul_vm_base_port(base_port);
}

void sdb_setup(void)
{
    int   tries     = 10;
    int   success   = 0;
    char buf[64] = {0,};
    int number;

    number = get_device_serial_number();

    for ( ; tries > 0; tries--, number += 10 ) {
        sprintf(buf, "tcp:%d::26101", number);
        if(net_slirp_redir((char*)buf) < 0)
            continue;

        INFO( "SDBD established on port %d\n", number);
        success = 1;
        break;
    }

    INFO("redirect [%s] success\n", buf);
    if (!success) {
        INFO( "it seems too many emulator instances are running on this machine. Aborting\n" );
        exit(1);
    }

    INFO( "Port(%d/tcp) listen for SDB\n", number);
}

int sdb_loopback_client(int port, int type)
{
    struct sockaddr_in addr;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    s = socket(AF_INET, type, 0);
    if(s < 0) return -1;

    if(connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    return s;
}

/*
 * SDB Notification server
 */

#define RECV_BUF_SIZE 32
typedef struct SDB_Noti_Server {
    int server_fd;
    GIOChannel *server_chan;
    guint server_tag;
} SDB_Noti_Server;

typedef struct SDB_Client {
    int port;
    struct sockaddr_in addr;
    char serial[RECV_BUF_SIZE];

    QTAILQ_ENTRY(SDB_Client) next;
} SDB_Client;

static QTAILQ_HEAD(SDB_ClientHead, SDB_Client)
clients = QTAILQ_HEAD_INITIALIZER(clients);

static SDB_Noti_Server *current_server;
static QemuMutex mutex_clients;

static void remove_sdb_client(SDB_Client* client)
{
    if (client == NULL) {
        return;
    }

    qemu_mutex_lock(&mutex_clients);

    QTAILQ_REMOVE(&clients, client, next);

    qemu_mutex_unlock(&mutex_clients);

    g_free(client);
}

static void send_to_sdb_client(SDB_Client* client, int state)
{
    struct sockaddr_in sock_addr;
    int s, slen = sizeof(sock_addr);
    int serial_len = 0;
    char buf [32];

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
          INFO("socket creation error! %d\n", errno);
          return;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(client->port);
    sock_addr.sin_addr = (client->addr).sin_addr;

    if (connect(s, (struct sockaddr*)&sock_addr, slen) == -1) {
        INFO("connect error! remove this client.\n");
        remove_sdb_client(client);
        close(s);
        return;
    }

    memset(buf, 0, sizeof(buf));

    serial_len = strlen(client->serial);

    // send message "[4 digit message length]host:sync:emulator-26101:[0|1]"
    sprintf(buf, "%04xhost:sync:%s:%01d", (serial_len + 12), client->serial, state);

    INFO("send %s to client %s\n", buf, inet_ntoa(client->addr.sin_addr));

    if (send(s, buf, sizeof(buf), 0) == -1)
    {
        INFO("send error! remove this client.\n");
        remove_sdb_client(client);
    }

    close(s);
}

void notify_all_sdb_clients(int state)
{
    qemu_mutex_lock(&mutex_clients);
    SDB_Client *client, *next;

    QTAILQ_FOREACH_SAFE(client, &clients, next, next)
    {
        send_to_sdb_client(client, state);
    }
    qemu_mutex_unlock(&mutex_clients);

}

static void add_sdb_client(struct sockaddr_in* addr, int port, const char* serial)
{
    SDB_Client *cli = NULL;
    SDB_Client *client = NULL, *next;

    if (addr == NULL) {
        INFO("SDB_Client client's address is EMPTY.\n");
        return;
    } else if (serial == NULL || strlen(serial) <= 0) {
        INFO("SDB_Client client's serial is EMPTY.\n");
        return;
    } else if (strlen(serial) > RECV_BUF_SIZE) {
        INFO("SDB_Client client's serial is too long. %s\n", serial);
        return;
    }

    qemu_mutex_lock(&mutex_clients);
    QTAILQ_FOREACH_SAFE(cli, &clients, next, next)
    {
        if (!strcmp(serial, cli->serial) && !strcmp(inet_ntoa(addr->sin_addr), inet_ntoa((cli->addr).sin_addr))) {
            INFO("Client cannot be duplicated.\n");
            qemu_mutex_unlock(&mutex_clients);
            return;
        }
    }
    qemu_mutex_unlock(&mutex_clients);

    client = g_malloc0(sizeof(SDB_Client));
    if (NULL == client) {
        INFO("SDB_Client allocation failed.\n");
        return;
    }

    memcpy(&client->addr, addr, sizeof(struct sockaddr_in));
    client->port = port;
    strcpy(client->serial, serial);

    qemu_mutex_lock(&mutex_clients);

    QTAILQ_INSERT_TAIL(&clients, client, next);

    qemu_mutex_unlock(&mutex_clients);

    INFO("Added new sdb client. ip: %s, port: %d, serial: %s\n", inet_ntoa((client->addr).sin_addr), client->port, client->serial);

    send_to_sdb_client(client, runstate_check(RUN_STATE_SUSPENDED));
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

#define SDB_SERVER_PORT 26097
static void register_sdb_server(char* readbuf, struct sockaddr_in* client_addr)
{
    int port = 0;
    char token[] = "\n";
    char* ret = NULL;
    char* serial = NULL;

    ret = strtok(readbuf, token);
    if (ret == NULL) {
        INFO("command is not found.");
        return;
    }

    serial = strtok(NULL, token);
    if (serial == NULL) {
        INFO("serial is not found.");
        return;
    }

    port = SDB_SERVER_PORT;

    add_sdb_client(client_addr, port, serial);
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

static void suspend_lock_state(int state)
{
    ecs_suspend_lock_state(state);
}

static void command_handler(char* readbuf, struct sockaddr_in* client_addr)
{
    char command[RECV_BUF_SIZE];
    memset(command, '\0', sizeof(command));

    parse_val(readbuf, 0x0a, command);

    TRACE("----------------------------------------\n");
    TRACE("command:%s\n", command);
    if (strcmp(command, "2\n" ) == 0) {
        notify_sdb_daemon_start();
    } else if (strcmp(command, "5\n") == 0) {
        register_sdb_server(readbuf, client_addr);
    } else if (strcmp(command, "6\n") == 0) {
        wakeup_guest();
    } else if (strcmp(command, "7\n") == 0) {
        suspend_lock_state(SUSPEND_LOCK);
    } else if (strcmp(command, "8\n") == 0) {
        suspend_lock_state(SUSPEND_UNLOCK);
    } else {
        INFO("!!! unknown command : %s\n", command);
    }
    TRACE("========================================\n");
}

static void close_clients(void)
{
    qemu_mutex_lock(&mutex_clients);
    SDB_Client * client, *next;

    QTAILQ_FOREACH_SAFE(client, &clients, next, next)
    {
        QTAILQ_REMOVE(&clients, client, next);

        if (NULL != client)
        {
            g_free(client);
        }
    }

    qemu_mutex_unlock(&mutex_clients);
}

static void close_server(void)
{
    if (current_server == NULL) {
        return;
    }

    close_clients();

    if (current_server->server_fd > 0) {
        if (current_server->server_tag) {
            g_source_remove(current_server->server_tag);
            current_server->server_tag = 0;
        }
        if (current_server->server_chan) {
            g_io_channel_unref(current_server->server_chan);
        }
        closesocket(current_server->server_fd);
    }

    g_free(current_server);

    qemu_mutex_destroy(&mutex_clients);
}

static gboolean sdb_noti_read(GIOChannel *channel, GIOCondition cond, void *opaque)
{
    int recv_cnt = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char readbuf[RECV_BUF_SIZE + 1];
    SDB_Noti_Server *server = opaque;

    memset(&readbuf, 0, sizeof(readbuf));


    recv_cnt = recvfrom(server->server_fd, readbuf, RECV_BUF_SIZE, 0,
                        (struct sockaddr*) &client_addr, &client_len);

    if (recv_cnt > 0) {
        command_handler((char*)readbuf, &client_addr);
    } else if (recv_cnt == 0) {
        INFO("noti server recvfrom returned 0.\n");
    } else {
#ifdef _WIN32
        errno = WSAGetLastError();
#endif
        TRACE("recvfrom error case (it can be from non-blocking socket): %d", errno);
    }

    return TRUE;
}

static int create_UDP_server(SDB_Noti_Server *server, int port)
{
    struct sockaddr_in server_addr;
    int opt = 1;

    if ((server->server_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        INFO("create listen socket error:%d\n", errno);
        return -1;
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    qemu_set_nonblock(server->server_fd);

    if (qemu_setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        INFO("setsockopt SO_REUSEADDR is failed.: %d\n", errno);
        return -1;
    }

    if (bind(server->server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        INFO("sdb noti server bind error: %d", errno);
        return -1;
    }

    return 0;
}

static GIOChannel *io_channel_from_socket(int fd)
{
    GIOChannel *chan;

    if (fd == -1) {
        return NULL;
    }

#ifdef _WIN32
    chan = g_io_channel_win32_new_socket(fd);
#else
    chan = g_io_channel_unix_new(fd);
#endif

    g_io_channel_set_encoding(chan, NULL, NULL);
    g_io_channel_set_buffered(chan, FALSE);

    return chan;
}

static void sdb_noti_server_notify_exit(Notifier *notifier, void *data)
{
    INFO("shutdown sdb notification server.\n");
    close_server();
}

static Notifier sdb_noti_server_exit = { .notify = sdb_noti_server_notify_exit };

void start_sdb_noti_server(int server_port)
{
    SDB_Noti_Server *server;
    int ret;

    INFO("start sdb noti server thread.\n");

    server = g_malloc0(sizeof(SDB_Noti_Server));
    if (server == NULL) {
        INFO("SDB Notification server allocation is failed.\n");
        return;
    }

    ret = create_UDP_server(server, server_port);
    if (ret < 0) {
        INFO("failed to create UDP server\n");
        close_server();
        return;
    }

    server->server_chan = io_channel_from_socket(server->server_fd);
    server->server_tag = g_io_add_watch(server->server_chan, G_IO_IN, sdb_noti_read, server);

    current_server = server;

    qemu_mutex_init(&mutex_clients);

    INFO("success to bind port[127.0.0.1:%d/udp] for sdb noti server in host \n", server_port);

    emulator_add_exit_notifier(&sdb_noti_server_exit);
}

