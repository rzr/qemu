/*
 * emulator controller client
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  JiHye Kim <jihye1128.kim@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
#ifndef __WIN32
#include <sys/ioctl.h>
#else
#define EISCONN WSAEISCONN
#define EALREADY WSAEALREADY
#endif

#include "qemu-common.h"
#include "qemu/main-loop.h"
#include "qemu/sockets.h"
#include "ui/console.h"

#include "emulator.h"
#include "common.h"
#include "sensor.h"
#include "touch.h"
#include "emul_state.h"
#include "ecs/ecs_tethering.h"
#include "genmsg/tethering.pb-c.h"

#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

#define TETHERING_MSG_HANDSHAKE_KEY     100
#define MSG_BUF_SIZE    255
#define MSG_LEN_SIZE    4

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define SEND_BUF_MAX_SIZE 4096

typedef struct tethering_recv_buf {
    uint32_t len;
    uint32_t stack_size;
    char data[MSG_BUF_SIZE];
} tethering_recv_buf;

typedef struct _TetheringState {
    int fd;

    // server address
    int port;
    gchar *ipaddress;

    // connection state
    int status;

    // receiver handling thread
    QemuThread thread;
    QemuMutex mutex;

    tethering_recv_buf recv_buf;

    // device state
    QTAILQ_HEAD(device, input_device_list) device;
    int device_node_cnt;

} TetheringState;

static TetheringState *tethering_client;
static tethering_recv_buf recv_buf;
// static bool app_state = false;

static void end_tethering_socket(int sockfd);
static void set_tethering_connection_status(int status);
#if 0
static void set_tethering_app_state(bool state);
static bool get_tethering_app_state(void);
#endif

// create master message
static void *build_tethering_msg(Tethering__TetheringMsg* msg, int *payloadsize)
{
    void *buf = NULL;
    int msg_packed_size = 0;

    msg_packed_size = tethering__tethering_msg__get_packed_size(msg);
    *payloadsize = msg_packed_size + MSG_LEN_SIZE;

    LOG_TRACE("create tethering_msg. msg_packed_size %d, payloadsize %d\n", msg_packed_size, *payloadsize);

    buf = g_malloc(*payloadsize);
    if (!buf) {
        LOG_SEVERE("failed to allocate memory\n");
        return NULL;
    }

    tethering__tethering_msg__pack(msg, buf + MSG_LEN_SIZE);

    LOG_TRACE("msg_packed_size 1 %x\n", msg_packed_size);
    msg_packed_size = htonl(msg_packed_size);
    LOG_TRACE("msg_packed_size 2 %x\n", msg_packed_size);

    memcpy(buf, &msg_packed_size, MSG_LEN_SIZE);

    return buf;
}

bool send_msg_to_controller(void *msg)
{
    Tethering__TetheringMsg * tetheringMsg = (Tethering__TetheringMsg *)msg;

    void *buf = NULL;
    int payload_size = 0, sent_size = 0;
    int total_buf_size = 0;
    int sockfd = 0;
    bool ret = true;
    uint32_t buf_offset = 0;

    buf = build_tethering_msg(tetheringMsg, &payload_size);
    if (!buf) {
        return false;
    }

    if (!tethering_client) {
        LOG_SEVERE("TetheringState is NULL\n");
        g_free(buf);
        return false;
    }
    sockfd = tethering_client->fd;

    total_buf_size = payload_size;
    do {
        LOG_TRACE("sending a buffer as many as this size: %d\n", total_buf_size);

        sent_size =
            qemu_sendto(sockfd, buf + buf_offset, total_buf_size, 0, NULL, 0);
        if (sent_size < 0) {
            perror("failed to send a packet");
            if (errno == EAGAIN) {
                fd_set writefds;
                struct timeval timeout;
                int result = 0;

                FD_ZERO(&writefds);
                FD_SET(sockfd, &writefds);

                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                result = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
                if (result < 0) {
                    LOG_INFO("not possible to send data\n");
                    ret = false;
                    break;
                }
                LOG_TRACE("possible to send data\n");
                continue;
            }

            LOG_SEVERE("failed to send a message. sent_size: %d\n", sent_size);
            ret = false;
            break;
        }

        LOG_TRACE("sent size: %d\n", sent_size);
        buf_offset += sent_size;
        total_buf_size -= sent_size;
    } while (total_buf_size > 0);

    LOG_TRACE("sent packets: %d, payload_size %d\n", (payload_size - total_buf_size), payload_size);
    g_free(buf);

    return ret;
}

static bool send_handshake_req_msg(void)
{
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;
    Tethering__HandShakeReq req = TETHERING__HAND_SHAKE_REQ__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    req.key = TETHERING_MSG_HANDSHAKE_KEY;

    msg.type = TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_REQ;
    msg.handshakereq = &req;

    LOG_TRACE("send handshake_req message\n");
    send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s\n", __func__);

    return true;
}

static bool send_emul_state_msg(void)
{
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;
    Tethering__EmulatorState emul_state = TETHERING__EMULATOR_STATE__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    emul_state.state = TETHERING__CONNECTION_STATE__DISCONNECTED;

    msg.type = TETHERING__TETHERING_MSG__TYPE__EMUL_STATE;
    msg.emulstate = &emul_state;

    LOG_INFO("send emulator_state message\n");
    send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s\n", __func__);

    return true;
}

// event messages
static bool build_event_msg(Tethering__EventMsg *event)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__EVENT_MSG;
    msg.eventmsg = event;

    ret = send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_event_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;
    Tethering__EventMsg event = TETHERING__EVENT_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    event.type = TETHERING__EVENT_MSG__TYPE__START_ANS;
    event.startans = &start_ans;

    LOG_TRACE("send event_start_ans message\n");
    ret = build_event_msg(&event);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_event_status_msg(Tethering__EventType event_type,
                                    Tethering__State status)
{
    bool ret = false;
    Tethering__EventMsg event = TETHERING__EVENT_MSG__INIT;
    Tethering__SetEventStatus event_status = TETHERING__SET_EVENT_STATUS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    event_status.type = event_type;
    event_status.state = status;

    event.type = TETHERING__EVENT_MSG__TYPE__EVENT_STATUS;
    event.setstatus = &event_status;

    LOG_TRACE("send event_set_event_status message\n");
    ret = build_event_msg(&event);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

// message handlers
static void msgproc_tethering_handshake_ans(Tethering__HandShakeAns *msg)
{
    // FIXME: handle handshake answer
    //  ans = msg->result;
}

static void msgproc_app_state_msg(Tethering__AppState *msg)
{
    int status = TETHERING__STATE__DISABLED;

    if (msg->state == TETHERING__CONNECTION_STATE__TERMINATED) {
        LOG_INFO("app is terminated\n");

        // set_tethering_app_state(false);
        set_tethering_sensor_status(status);
        set_tethering_touch_status(status);

        disconnect_tethering_app();
    } else {
        // does nothing
    }
}

static bool msgproc_tethering_event_msg(Tethering__EventMsg *msg)
{
    bool ret = true;

    switch(msg->type) {
    case TETHERING__EVENT_MSG__TYPE__START_REQ:
    {
        int touch_status = 0;

        LOG_TRACE("EVENT_MSG_TYPE_START_REQ\n");
        send_set_event_status_msg(TETHERING__EVENT_TYPE__SENSOR,
                                TETHERING__STATE__ENABLED);

        // TODO: check sensor device whether it exists or not
        set_tethering_sensor_status(TETHERING__STATE__ENABLED);

        if (is_emul_input_touch_enable()) {
            touch_status = TETHERING__STATE__ENABLED;
        } else {
            touch_status = TETHERING__STATE__DISABLED;
        }
        set_tethering_touch_status(touch_status);

        LOG_TRACE("send touch event_status msg: %d\n", touch_status);
        send_set_event_status_msg(TETHERING__EVENT_TYPE__TOUCH, touch_status);

        LOG_TRACE("send event_start_ans msg: %d\n", touch_status);
        send_event_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);
    }
        break;
    case TETHERING__EVENT_MSG__TYPE__TERMINATE:
        break;

    default:
        LOG_WARNING("invalid event_msg type\n");
        ret = false;
        break;
    }

    return ret;
}

static bool handle_tethering_msg_from_controller(char *data, int len)
{
    Tethering__TetheringMsg *tethering = NULL;
    bool ret = true;

    tethering = tethering__tethering_msg__unpack(NULL, (size_t)len,
                                            (const uint8_t *)data);

    if (!tethering) {
        LOG_SEVERE("no tethering massage\n");
        return false;
    }

    switch (tethering->type) {
    case TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_ANS:
    {
        // TODO: set the result of handshake_ans to
        Tethering__HandShakeAns *msg = tethering->handshakeans;
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_handshake_ans(msg);
            LOG_TRACE("receive handshake answer\n");

            set_tethering_connection_status(CONNECTED);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__APP_STATE:
    {
        Tethering__AppState *msg = tethering->appstate;

        LOG_TRACE("receive app_state msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_app_state_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__EVENT_MSG:
    {
        Tethering__EventMsg *msg = tethering->eventmsg;

        LOG_TRACE("receive event_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_event_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__SENSOR_MSG:
    {
        Tethering__SensorMsg *msg = tethering->sensormsg;

        LOG_TRACE("receive sensor_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_sensor_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG:
    {
        Tethering__TouchMsg *msg = tethering->touchmsg;

        LOG_TRACE("receive touch_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_touch_msg(msg);
        }
    }
        break;

    default:
        LOG_WARNING("invalid type message\n");
        ret = false;
        break;
    }

    tethering__tethering_msg__free_unpacked(tethering, NULL);
    return ret;
}

static void reset_tethering_recv_buf(void *opaque)
{
    memset(opaque, 0x00, sizeof(tethering_recv_buf));
}

// tethering client socket
static void tethering_io_handler(void *opaque)
{
    int payloadsize = 0, read_size = 0;
    int to_read_bytes = 0;
    int sockfd = 0, ret = 0;

    if (!tethering_client) {
        return;
    }
    sockfd = tethering_client->fd;

#ifndef CONFIG_WIN32
    ret = ioctl(sockfd, FIONREAD, &to_read_bytes);
    if (ret < 0) {
        perror("invalid ioctl opertion\n");
        disconnect_tethering_app();
        return;
    }

#else
    unsigned long to_read_bytes_long = 0;
    ret = ioctlsocket(sockfd, FIONREAD, &to_read_bytes_long);
    if (ret < 0) {
        perror("invalid ioctl opertion\n");
        disconnect_tethering_app();
        return;
    }

    to_read_bytes = (int)to_read_bytes_long;
#endif

    LOG_TRACE("ioctl: ret: %d, FIONREAD: %d\n", ret, to_read_bytes);
    if (to_read_bytes == 0) {
        LOG_INFO("there is no read data\n");
        disconnect_tethering_app();
        return;
    }

    // TODO: why this conditional is used??
    if (recv_buf.len == 0) {
        ret = qemu_recv(sockfd, &payloadsize, sizeof(payloadsize), 0);
        if (ret < sizeof(payloadsize)) {
            return;
        }

        payloadsize = ntohl(payloadsize);
        LOG_TRACE("payload size: %d\n", payloadsize);

#if 0
        if (payloadsize > to_read_bytes) {
            LOG_INFO("invalid payload size: %d\n", payloadsize);
            return;
        }
#endif
        recv_buf.len = payloadsize;
        to_read_bytes -= sizeof(payloadsize);
    }

    if (to_read_bytes == 0) {
        return;
    }

    to_read_bytes = min(to_read_bytes, (recv_buf.len - recv_buf.stack_size));

    read_size =
        qemu_recv(sockfd, (char *)(recv_buf.data + recv_buf.stack_size),
                    to_read_bytes, 0);
    if (read_size == 0) {
        LOG_SEVERE("failed to read data\n");
        disconnect_tethering_app();
        return;
    }

    recv_buf.stack_size += read_size;

    if (recv_buf.len == recv_buf.stack_size) {
        char *snd_buf = NULL;

        snd_buf = g_malloc(recv_buf.stack_size);
        if (!snd_buf) {
            return;
        } else {
            memcpy(snd_buf, recv_buf.data, recv_buf.stack_size);
            handle_tethering_msg_from_controller(snd_buf, recv_buf.stack_size);
            g_free(snd_buf);
            reset_tethering_recv_buf(&recv_buf);
        }
    }
}

// socket functions
static int start_tethering_socket(const char *ipaddress, int port)
{
    struct sockaddr_in addr;

    gchar serveraddr[32] = { 0, };
    int sock = -1;
    int ret = 0;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // i.e. 1234

    if (ipaddress == NULL) {
        g_strlcpy(serveraddr, "127.0.0.1", sizeof(serveraddr));
    } else {
        g_strlcpy(serveraddr, ipaddress, sizeof(serveraddr));
    }

    LOG_INFO("server ip address: %s, port: %d\n", serveraddr, port);
    ret = inet_aton(serveraddr, &addr.sin_addr);

    if (ret == 0) {
        LOG_SEVERE("inet_aton failure\n");
        return -1;
    }

    sock = qemu_socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        // set_tethering_connection_status(DISCONNECTED);
        LOG_SEVERE("tethering socket creation is failed\n", sock);
        return -1;
    }
    LOG_INFO("tethering socket is created: %d\n", sock);

    qemu_set_nonblock(sock);

    set_tethering_connection_status(CONNECTING);

    while (1) {
        ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

        if (ret == 0) {
            LOG_INFO("tethering socket is connected.\n");
            break;
        } else {
            int connection_errno = socket_error();

            if (connection_errno == EINPROGRESS) {
                fd_set writefds;
                struct timeval timeout;

                LOG_INFO("connection is progressing\n");

                FD_ZERO(&writefds);
                FD_SET(sock, &writefds);

                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                if (select(sock + 1, NULL, &writefds, NULL, &timeout) > 0) {
                    int opt;
                    socklen_t opt_size = sizeof(opt);

                    qemu_getsockopt(sock, SOL_SOCKET, SO_ERROR, &opt, &opt_size);
                    if (opt) {
                        LOG_SEVERE("error in connection %d - %s\n", opt, strerror(opt));
                    } else {
                        LOG_INFO("timeout or error is %d - %s\n", opt, strerror(opt));
                    }
                } else {
                    LOG_INFO("error connection %d - %s\n", errno, strerror(errno));
                }
                continue;
            } else if (connection_errno == EALREADY) {
                LOG_INFO("a previous connection has not yet been completed\n");
                ret = 0;
                continue;
            } else if (connection_errno == EISCONN) {
                LOG_INFO("connection is already connected\n");
                ret = 0;
                break;
            } else {
                perror("connect failure");
                ret = -connection_errno;
                break;
            }
        }
    }

    if (ret < 0) {
        if (ret == -ECONNREFUSED) {
            LOG_INFO("socket connection is refused\n");
            set_tethering_connection_status(CONNREFUSED);
        }
        LOG_TRACE("close socket\n");
        end_tethering_socket(sock);
        sock = -1;
    }

    return sock;
}

static void end_tethering_socket(int sockfd)
{
    int status = TETHERING__STATE__DISABLED;

    LOG_TRACE("enter: %s\n", __func__);

    if (closesocket(sockfd) < 0) {
        perror("closesocket failure");
        return;
    }

    tethering_client->fd = -1;

    LOG_INFO("close tethering socket\n");
    set_tethering_connection_status(DISCONNECTED);
    set_tethering_sensor_status(status);
    set_tethering_touch_status(status);

    LOG_TRACE("leave: %s\n", __func__);
}

#if 0
static void set_tethering_app_state(bool state)
{
    LOG_TRACE("set tethering_app state: %d", state);
    app_state = state;
}

static bool get_tethering_app_state(void)
{
    return app_state;
}
#endif

// ecs <-> tethering
int get_tethering_connection_status(void)
{
    int status = 0;

    if (!tethering_client) {
        return -1;
    }

    qemu_mutex_lock(&tethering_client->mutex);
    status = tethering_client->status;
    qemu_mutex_unlock(&tethering_client->mutex);

    return status;
}

static void set_tethering_connection_status(int status)
{
    if (!tethering_client) {
        return;
    }

    qemu_mutex_lock(&tethering_client->mutex);
    tethering_client->status = status;
    qemu_mutex_unlock(&tethering_client->mutex);

    send_tethering_connection_status_ecp();
}

static void tethering_notify_exit(Notifier *notifier, void *data)
{
    LOG_INFO("tethering_notify_exit\n");
    disconnect_tethering_app();
}
static Notifier tethering_exit = { .notify = tethering_notify_exit };

static void *initialize_tethering_socket(void *opaque);

int connect_tethering_app(const char *ipaddress, int port)
{
    TetheringState *client = NULL;

    client = g_malloc0(sizeof(TetheringState));
    if (!client) {
        return -1;
    }

    client->port = port;

    if (ipaddress) {
        int ipaddr_len = 0;

        ipaddr_len = strlen(ipaddress);

        client->ipaddress = g_malloc0(ipaddr_len + 1);
        if (!client->ipaddress) {
            g_free(client);
            return -1;
        }

        g_strlcpy(client->ipaddress, ipaddress, ipaddr_len);
    } else {
        client->ipaddress = NULL;
    }

    tethering_client = client;

    qemu_mutex_init(&tethering_client->mutex);

    qemu_thread_create(&tethering_client->thread, "tethering-io-thread",
            initialize_tethering_socket, client,
            QEMU_THREAD_DETACHED);

    return 0;
}

int disconnect_tethering_app(void)
{
    int sock = 0;

    LOG_TRACE("disconnect app from ecp\n");
    if (!tethering_client) {
        LOG_SEVERE("tethering client instance is NULL\n");
        return -1;
    }

    sock = tethering_client->fd;
    if (sock < 0) {
        LOG_SEVERE("tethering socket is terminated or not ready\n");
        return -1;
    } else {
        send_emul_state_msg();
        end_tethering_socket(sock);
    }

    return 0;
}

static int tethering_loop(int sockfd)
{
    int ret = 0;
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    LOG_TRACE("select timeout! result: %d\n", ret);

    if (ret > 0) {
        LOG_TRACE("ready for read operation!!\n");
        tethering_io_handler(socket);
    }

    return ret;
}

static void *initialize_tethering_socket(void *opaque)
{
    TetheringState *client = (TetheringState *)opaque;
    LOG_TRACE("callback function for tethering_thread\n");

    if (!client) {
        LOG_SEVERE("TetheringState is NULL\n");
        return NULL;
    }

    client->fd = start_tethering_socket(client->ipaddress, client->port);
    if (client->fd < 0) {
        LOG_SEVERE("failed to start tethering_socket\n");
        // tethering_sock = -1;
        return NULL;
    }
    LOG_TRACE("tethering_sock: %d\n", client->fd);

    reset_tethering_recv_buf(&recv_buf);
    send_handshake_req_msg();

    emulator_add_exit_notifier(&tethering_exit);

    while (1) {
        qemu_mutex_lock(&client->mutex);
        if (client->status == DISCONNECTED) {
            qemu_mutex_unlock(&client->mutex);
            LOG_INFO("disconnected socket. destroy this thread\n");
            break;
        }
        qemu_mutex_unlock(&client->mutex);

        tethering_loop(client->fd);
    }

    return client;
}
