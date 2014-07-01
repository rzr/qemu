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
#include <glib.h>

#ifdef CONFIG_LINUX
#include <sys/epoll.h>
#endif

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
#include "qemu-common.h"
#include "util/sdb.h"
#include "ecs-json-streamer.h"
#include "qmp-commands.h"

#include "ecs.h"
#ifndef CONFIG_USE_SHM
#include "display/maru_finger.h"
#endif

#include "hw/virtio/maru_virtio_evdi.h"
#include "hw/virtio/maru_virtio_sensor.h"
#include "hw/virtio/maru_virtio_jack.h"
#include "hw/virtio/maru_virtio_power.h"
#include "hw/virtio/maru_virtio_nfc.h"
#include "hw/virtio/maru_virtio_vmodem.h"
#include "skin/maruskin_operation.h"
#include "skin/maruskin_server.h"
#include "util/maru_device_hotplug.h"
#include "emulator.h"
#include "emul_state.h"

#include "debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, ecs-msg);

// utility functions
static int guest_connection = 0;
extern QemuMutex mutex_guest_connection;

/*static function define*/
static void handle_sdcard(char* dataBuf, size_t dataLen);
static char* get_emulator_sdcard_path(void);
static char *get_old_tizen_sdk_data_path(void);

static void* build_master(ECS__Master* master, int* payloadsize)
{
    int len_pack = ecs__master__get_packed_size(master);
    *payloadsize = len_pack + 4;
    TRACE("pack size=%d\n", len_pack);
    void* buf = g_malloc(len_pack + 4);
    if (!buf)
        return NULL;

    ecs__master__pack(master, buf + 4);

    len_pack = htonl(len_pack);
    memcpy(buf, &len_pack, 4);

    return buf;
}

static bool send_single_msg(ECS__Master* master, ECS_Client *clii)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        ERR("invalid buf\n");
        return false;
    }

    send_to_single_client(clii, buf, payloadsize);

    if (buf)
    {
        g_free(buf);
    }
    return true;
}

bool send_to_ecp(ECS__Master* master)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        ERR("invalid buf\n");
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
        ERR("invalid buf\n");
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

static void send_status_injector_ntf(const char* cmd, int cmdlen, int act, char* on)
{
    int msglen = 0, datalen = 0;
    type_length length  = 0;
    type_group group = GROUP_STATUS;
    type_action action = act;

    if (cmd == NULL || cmdlen > 10)
        return;

    if (on == NULL) {
        msglen = 14;
    } else {
        datalen = strlen(on);
        length  = (unsigned short)datalen;

        msglen = datalen + 15;
    }

    char* status_msg = (char*) malloc(msglen);
    if(!status_msg)
        return;

    memset(status_msg, 0, msglen);

    memcpy(status_msg, cmd, cmdlen);
    memcpy(status_msg + 10, &length, sizeof(unsigned short));
    memcpy(status_msg + 12, &group, sizeof(unsigned char));
    memcpy(status_msg + 13, &action, sizeof(unsigned char));

    if (on != NULL) {
        memcpy(status_msg + 14, on, datalen);
    }

    send_injector_ntf(status_msg, msglen);

    if (status_msg)
        free(status_msg);
}

static void msgproc_injector_ans(ECS_Client* ccli, const char* category, bool succeed)
{
    if (ccli == NULL) {
        return;
    }
    int catlen = 0;
    ECS__Master master = ECS__MASTER__INIT;
    ECS__InjectorAns ans = ECS__INJECTOR_ANS__INIT;

    TRACE("injector ans - category : %s, succed : %d\n", category, succeed);

    catlen = strlen(category);
    ans.category = (char*) g_malloc0(catlen + 1);
    memcpy(ans.category, category, catlen);

    ans.errcode = !succeed;
    master.type = ECS__MASTER__TYPE__INJECTOR_ANS;
    master.injector_ans = &ans;

    send_single_msg(&master, ccli);

    if (ans.category)
        g_free(ans.category);
}

static void msgproc_device_ans(ECS_Client* ccli, const char* category, bool succeed, char* data)
{
    if (ccli == NULL) {
        return;
    }
    int catlen = 0;
    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceAns ans = ECS__DEVICE_ANS__INIT;

    TRACE("device ans - category : %s, succed : %d\n", category, succeed);

    catlen = strlen(category);
    ans.category = (char*) g_malloc0(catlen + 1);
    memcpy(ans.category, category, catlen);

    ans.errcode = !succeed;

    if (data != NULL) {
        ans.length = strlen(data);

        if (ans.length > 0) {
            ans.has_data = 1;
            ans.data.data = g_malloc(ans.length);
            ans.data.len = ans.length;
            memcpy(ans.data.data, data, ans.length);
            TRACE("data = %s, length = %hu\n", data, ans.length);
        }
    }

    master.type = ECS__MASTER__TYPE__DEVICE_ANS;
    master.device_ans = &ans;

    send_single_msg(&master, ccli);

    if (ans.category)
        g_free(ans.category);
}

bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg)
{
    char cmd[10];
    char data[10];
    bool ret = false;
    int sndlen = 0;
    int value = 0;
    char* sndbuf;
    memset(cmd, 0, 10);
    strncpy(cmd, msg->category, sizeof(cmd) -1);
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);


    int datalen = 0;
    if (msg->has_data)
    {
        datalen = msg->data.len;
    }
    //TRACE(">> count= %d", ++ijcount);

    TRACE(">> header = cmd = %s, length = %d, action=%d, group=%d\n", cmd, length,
            action, group);

    /*SD CARD msg process*/
    if (!strncmp(cmd, MSG_TYPE_SDCARD, strlen(MSG_TYPE_SDCARD))) {
        if (msg->has_data) {
            TRACE("msg(%zu) : %s\n", msg->data.len, msg->data.data);
            handle_sdcard((char*) msg->data.data, msg->data.len);

        } else {
            ERR("has no msg\n");
        }

    } else if (!strncmp(cmd, MSG_TYPE_SENSOR, sizeof(MSG_TYPE_SENSOR))) {
        if (group == MSG_GROUP_STATUS) {
            memset(data, 0, 10);
            if (action == MSG_ACT_BATTERY_LEVEL) {
                sprintf(data, "%d", get_power_capacity());
            } else if (action == MSG_ACT_BATTERY_CHARGER) {
                sprintf(data, "%d", get_jack_charger());
            } else {
                goto injector_send;
            }
            TRACE("status : %s", data);
            send_status_injector_ntf(MSG_TYPE_SENSOR, 6, action, data);
            msgproc_injector_ans(ccli, cmd, true);
            return true;
        } else {
            if (msg->data.data && datalen > 0) {
                set_injector_data((char*) msg->data.data);
            }
        }
    } else if (!strncmp(cmd, MSG_TYPE_GUEST, 5)) {
        qemu_mutex_lock(&mutex_guest_connection);
        value = guest_connection;
        qemu_mutex_unlock(&mutex_guest_connection);
        send_status_injector_ntf(MSG_TYPE_GUEST, 5, value, NULL);
        return true;
    }

injector_send:
    sndlen = datalen + 14;
    sndbuf = (char*) g_malloc(sndlen + 1);
    if (!sndbuf) {
        msgproc_injector_ans(ccli, cmd, ret);
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
            TRACE(">> print len = %zd, data\" %s\"\n", strlen(data), data);
        }
    }

    if(strcmp(cmd, "telephony") == 0) {
       TRACE("telephony msg >>");
       ret = send_to_vmodem(route_ij, sndbuf, sndlen);
    } else {
       TRACE("evdi msg >> %s", cmd);
       ret = send_to_evdi(route_ij, sndbuf, sndlen);
    }


    g_free(sndbuf);

    msgproc_injector_ans(ccli, cmd, ret);

    if (!ret) {
        return false;
    }

    return true;
}

void ecs_suspend_lock_state(int state)
{
    int catlen;

    ECS__InjectorReq msg = ECS__INJECTOR_REQ__INIT;
    const char* category = "suspend";

    catlen = strlen(category);
    msg.category = (char*) g_malloc0(catlen + 1);
    memcpy(msg.category, category, catlen);

    msg.group = 5;
    msg.action = state;

    msgproc_injector_req(NULL, &msg);
}


void msgproc_keepalive_ans (ECS_Client* ccli, ECS__KeepAliveAns* msg)
{
    ccli->keep_alive = 0;
}

void send_host_keyboard_ntf (int on)
{
    type_length length = (unsigned short)1;
    type_group group = GROUP_STATUS;
    type_action action = 122;

    char* keyboard_msg = (char*) malloc(15);
    if(!keyboard_msg)
        return;

    memcpy(keyboard_msg, "HKeyboard", 10);
    memcpy(keyboard_msg + 10, &length, sizeof(unsigned short));
    memcpy(keyboard_msg + 12, &group, sizeof(unsigned char));
    memcpy(keyboard_msg + 13, &action, sizeof(unsigned char));
    memcpy(keyboard_msg + 14, ((on == 1) ? "1":"0"), 1);

    send_device_ntf(keyboard_msg, 15);

    if (keyboard_msg)
        free(keyboard_msg);
}

extern char tizen_target_img_path[];
void send_target_image_information(ECS_Client* ccli) {
    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceAns ans = ECS__DEVICE_ANS__INIT;
    int length = strlen(tizen_target_img_path); // ??

    ans.category = (char*) g_malloc(10 + 1);
    strncpy(ans.category, "info", 10);

    ans.errcode = 0;
    ans.length = length;
    ans.group = 1;
    ans.action = 1;

    if (length > 0)
    {
        ans.has_data = 1;

        ans.data.data = g_malloc(length);
        ans.data.len = length;
        memcpy(ans.data.data, tizen_target_img_path, length);

        TRACE("data = %s, length = %hu\n", tizen_target_img_path, length);
    }

    master.type = ECS__MASTER__TYPE__DEVICE_ANS;
    master.device_ans = &ans;

    send_single_msg(&master, ccli);

    if (ans.data.len > 0)
    {
        g_free(ans.data.data);
    }

    g_free(ans.category);

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

    TRACE(">> header = cmd = %s, length = %d, action=%d, group=%d\n", cmd, length,
            action, group);

    if (!strncmp(cmd, MSG_TYPE_SENSOR, 6)) {
        if (group == MSG_GROUP_STATUS) {
            if (action == MSG_ACT_ACCEL) {
                get_sensor_accel();
            } else if (action == MSG_ACT_GYRO) {
                get_sensor_gyro();
            } else if (action == MSG_ACT_MAG) {
                get_sensor_mag();
            } else if (action == MSG_ACT_LIGHT) {
                get_sensor_light();
            } else if (action == MSG_ACT_PROXI) {
                get_sensor_proxi();
            }
        } else {
            if (data != NULL) {
                set_injector_data(data);
            } else {
                ERR("sensor set data is null\n");
            }
        }
        msgproc_device_ans(ccli, cmd, true, NULL);
    } else if (!strncmp(cmd, "Network", 7)) {
        if (data != NULL) {
            TRACE(">>> Network msg: '%s'\n", data);
            if(net_slirp_redir(data) < 0) {
                ERR( "redirect [%s] fail\n", data);
            } else {
                TRACE("redirect [%s] success\n", data);
            }
        } else {
            ERR("Network redirection data is null.\n");
        }
    } else if (!strncmp(cmd, "HKeyboard", 8)) {
        if (group == MSG_GROUP_STATUS) {
            send_host_keyboard_ntf(is_host_keyboard_attached());
        } else {
            if (data == NULL) {
                ERR("HKeyboard data is NULL\n");
                return false;
            }

            if (!strncmp(data, "1", 1)) {
                is_on = 1;
            }
            do_host_kbd_enable(is_on);
            notify_host_kbd_state(is_on);
        }
    } else if (!strncmp(cmd, "TGesture", strlen("TGesture"))) {
        /* release multi-touch */
#ifndef CONFIG_USE_SHM
        if (get_multi_touch_enable() != 0) {
            clear_finger_slot(false);
        }
#else
        // TODO:
#endif

        if (data == NULL) {
            ERR("touch gesture data is NULL\n");
            return false;
        }

        TRACE("%s\n", data);

        char token[] = "#";

        if (group == 1) { /* HW key event */
            char *section = strtok(data, token);
            int event_type = atoi(section);

            section = strtok(NULL, token);
            int keycode = atoi(section);

            do_hw_key_event(event_type, keycode);
        } else { /* touch event */
            char *section = strtok(data, token);
            int event_type = atoi(section);

            section = strtok(NULL, token);
            int xx = atoi(section);

            section = strtok(NULL, token);
            int yy = atoi(section);

            section = strtok(NULL, token);
            int zz = atoi(section);

            do_mouse_event(1/* LEFT */, event_type, 0, 0, xx, yy, zz);
        }
    } else if (!strncmp(cmd, "info", 4)) {
        // check to emulator target image path
        TRACE("receive info message %s\n", tizen_target_img_path);
        send_target_image_information(ccli);

    } else if (!strncmp(cmd, "input", strlen("input"))) {
        // cli input
        TRACE("receive input message [%s]\n", data);

        if (group == 0) {

            TRACE("input keycode data : [%s]\n", data);

            char token[] = " ";
            char *section = strtok(data, token);
            int keycode = atoi(section);
            if (action == 1) {
                //action 1 press
                do_hw_key_event(KEY_PRESSED, keycode);

            } else if (action == 2) {
                //action 2 released
                do_hw_key_event(KEY_RELEASED, keycode);

            } else {
                ERR("unknown action : [%d]\n", (int)action);
            }
        } else if (group == 1) {
            //spec out
            TRACE("input category's group 1 is spec out\n");
        } else {
            ERR("unknown group [%d]\n", (int)group);
        }
        msgproc_device_ans(ccli, cmd, true, NULL);

    } else if (!strncmp(cmd, "vmname", strlen("vmname"))) {
        char* vmname = get_emul_vm_name();
        msgproc_device_ans(ccli, cmd, true, vmname);
    } else if (!strncmp(cmd, "nfc", strlen("nfc"))) {
        if (group == MSG_GROUP_STATUS) {
            //TODO:
            INFO("get nfc data: do nothing\n");
        } else {
            if (data != NULL) {
                send_to_nfc(ccli->client_id, ccli->client_type, data, msg->data.len);
            } else {
                ERR("nfc data is null\n");
            }
        }
    } else {
        ERR("unknown cmd [%s]\n", cmd);
    }

    if (data) {
        g_free(data);
    }

    return true;
}

bool msgproc_nfc_req(ECS_Client* ccli, ECS__NfcReq* msg)
{
    int datalen = msg->data.len;
    void* data = (void*)g_malloc(datalen);
    if(!data) {
        ERR("g_malloc failed!\n");
        return false;
    }

    memset(data, 0, datalen);
    memcpy(data, msg->data.data, msg->data.len);

    if (msg->has_data && msg->data.len > 0)
    {
        TRACE("recv from nfc injector: %s, %z", msg->has_data, msg->data.len);
        print_binary(data, datalen);
    }

    send_to_nfc(ccli->client_id, ccli->client_type, data, msg->data.len);
    g_free(data);
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

    const char *encoded_ijdata = "";
    TRACE("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,
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

    TRACE("<< json str = %s", snddata);

    send_to_all_client(snddata, strlen(snddata));

    QDECREF(json);

    QDECREF(obj_header);
    QDECREF(objData);
    QDECREF(objMsg);

    return true;
}

static bool injector_req_handle(const char* cat, type_action action)
{
    /*SD CARD msg process*/
    if (!strncmp(cat, MSG_TYPE_SDCARD, strlen(MSG_TYPE_SDCARD))) {
       return false;

    } else if (!strncmp(cat, "suspend", 7)) {
        ecs_suspend_lock_state(ecs_get_suspend_state());
        return true;
    } else if (!strncmp(cat, MSG_TYPE_GUEST, 5)) {
        INFO("emuld connection is %d\n", action);
        qemu_mutex_lock(&mutex_guest_connection);
        guest_connection = action;
        qemu_mutex_unlock(&mutex_guest_connection);
        return false;
    }

    return false;
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

    if (injector_req_handle(cat, action)) {
        return true;
    }

    const char* ijdata = (data + catsize + 2 + 1 + 1);

    TRACE("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,action, group);

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

    if (ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

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

    TRACE("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,action, group);

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

        TRACE("data = %s, length = %hu", ijdata, length);
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

bool send_nfc_ntf(struct nfc_msg_info* msg)
{
    const int catsize = 10;
    char cat[catsize + 1];
    ECS_Client *clii;
    memset(cat, 0, catsize + 1);

    print_binary((char*)msg->buf, msg->use);
    TRACE("id: %02x, type: %02x, use: %d", msg->client_id, msg->client_type, msg->use);
    clii =  find_client(msg->client_id, msg->client_type);
    if (clii) {
        if(clii->client_type == TYPE_SIMUL_NFC) {
            strncpy(cat, MSG_TYPE_NFC, 3);
        } else if (clii->client_type == TYPE_ECP) {
            strncpy(cat, MSG_TYPE_SIMUL_NFC, 9);
        }else {
            ERR("cannot find type! : %d", clii->client_type);
        }
        TRACE("header category = %s", cat);
    }
    else {
        ERR("cannot find client!\n");
    }

    ECS__Master master = ECS__MASTER__INIT;
    ECS__NfcNtf ntf = ECS__NFC_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);

    ntf.has_data = 1;

    ntf.data.data = g_malloc(NFC_MAX_BUF_SIZE);
    ntf.data.len = NFC_MAX_BUF_SIZE;
    memcpy(ntf.data.data, msg->buf, NFC_MAX_BUF_SIZE);

    printf("send to nfc injector: ");
    master.type = ECS__MASTER__TYPE__NFC_NTF;
    master.nfc_ntf = &ntf;

    send_single_msg(&master, clii);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}


static void handle_sdcard(char* dataBuf, size_t dataLen)
{

    char ret = 0;

    if (dataBuf != NULL){
        ret = dataBuf[0];

        if (ret == '0' ) {
            /* umount sdcard */
            do_hotplug(DETACH_SDCARD, NULL, 0);
        } else if (ret == '1') {
            /* mount sdcard */
            char sdcard_img_path[256];
            char* sdcard_path = NULL;

            sdcard_path = get_emulator_sdcard_path();
            if (sdcard_path) {
                g_strlcpy(sdcard_img_path, sdcard_path,
                        sizeof(sdcard_img_path));

                /* emulator_sdcard_img_path + sdcard img name */
                char* sdcard_img_name = dataBuf+2;
                if(dataLen > 3 && sdcard_img_name != NULL){
                    char* pLinechange = strchr(sdcard_img_name, '\n');
                    if(pLinechange != NULL){
                        sdcard_img_name = g_strndup(sdcard_img_name, pLinechange - sdcard_img_name);
                    }

                    g_strlcat(sdcard_img_path, sdcard_img_name, sizeof(sdcard_img_path));
                    TRACE("sdcard path: [%s]\n", sdcard_img_path);

                    do_hotplug(ATTACH_SDCARD, sdcard_img_path, strlen(sdcard_img_path) + 1);

                    /*if using strndup than free string*/
                    if(pLinechange != NULL && sdcard_img_name!= NULL){
                        free(sdcard_img_name);
                    }

                }

                g_free(sdcard_path);
            } else {
                ERR("failed to get sdcard path!!\n");
            }
        } else if(ret == '2'){
            TRACE("sdcard status 2 bypass" );
        }else {
            ERR("!!! unknown command : %c\n", ret);
        }

    }else{
        ERR("!!! unknown data : %c\n", ret);
    }
}

static char* get_emulator_sdcard_path(void)
{
    char *emulator_sdcard_path = NULL;
    char *tizen_sdk_data = NULL;

#ifndef CONFIG_WIN32
    char emulator_sdcard[] = "/emulator/sdcard/";
#else
    char emulator_sdcard[] = "\\emulator\\sdcard\\";
#endif

    TRACE("emulator_sdcard: %s, %zu\n", emulator_sdcard, sizeof(emulator_sdcard));

    tizen_sdk_data = get_tizen_sdk_data_path();
    if (!tizen_sdk_data) {
        ERR("failed to get tizen-sdk-data path.\n");
        return NULL;
    }

    emulator_sdcard_path =
        g_malloc(strlen(tizen_sdk_data) + sizeof(emulator_sdcard) + 1);
    if (!emulator_sdcard_path) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }

    g_snprintf(emulator_sdcard_path, strlen(tizen_sdk_data) + sizeof(emulator_sdcard),
             "%s%s", tizen_sdk_data, emulator_sdcard);

    g_free(tizen_sdk_data);

    TRACE("sdcard path: %s\n", emulator_sdcard_path);
    return emulator_sdcard_path;
}

/*
 *  get tizen-sdk-data path from sdk.info.
 */
char *get_tizen_sdk_data_path(void)
{
    char *emul_bin_path = NULL;
    char *sdk_info_file_path = NULL;
    char *tizen_sdk_data_path = NULL;
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
        ERR("failed to get emulator path.\n");
        return NULL;
    }

    sdk_info_path_len = strlen(emul_bin_path) + strlen(sdk_info) + 1;
    sdk_info_file_path = g_malloc(sdk_info_path_len);
    if (!sdk_info_file_path) {
        ERR("failed to allocate sdk-data buffer.\n");
        return NULL;
    }

    g_snprintf(sdk_info_file_path, sdk_info_path_len, "%s%s",
                emul_bin_path, sdk_info);
    INFO("sdk.info path: %s\n", sdk_info_file_path);

    sdk_info_fp = fopen(sdk_info_file_path, "r");
    g_free(sdk_info_file_path);

    if (sdk_info_fp) {
        TRACE("Succeeded to open [sdk.info].\n");

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

            fclose(sdk_info_fp);
            return tizen_sdk_data_path;
        }

        fclose(sdk_info_fp);
    }

    // legacy mode
    ERR("Failed to open [sdk.info].\n");

    return get_old_tizen_sdk_data_path();
}

static char *get_old_tizen_sdk_data_path(void)
{
    char *tizen_sdk_data_path = NULL;

    INFO("try to search tizen-sdk-data path in another way.\n");

#ifndef CONFIG_WIN32
    char tizen_sdk_data[] = "/tizen-sdk-data";
    int tizen_sdk_data_len = 0;
    char *home_dir;

    home_dir = (char *)g_getenv("HOME");
    if (!home_dir) {
        home_dir = (char *)g_get_home_dir();
    }

    tizen_sdk_data_len = strlen(home_dir) + sizeof(tizen_sdk_data) + 1;
    tizen_sdk_data_path = g_malloc(tizen_sdk_data_len);
    if (!tizen_sdk_data_path) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }
    g_strlcpy(tizen_sdk_data_path, home_dir, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);

#else
    char tizen_sdk_data[] = "\\tizen-sdk-data\\";
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
        ERR("failed to allocate memory.\n");
        return NULL;
    }

    g_strlcpy(tizen_sdk_data_path, strLocalAppDataPath, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);
#endif

    INFO("tizen-sdk-data path: %s\n", tizen_sdk_data_path);
    return tizen_sdk_data_path;
}

