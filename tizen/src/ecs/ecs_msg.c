/* Emulator Control Server
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

#include <stdbool.h>
#include <pthread.h>

#include "hw/qdev.h"
#include "net/net.h"
#include "net/slirp.h"
#include "ui/console.h"
#include "migration/migration.h"
#include "qapi/qmp/qint.h"
#include "qapi/qmp/qbool.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/json-parser.h"
#include "ui/qemu-spice.h"
#include "qemu/queue.h"
#include "qemu/option.h"
#include "sysemu/char.h"
#include "qemu/main-loop.h"

#ifdef CONFIG_LINUX
#include <sys/epoll.h>
#endif

#include "qemu-common.h"
//#include "qemu_socket.h"
#include "sdb.h"
#include "ecs-json-streamer.h"
#include "qmp-commands.h"

#include "ecs.h"
#include "mloop_event.h"
#include "hw/maru_virtio_evdi.h"
#include "hw/maru_virtio_sensor.h"
#include "hw/maru_virtio_nfc.h"
#include "skin/maruskin_operation.h"

#define MAX_BUF_SIZE  255
// utility functions

static void* build_master(ECS__Master* master, int* payloadsize)
{
    int len_pack = ecs__master__get_packed_size(master);
    *payloadsize = len_pack + 4;
    LOG("pack size=%d", len_pack);
    void* buf = g_malloc(len_pack + 4);
    if (!buf)
        return NULL;

    ecs__master__pack(master, buf + 4);

    len_pack = htonl(len_pack);
    memcpy(buf, &len_pack, 4);

    return buf;
}

bool send_to_ecp(ECS__Master* master)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        LOG("invalid buf");
        return false;
    }

    if (!send_to_all_client(buf, payloadsize))
        return false;

    if (buf)
    {
        g_free(buf);
    }
    return true;
}
#if 0
static bool send_to_single_client(ECS__Master* master, ECS_Client *ccli)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        LOG("invalid buf");
        return false;
    }

    if (!send_to_client(ccli->client_fd, buf, payloadsize))
        return false;

    if (buf)
    {
        g_free(buf);
    }
    return true;
}
#endif
bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg)
{
    char cmd[10];
    memset(cmd, 0, 10);
    strcpy(cmd, msg->category);
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);


    int datalen = 0;
    if (msg->has_data)
    {
        datalen = msg->data.len;
    }
    //LOG(">> count= %d", ++ijcount);

    LOG(">> header = cmd = %s, length = %d, action=%d, group=%d", cmd, length,
            action, group);


    int sndlen = datalen + 14;
    char* sndbuf = (char*) g_malloc(sndlen + 1);
    if (!sndbuf) {
        return false;
    }

    memset(sndbuf, 0, sndlen + 1);

    // set data
    memcpy(sndbuf, cmd, 10);
    memcpy(sndbuf + 10, &length, 2);
    memcpy(sndbuf + 12, &group, 1);
    memcpy(sndbuf + 13, &action, 1);


    if (msg->has_data)
    {
        if (msg->data.data && msg->data.len > 0)
        {
            const char* data = (const char*)msg->data.data;
            memcpy(sndbuf + 14, data, datalen);
            LOG(">> print len = %zd, data\" %s\"", strlen(data), data);
        }
    }

    send_to_evdi(route_ij, sndbuf, sndlen);

    g_free(sndbuf);

    return true;
}
#if 0
void msgproc_checkversion_req(ECS_Client* ccli, ECS__CheckVersionReq* msg)
{
    int datalen = 0;
    if (msg->has_data)
    {
        datalen = msg->data.len;
        if (msg->data.data && msg->data.len > 0)
        {
            const char* data = (const char*)msg->data.data;
            memcpy(sndbuf + 14, data, datalen);
            LOG(">> print len = %zd, data\" %s\"", strlen(data), data);
        }
    }
}

void send_ecs_version_check(ECS_Client *ccli)
{
    int len_pack = 0;
    int payloadsize = 0;
    const char* ecs_version = ECS_VERSION;

    ECS__Master master = ECS__MASTER__INIT;
    ECS__CheckVersionReq req = ECS__CHECK_VERSION_REQ__INIT;

    req.version_str = (char*) g_malloc(10);

    strncpy(req.version_str, ecs_version, strlen(ecs_version));
    LOG("ecs version: %s", req.version_str);

    master.type = ECS__MASTER__TYPE__CHECKVERSION_REQ;
    master.checkversion_req = &req;

    send_to_single_client(master, ccli);
}
#endif
void msgproc_keepalive_ans (ECS_Client* ccli, ECS__KeepAliveAns* msg)
{
    ccli->keep_alive = 0;
}

static void send_host_keyboard_ntf (void)
{
    type_length length = (unsigned short)1;
    type_group group = GROUP_STATUS;
    type_action action = 122;
    int is_on = mloop_evcmd_get_hostkbd_status();

    char* keyboard_msg = (char*) malloc(15);
    if(!keyboard_msg)
        return;

    memcpy(keyboard_msg, "HKeyboard", 10);
    memcpy(keyboard_msg + 10, &length, sizeof(unsigned short));
    memcpy(keyboard_msg + 12, &group, sizeof(unsigned char));
    memcpy(keyboard_msg + 13, &action, sizeof(unsigned char));
    memcpy(keyboard_msg + 14, (is_on?"1":"0"), 1);

    send_device_ntf(keyboard_msg, 15);

    if (keyboard_msg)
        free(keyboard_msg);
}

bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    int is_on = 0;
    char cmd[10];
    char* data = NULL;
    memset(cmd, 0, 10);
    strcpy(cmd, msg->category);
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    LOG(">> header = cmd = %s, length = %d, action=%d, group=%d", cmd, length,
            action, group);

    if (!strncmp(cmd, MSG_TYPE_SENSOR, 6)) {
        if (group == MSG_GROUP_STATUS) {
            if (action ==MSG_ACTION_ACCEL) {
                get_sensor_accel();
            } else if (action == MSG_ACTION_GYRO) {
                get_sensor_gyro();
            } else if (action == MSG_ACTION_MAG) {
                get_sensor_mag();
            } else if (action == MSG_ACTION_LIGHT) {
                get_sensor_light();
            } else if (action == MSG_ACTION_PROXI) {
                get_sensor_proxi();
            }
        } else {
            set_sensor_data(length, data);
        }
    } else if (!strncmp(cmd, "Network", 7)) {
        LOG(">>> Network msg: '%s'", data);
        if(net_slirp_redir(data) < 0) {
            LOG( "redirect [%s] fail \n", data);
        } else {
            LOG("redirect [%s] success\n", data);
        }
    } else if (!strncmp(cmd, "HKeyboard", 8)) {
        if (group == MSG_GROUP_STATUS) {
            send_host_keyboard_ntf();
        } else {
            if (!strncmp(data, "1", 1)) {
                is_on = 1;
            }
            onoff_host_kbd(is_on);
        }
    } else if (!strncmp(cmd, MSG_TYPE_NFC, 3)) {
        if (group == MSG_GROUP_STATUS) {
            send_to_nfc(request_nfc_get, data, length);
        }
        else
        {
            send_to_nfc(request_nfc_set, data, length);
        }
    }

    if (data)
        g_free(data);

    return true;
}

bool msgproc_nfc_req(ECS_Client* ccli, ECS__NfcReq* msg)
{
    char cmd[10];
    char* data = NULL;
    memset(cmd, 0, 10);
    strcpy(cmd, msg->category);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*)msg->data.data;
        printf("recv from nfc injector: ");
        print_binary(data, data[1] + 2);
    }

    if (!strncmp(cmd, MSG_TYPE_NFC, 3)) {
        send_to_nfc(request_nfc_set, data, msg->data.len);
    }

    return true;
}

bool ntf_to_injector(const char* data, const int len) {
    type_length length = 0;
    type_group group = 0;
    type_action action = 0;

    const int catsize = 10;
    char cat[catsize + 1];
    memset(cat, 0, catsize + 1);

    read_val_str(data, cat, catsize);
    read_val_short(data + catsize, &length);
    read_val_char(data + catsize + 2, &group);
    read_val_char(data + catsize + 2 + 1, &action);


    const char* ijdata = (data + catsize + 2 + 1 + 1);

    char *encoded_ijdata = NULL;
     LOG("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,
            action, group);

    QDict* obj_header = qdict_new();
    ecs_make_header(obj_header, length, group, action);

    QDict* objData = qdict_new();
    qobject_incref(QOBJECT(obj_header));

    qdict_put(objData, "cat", qstring_from_str(cat));
    qdict_put(objData, "header", obj_header);
    if(!strcmp(cat, "telephony")) {
        qdict_put(objData, "ijdata", qstring_from_str(encoded_ijdata));
    } else {
        qdict_put(objData, "ijdata", qstring_from_str(ijdata));
    }

    QDict* objMsg = qdict_new();
    qobject_incref(QOBJECT(objData));

    qdict_put(objMsg, "type", qstring_from_str("injector"));
    qdict_put(objMsg, "result", qstring_from_str("success"));
    qdict_put(objMsg, "data", objData);

    QString *json;
    json = qobject_to_json(QOBJECT(objMsg));

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    const char* snddata = qstring_get_str(json);

    LOG("<< json str = %s", snddata);

    send_to_all_client(snddata, strlen(snddata));

    QDECREF(json);

    QDECREF(obj_header);
    QDECREF(objData);
    QDECREF(objMsg);

    return true;
}

bool send_injector_ntf(const char* data, const int len)
{
    type_length length = 0;
    type_group group = 0;
    type_action action = 0;

    const int catsize = 10;
    char cat[catsize + 1];
    memset(cat, 0, catsize + 1);

    read_val_str(data, cat, catsize);
    read_val_short(data + catsize, &length);
    read_val_char(data + catsize + 2, &group);
    read_val_char(data + catsize + 2 + 1, &action);


    const char* ijdata = (data + catsize + 2 + 1 + 1);

    LOG("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__InjectorNtf ntf = ECS__INJECTOR_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);


    ntf.length = length;
    ntf.group = group;
    ntf.action = action;

    if (length > 0)
    {
        ntf.has_data = 1;

        ntf.data.data = g_malloc(length);
        ntf.data.len = length;
        memcpy(ntf.data.data, ijdata, length);
    }

    master.type = ECS__MASTER__TYPE__INJECTOR_NTF;
    master.injector_ntf = &ntf;

    send_to_ecp(&master);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}

bool send_device_ntf(const char* data, const int len)
{
    type_length length = 0;
    type_group group = 0;
    type_action action = 0;

    const int catsize = 10;
    char cat[catsize + 1];
    memset(cat, 0, catsize + 1);

    read_val_str(data, cat, catsize);
    read_val_short(data + catsize, &length);
    read_val_char(data + catsize + 2, &group);
    read_val_char(data + catsize + 2 + 1, &action);

    const char* ijdata = (data + catsize + 2 + 1 + 1);

    LOG("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceNtf ntf = ECS__DEVICE_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);


    ntf.length = length;
    ntf.group = group;
    ntf.action = action;

    if (length > 0)
    {
        ntf.has_data = 1;

        ntf.data.data = g_malloc(length);
        ntf.data.len = length;
        memcpy(ntf.data.data, ijdata, length);

        LOG("data = %s, length = %hu", ijdata, length);
    }

    master.type = ECS__MASTER__TYPE__DEVICE_NTF;
    master.device_ntf = &ntf;

    send_to_ecp(&master);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}

bool send_nfc_ntf(const char* data, const int len)
{
    const int catsize = 10;
    char cat[catsize + 1];
    memset(cat, 0, catsize + 1);

    read_val_str(data, cat, catsize);

    const char* ijdata = data + catsize;

    LOG("header category = %s", cat);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__NfcNtf ntf = ECS__NFC_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);

    ntf.has_data = 1;

    ntf.data.data = g_malloc(MAX_BUF_SIZE);
    ntf.data.len = MAX_BUF_SIZE;
    memcpy(ntf.data.data, ijdata, MAX_BUF_SIZE);

    LOG("length = %d", len);
    printf("send to nfc injector: ");
    print_binary(ijdata, ijdata[1] + 2);
    master.type = ECS__MASTER__TYPE__NFC_NTF;
    master.nfc_ntf = &ntf;

    send_to_ecp(&master);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}

