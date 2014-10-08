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
#include "ecs/ecs_eventcast.h"
#include "genmsg/eventcast.pb-c.h"

#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

#define EVENTCAST_MSG_HANDSHAKE_KEY     100
#define MSG_BUF_SIZE    255
#define MSG_LEN_SIZE    4

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define SEND_BUF_MAX_SIZE 4096
static const char *loopback = "127.0.0.1";

enum connection_type {
    NONE = 0,
    USB,
    WIFI,
};

typedef struct eventcast_recv_buf {
    uint32_t len;
    uint32_t stack_size;
    char data[MSG_BUF_SIZE];
} eventcast_recv_buf;

typedef struct _EventcastState {
    int fd;

    // server address
    int port;
    gchar *ipaddress;

    // connection state
    int status;
    int type;

    // receiver handling thread
    QemuThread thread;
    QemuMutex mutex;

    eventcast_recv_buf recv_buf;

    // device state
    QTAILQ_HEAD(device, input_device_list) device;
    int device_node_cnt;

} EventcastState;

static EventcastState *eventcast_client = NULL;
static eventcast_recv_buf recv_buf;

static void end_eventcast_socket(int sockfd);
static void set_eventcast_connection_status(int status);
#if 0
static void set_eventcast_app_state(bool state);
static bool get_eventcast_app_state(void);
#endif

// create master message
static void *build_eventcast_msg(Eventcast__EventCastMsg* msg, int *payloadsize)
{
    void *buf = NULL;
    int msg_packed_size = 0;

    msg_packed_size = eventcast__event_cast_msg__get_packed_size(msg);
    *payloadsize = msg_packed_size + MSG_LEN_SIZE;

    LOG_TRACE("create eventcast_msg. msg_packed_size %d, payloadsize %d\n", msg_packed_size, *payloadsize);

    buf = g_malloc(*payloadsize);
    if (!buf) {
        LOG_SEVERE("failed to allocate memory\n");
        return NULL;
    }

    eventcast__event_cast_msg__pack(msg, buf + MSG_LEN_SIZE);

    LOG_TRACE("msg_packed_size 1 %x\n", msg_packed_size);
    msg_packed_size = htonl(msg_packed_size);
    LOG_TRACE("msg_packed_size 2 %x\n", msg_packed_size);

    memcpy(buf, &msg_packed_size, MSG_LEN_SIZE);

    return buf;
}

bool send_msg_to_controller(void *msg)
{
    Eventcast__EventCastMsg * eventcastMsg = (Eventcast__EventCastMsg *)msg;

    void *buf = NULL;
    int payload_size = 0, sent_size = 0;
    int total_buf_size = 0;
    int sockfd = 0;
    bool ret = true;
    uint32_t buf_offset = 0;

    buf = build_eventcast_msg(eventcastMsg, &payload_size);
    if (!buf) {
        return false;
    }

    if (!eventcast_client) {
        LOG_SEVERE("EventcastState is NULL\n");
        g_free(buf);
        return false;
    }
    sockfd = eventcast_client->fd;

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
    Eventcast__EventCastMsg msg = EVENTCAST__EVENT_CAST_MSG__INIT;
    Eventcast__HandShakeReq req = EVENTCAST__HAND_SHAKE_REQ__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    req.key = EVENTCAST_MSG_HANDSHAKE_KEY;

    msg.type = EVENTCAST__EVENT_CAST_MSG__TYPE__HANDSHAKE_REQ;
    msg.handshakereq = &req;

    LOG_TRACE("send handshake_req message\n");
    send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s\n", __func__);

    return true;
}

static bool send_emul_state_msg(void)
{
    Eventcast__EventCastMsg msg = EVENTCAST__EVENT_CAST_MSG__INIT;
    Eventcast__EmulatorState emul_state = EVENTCAST__EMULATOR_STATE__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    emul_state.state = EVENTCAST__CONNECTION_STATE__DISCONNECTED;

    msg.type = EVENTCAST__EVENT_CAST_MSG__TYPE__EMUL_STATE;
    msg.emulstate = &emul_state;

    LOG_INFO("send emulator_state message\n");
    send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s\n", __func__);

    return true;
}

// event messages
static bool build_event_msg(Eventcast__EventMsg *event)
{
    bool ret = false;
    Eventcast__EventCastMsg msg = EVENTCAST__EVENT_CAST_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    msg.type = EVENTCAST__EVENT_CAST_MSG__TYPE__EVENT_MSG;
    msg.eventmsg = event;

    ret = send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_event_start_ans_msg(Eventcast__MessageResult result)
{
    bool ret = false;
    Eventcast__EventMsg event = EVENTCAST__EVENT_MSG__INIT;
    Eventcast__StartAns start_ans = EVENTCAST__START_ANS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    event.type = EVENTCAST__EVENT_MSG__TYPE__START_ANS;
    event.startans = &start_ans;

    LOG_TRACE("send event_start_ans message\n");
    ret = build_event_msg(&event);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_event_status_msg(Eventcast__EventType event_type,
                                    Eventcast__State status)
{
    bool ret = false;
    Eventcast__EventMsg event = EVENTCAST__EVENT_MSG__INIT;
    Eventcast__SetEventStatus event_status = EVENTCAST__SET_EVENT_STATUS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    event_status.type = event_type;
    event_status.state = status;

    event.type = EVENTCAST__EVENT_MSG__TYPE__EVENT_STATUS;
    event.setstatus = &event_status;

    LOG_TRACE("send event_set_event_status message\n");
    ret = build_event_msg(&event);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

// message handlers
static void msgproc_eventcast_handshake_ans(Eventcast__HandShakeAns *msg)
{
    // handle handshake answer
}

static void msgproc_app_state_msg(Eventcast__AppState *msg)
{
    int status = EVENTCAST__STATE__DISABLED;

    if (msg->state == EVENTCAST__CONNECTION_STATE__TERMINATED) {
        LOG_INFO("app is terminated\n");

        // set_eventcast_app_state(false);
        set_eventcast_sensor_status(status);
        set_eventcast_touch_status(status);

        disconnect_eventcast_app();
    }
}

static bool msgproc_eventcast_event_msg(Eventcast__EventMsg *msg)
{
    bool ret = true;

    switch(msg->type) {
    case EVENTCAST__EVENT_MSG__TYPE__START_REQ:
    {
        int touch_status = 0;

        LOG_TRACE("EVENT_MSG_TYPE_START_REQ\n");
        send_set_event_status_msg(EVENTCAST__EVENT_TYPE__SENSOR,
                                EVENTCAST__STATE__ENABLED);

        // TODO: check sensor device whether it exists or not
        set_eventcast_sensor_status(EVENTCAST__STATE__ENABLED);

        if (is_emul_input_touch_enable()) {
            touch_status = EVENTCAST__STATE__ENABLED;
        } else {
            touch_status = EVENTCAST__STATE__DISABLED;
        }
        set_eventcast_touch_status(touch_status);

        LOG_TRACE("send touch event_status msg: %d\n", touch_status);
        send_set_event_status_msg(EVENTCAST__EVENT_TYPE__TOUCH, touch_status);

        LOG_TRACE("send event_start_ans msg: %d\n", touch_status);
        send_event_start_ans_msg(EVENTCAST__MESSAGE_RESULT__SUCCESS);
    }
        break;
    case EVENTCAST__EVENT_MSG__TYPE__TERMINATE:
        break;

    default:
        LOG_TRACE("invalid event_msg type\n");
        ret = false;
        break;
    }

    return ret;
}

static bool handle_eventcast_msg_from_controller(char *data, int len)
{
    Eventcast__EventCastMsg *eventcast = NULL;
    bool ret = true;

    eventcast = eventcast__event_cast_msg__unpack(NULL, (size_t)len,
                                            (const uint8_t *)data);

    if (!eventcast) {
        LOG_SEVERE("no eventcast massage\n");
        return false;
    }

    switch (eventcast->type) {
    case EVENTCAST__EVENT_CAST_MSG__TYPE__HANDSHAKE_ANS:
    {
        // TODO: set the result of handshake_ans to
        Eventcast__HandShakeAns *msg = eventcast->handshakeans;
        if (!msg) {
            ret = false;
        } else {
            msgproc_eventcast_handshake_ans(msg);
            LOG_TRACE("receive handshake answer\n");

            set_eventcast_connection_status(CONNECTED);
        }
    }
        break;
    case EVENTCAST__EVENT_CAST_MSG__TYPE__APP_STATE:
    {
        Eventcast__AppState *msg = eventcast->appstate;

        LOG_TRACE("receive app_state msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_app_state_msg(msg);
        }
    }
        break;
    case EVENTCAST__EVENT_CAST_MSG__TYPE__EVENT_MSG:
    {
        Eventcast__EventMsg *msg = eventcast->eventmsg;

        LOG_TRACE("receive event_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_eventcast_event_msg(msg);
        }
    }
        break;
    case EVENTCAST__EVENT_CAST_MSG__TYPE__SENSOR_MSG:
    {
        Eventcast__SensorMsg *msg = eventcast->sensormsg;

        LOG_TRACE("receive sensor_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_eventcast_sensor_msg(msg);
        }
    }
        break;
    case EVENTCAST__EVENT_CAST_MSG__TYPE__TOUCH_MSG:
    {
        Eventcast__TouchMsg *msg = eventcast->touchmsg;

        LOG_TRACE("receive touch_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_eventcast_touch_msg(msg);
        }
    }
        break;

    default:
        LOG_WARNING("invalid type message\n");
        ret = false;
        break;
    }

    eventcast__event_cast_msg__free_unpacked(eventcast, NULL);
    return ret;
}

static void reset_eventcast_recv_buf(void *opaque)
{
    memset(opaque, 0x00, sizeof(eventcast_recv_buf));
}

// eventcast client socket
static void eventcast_io_handler(void *opaque)
{
    int payloadsize = 0, read_size = 0;
    int to_read_bytes = 0;
    int sockfd = 0, ret = 0;

    if (!eventcast_client) {
        return;
    }
    sockfd = eventcast_client->fd;

#ifndef CONFIG_WIN32
    ret = ioctl(sockfd, FIONREAD, &to_read_bytes);
    if (ret < 0) {
        perror("invalid ioctl opertion\n");
        disconnect_eventcast_app();
        return;
    }

#else
    unsigned long to_read_bytes_long = 0;
    ret = ioctlsocket(sockfd, FIONREAD, &to_read_bytes_long);
    if (ret < 0) {
        perror("invalid ioctl opertion\n");
        disconnect_eventcast_app();
        return;
    }

    to_read_bytes = (int)to_read_bytes_long;
#endif

    LOG_TRACE("ioctl: ret: %d, FIONREAD: %d\n", ret, to_read_bytes);
    if (to_read_bytes == 0) {
        LOG_INFO("there is no read data\n");
        disconnect_eventcast_app();
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
        disconnect_eventcast_app();
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
            handle_eventcast_msg_from_controller(snd_buf, recv_buf.stack_size);
            g_free(snd_buf);
            reset_eventcast_recv_buf(&recv_buf);
        }
    }
}

// socket functions
static int start_eventcast_socket(const char *ipaddress, int port)
{
    struct sockaddr_in addr;
    int sock = -1, ret = 0;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // i.e. 1234

    LOG_INFO("server ip address: %s, port: %d\n", ipaddress, port);
    ret = inet_aton(ipaddress, &addr.sin_addr);
    if (ret == 0) {
        LOG_SEVERE("inet_aton failure\n");
        return -1;
    }

    sock = qemu_socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        // set_eventcast_connection_status(DISCONNECTED);
        LOG_SEVERE("eventcast socket creation is failed\n", sock);
        return -1;
    }
    LOG_INFO("eventcast socket is created: %d\n", sock);

    qemu_set_nonblock(sock);

    set_eventcast_connection_status(CONNECTING);

    while (1) {
        ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

        if (ret == 0) {
            LOG_INFO("eventcast socket is connected.\n");
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
            set_eventcast_connection_status(CONNREFUSED);
        }
        LOG_INFO("close socket\n");
        end_eventcast_socket(sock);
        sock = -1;
    }

    return sock;
}

static void end_eventcast_socket(int sockfd)
{
    int status = EVENTCAST__STATE__DISABLED;

    LOG_TRACE("enter: %s\n", __func__);

    if (closesocket(sockfd) < 0) {
        perror("closesocket failure");
        return;
    }

    eventcast_client->fd = -1;

    LOG_INFO("close eventcast socket\n");
    set_eventcast_connection_status(DISCONNECTED);
    set_eventcast_sensor_status(status);
    set_eventcast_touch_status(status);

    LOG_TRACE("leave: %s\n", __func__);
}

#if 0
static void set_eventcast_app_state(bool state)
{
    LOG_TRACE("set eventcast_app state: %d", state);
    app_state = state;
}

static bool get_eventcast_app_state(void)
{
    return app_state;
}
#endif

// ecs <-> eventcast
int get_eventcast_connection_status(void)
{
    int status = 0;

    if (!eventcast_client) {
        LOG_INFO("eventcast_client is null\n");
        LOG_INFO("tetherging connection status: %d\n", status);
        return DISCONNECTED;
    }

    qemu_mutex_lock(&eventcast_client->mutex);
    status = eventcast_client->status;
    qemu_mutex_unlock(&eventcast_client->mutex);

    LOG_INFO("tetherging connection status: %d\n", status);

    return status;
}

int get_eventcast_connected_port(void)
{
	if (!eventcast_client) {
		LOG_SEVERE("eventcast_client is null\n");
		return 0;
	}

	LOG_TRACE("connected port: %d\n", eventcast_client->port);
	return eventcast_client->port;
}

const char *get_eventcast_connected_ipaddr(void)
{
	if (!eventcast_client) {
		LOG_SEVERE("eventcast client is null\n");
		return NULL;
	}

	LOG_TRACE("connected ip address: %s\n", eventcast_client->ipaddress);
	return eventcast_client->ipaddress;
}

static void set_eventcast_connection_status(int status)
{
    if (!eventcast_client) {
        return;
    }

    qemu_mutex_lock(&eventcast_client->mutex);
    eventcast_client->status = status;
    qemu_mutex_unlock(&eventcast_client->mutex);

    send_eventcast_connection_status_ecp();
}

static void eventcast_notify_exit(Notifier *notifier, void *data)
{
    LOG_INFO("eventcast_notify_exit\n");
    disconnect_eventcast_app();
}
static Notifier eventcast_exit = { .notify = eventcast_notify_exit };

static void *initialize_eventcast_socket(void *opaque);

int connect_eventcast_app(const char *ipaddress, int port)
{
    EventcastState *client = NULL;
    int ipaddr_len = 0;

    client = g_malloc0(sizeof(EventcastState));
    if (!client) {
        return -1;
    }

    client->port = port;

    if (ipaddress) {
        ipaddr_len = strlen(ipaddress);
        client->type = WIFI;
    } else {
        ipaddr_len = strlen(loopback);
        ipaddress = loopback;
        client->type = USB;
    }

    client->ipaddress = g_malloc0(ipaddr_len + 1);
    if (!client->ipaddress) {
        LOG_SEVERE("failed to allocate ipaddress buffer\n");
        g_free(client);
        return -1;
    }
    g_strlcpy(client->ipaddress, ipaddress, ipaddr_len + 1);
    LOG_INFO("connection info. ip %s, port %d type %d\n",
        client->ipaddress, client->port, client->type);

    eventcast_client = client;

    qemu_mutex_init(&eventcast_client->mutex);
    qemu_thread_create(&eventcast_client->thread, "eventcast-io-thread",
            initialize_eventcast_socket, client,
            QEMU_THREAD_DETACHED);

    return 0;
}

int disconnect_eventcast_app(void)
{
    int sock = 0;

    LOG_INFO("disconnect app from ecp\n");
    if (!eventcast_client) {
        LOG_SEVERE("eventcast client instance is NULL\n");
        return -1;
    }

    sock = eventcast_client->fd;
    if (sock < 0) {
        LOG_SEVERE("eventcast socket is already terminated or not ready\n");
        return -1;
    } else {
        send_emul_state_msg();
        end_eventcast_socket(sock);
    }

    return 0;
}

static int eventcast_loop(int sockfd)
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
        eventcast_io_handler(socket);
    }

    return ret;
}

static void *initialize_eventcast_socket(void *opaque)
{
    EventcastState *client = (EventcastState *)opaque;
    LOG_TRACE("callback function for eventcast_thread\n");

    if (!client) {
        LOG_SEVERE("EventcastState is NULL\n");
        return NULL;
    }

    client->fd = start_eventcast_socket(client->ipaddress, client->port);
    if (client->fd < 0) {
        LOG_SEVERE("failed to start eventcast_socket\n");
        return NULL;
    }
    LOG_TRACE("eventcast_sock: %d\n", client->fd);

    reset_eventcast_recv_buf(&recv_buf);
    send_handshake_req_msg();

    emulator_add_exit_notifier(&eventcast_exit);

    while (1) {
        qemu_mutex_lock(&client->mutex);
        if (client->status == DISCONNECTED) {
            qemu_mutex_unlock(&client->mutex);
            LOG_INFO("disconnected socket. destroy this thread\n");
            break;
        }
        qemu_mutex_unlock(&client->mutex);

        eventcast_loop(client->fd);
    }

    return client;
}
