#include <stdbool.h>
#include <pthread.h>

#include "hw/qdev.h"
#include "net/net.h"
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
#include "base64.h"
#include "mloop_event.h"
#include "hw/maru_virtio_evdi.h"
#include "hw/maru_virtio_sensor.h"
#include "hw/maru_virtio_nfc.h"
#include "skin/maruskin_operation.h"

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


// message handlers


bool msgproc_start_req(ECS_Client* ccli, ECS__StartReq* msg)
{
    LOG("ecs_startinfo_req");

    int hostkbd_status = mloop_evcmd_get_hostkbd_status();

    LOG("hostkbd_status = %d", hostkbd_status);

    send_start_ans(hostkbd_status);

    return true;
}

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

bool msgproc_control_msg(ECS_Client *cli, ECS__ControlMsg* msg)
{
    if (msg->type == ECS__CONTROL_MSG__CONTROL_TYPE__HOSTKEYBOARD_REQ)
    {
        ECS__HostKeyboardReq* hkr = msg->hostkeyboard_req;
        if (!hkr)
            return false;
        msgproc_control_hostkeyboard_req(cli, hkr);
    }

    return true;
}

bool msgproc_monitor_req(ECS_Client *ccli, ECS__MonitorReq* msg)
{

    return true;
}

bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    char cmd[10];
    char* data = NULL;
    memset(cmd, 0, 10);
    strcpy(cmd, msg->category);
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*)msg->data.data;
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
    }
    else if (!strncmp(cmd, MSG_TYPE_NFC, 3)) {
        if (group == MSG_GROUP_STATUS) {
            send_to_nfc(request_nfc_get, data, length);
        }
        else
        {
            send_to_nfc(request_nfc_set, data, length);
        }
    }

    return true;
}

bool msgproc_screen_dump_req(ECS_Client *ccli, ECS__ScreenDumpReq* msg)
{

    return true;
}


// begin control command

void msgproc_control_hostkeyboard_req(ECS_Client *clii, ECS__HostKeyboardReq* req)
{
    int64_t is_on = req->ison;
    onoff_host_kbd(is_on);
}

// end control command


//

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

    if(!strcmp(cat, "telephony")) {
        base64_encode(ijdata, length, &encoded_ijdata);
    }

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

bool send_start_ans(int host_keyboard_onff)
{
    ECS__Master master = ECS__MASTER__INIT;
    ECS__StartAns ans = ECS__START_ANS__INIT;

    ans.has_host_keyboard_onoff = 1;
    ans.host_keyboard_onoff = host_keyboard_onff;

    ans.has_camera_onoff = 1;
    ans.camera_onoff = 1;

    ans.has_earjack_onoff = 1;
    ans.earjack_onoff = 1;

    master.type = ECS__MASTER__TYPE__START_ANS;
    master.start_ans = &ans;

    return send_to_ecp(&master);
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


bool send_hostkeyboard_ntf(int is_on)
{
    ECS__Master master = ECS__MASTER__INIT;
    ECS__ControlMsg ctl = ECS__CONTROL_MSG__INIT;

    ECS__HostKeyboardNtf ntf = ECS__HOST_KEYBOARD_NTF__INIT;

    ntf.has_ison = 1;
    ntf.ison = is_on;

    ctl.type = ECS__CONTROL_MSG__CONTROL_TYPE__HOSTKEYBOARD_NTF;
    ctl.hostkeyboard_ntf = &ntf;

    master.type = ECS__MASTER__TYPE__CONTROL_MSG;
    master.control_msg = &ctl;

    return send_to_ecp(&master);
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

