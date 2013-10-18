/*
 * Emulator Control Server
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  MunKyu Im       <munkyu.im@samsung.com>
 *  Daiyoung Kim    <daiyoung777.kim@samsung.com>
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

#ifndef __ECS_H__
#define __ECS_H__

#ifdef CONFIG_LINUX
#include <sys/epoll.h>
#endif

#include "qapi/qmp/qerror.h"
#include "qemu-common.h"
#include "ecs-json-streamer.h"
#include "genmsg/ecs.pb-c.h"
#include "genmsg/ecs_ids.pb-c.h"

#define ECS_VERSION   "1.0"

#define ECS_DEBUG   1

#ifdef ECS_DEBUG
#define LOG(fmt, arg...)    \
    do {    \
        fprintf(stdout,"[%s-%s:%d] "fmt"\n", __TIME__, __FUNCTION__, __LINE__, ##arg);  \
    } while (0)
#else
#define LOG(fmt, arg...)
#endif

#ifndef _WIN32
#define LOG_HOME                "HOME"
#define LOG_PATH                "/tizen-sdk-data/emulator/vms/ecs.log"
#else
#define LOG_HOME                "LOCALAPPDATA"
#define LOG_PATH                "\\tizen-sdk-data\\emulator\\vms\\ecs.log"
#endif

#define ECS_OPTS_NAME           "ecs"
#define HOST_LISTEN_ADDR        "127.0.0.1"
#define HOST_LISTEN_PORT        27000
#define EMULATOR_SERVER_NUM     10

#define COMMANDS_TYPE           "type"
#define COMMANDS_DATA           "data"


#define COMMAND_TYPE_INJECTOR   "injector"
#define COMMAND_TYPE_CONTROL    "control"
#define COMMAND_TYPE_MONITOR    "monitor"
#define COMMAND_TYPE_DEVICE     "device"

#define ECS_MSG_STARTINFO_REQ   "startinfo_req"
#define ECS_MSG_STARTINFO_ANS   "startinfo_ans"

#define MSG_TYPE_SENSOR         "sensor"
#define MSG_TYPE_NFC            "nfc"

#define MSG_GROUP_STATUS        15

#define MSG_ACTION_ACCEL        110
#define MSG_ACTION_GYRO         111
#define MSG_ACTION_MAG          112
#define MSG_ACTION_LIGHT        113
#define MSG_ACTION_PROXI        114

#define TIMER_ALIVE_S           60
#define TYPE_DATA_SELF          "self"

enum sensor_level {
    level_accel = 1,
    level_proxi = 2,
    level_light = 3,
    level_gyro = 4,
    level_geo = 5,
    level_tilt = 12,
    level_magnetic = 13
};

typedef unsigned short  type_length;
typedef unsigned char   type_group;
typedef unsigned char   type_action;

#define OUT_BUF_SIZE    4096
#define READ_BUF_LEN    4096

typedef struct sbuf
{
    int _netlen;
    int _use;
    char _buf[4096];
}sbuf;

struct Monitor {
    int suspend_cnt;
    uint8_t outbuf[OUT_BUF_SIZE];
    int outbuf_index;
    CPUArchState *mon_cpu;
    void *password_opaque;
    QError *error;
    QLIST_HEAD(,mon_fd_t) fds;
    QLIST_ENTRY(Monitor) entry;
};

#define MAX_EVENTS  1000
#define MAX_FD_NUM  300
typedef struct ECS_State {
    int listen_fd;
#ifdef CONFIG_LINUX
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
#else
    fd_set reads;
#endif
    int is_unix;
    int ecs_running;
    QEMUTimer *alive_timer;
    Monitor *mon;
} ECS_State;

typedef struct ECS_Client {
    int client_fd;
    int client_id;
    int keep_alive;
    const char* type;

    sbuf sbuf;

    ECS_State *cs;
    JSONMessageParser parser;
    QTAILQ_ENTRY(ECS_Client) next;
} ECS_Client;


int start_ecs(void);
int stop_ecs(void);
int get_ecs_port(void);

bool handle_protobuf_msg(ECS_Client* cli, char* data, const int len);

bool ntf_to_injector(const char* data, const int len);
bool ntf_to_control(const char* data, const int len);
bool ntf_to_monitor(const char* data, const int len);

bool send_to_ecp(ECS__Master* master);

bool send_injector_ntf(const char* data, const int len);
bool send_monitor_ntf(const char* data, const int len);
bool send_device_ntf(const char* data, const int len);
bool send_nfc_ntf(const char* data, const int len);

bool send_to_all_client(const char* data, const int len);
void send_to_client(int fd, const char* data, const int len) ;

void ecs_client_close(ECS_Client* clii);
int ecs_write(int fd, const uint8_t *buf, int len);
void ecs_make_header(QDict* obj, type_length length, type_group group, type_action action);

void read_val_short(const char* data, unsigned short* ret_val);
void read_val_char(const char* data, unsigned char* ret_val);
void read_val_str(const char* data, char* ret_val, int len);
void print_binary(const char* data, const int len);

bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg);
bool msgproc_monitor_req(ECS_Client *ccli, ECS__MonitorReq* msg);
bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg);
bool msgproc_nfc_req(ECS_Client* ccli, ECS__NfcReq* msg);
void msgproc_checkversion_req(ECS_Client* ccli, ECS__CheckVersionReq* msg);
void msgproc_keepalive_ans(ECS_Client* ccli, ECS__KeepAliveAns* msg);

/* version check  */
//void send_ecs_version_check(ECS_Client* ccli);

/* request */
int accel_min_max(double value);
void req_set_sensor_accel(int x, int y, int z);
void set_sensor_data(int length, const char* data);

/* Monitor */
void handle_ecs_command(JSONMessageParser *parser, QList *tokens, void *opaque);
void handle_qmp_command(JSONMessageParser *parser, QList *tokens, void *opaque);

static QemuOptsList qemu_ecs_opts = {
    .name = ECS_OPTS_NAME,
    .implied_opt_name = ECS_OPTS_NAME,
    .head = QTAILQ_HEAD_INITIALIZER(qemu_ecs_opts.head),
    .desc = {
        {
            .name = "host",
            .type = QEMU_OPT_STRING,
        },{
            .name = "port",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localaddr",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localport",
            .type = QEMU_OPT_STRING,
        },{
            .name = "to",
            .type = QEMU_OPT_NUMBER,
        },{
            .name = "ipv4",
            .type = QEMU_OPT_BOOL,
        },{
            .name = "ipv6",
            .type = QEMU_OPT_BOOL,
        },
        { /* End of list */ }
    },
};

#endif /* __ECS_H__ */
