
#include "hw/qdev.h"
#include "net.h"
#include "console.h"
#include "migration.h"

#ifndef _WIN32
#include <sys/epoll.h>
#endif

#include "qemu-common.h"
#include "qemu_socket.h"
#include "qemu-queue.h"
#include "qemu-option.h"
#include "main-loop.h"
#include "ui/qemu-spice.h"
#include "qemu-char.h"
#include "sdb.h"
#include "qjson.h"
#include "ecs-json-streamer.h"
#include "json-parser.h"
#include "qmp-commands.h"
#include "qint.h"
#include "qbool.h"
#include "ecs.h"
#include "hw/maru_virtio_evdi.h"
#include "skin/maruskin_operation.h"
#include <stdbool.h>
#include <pthread.h>


void ecs_startinfo_req(ECS_Client *clii)
{
	LOG("ecs_startinfo_req");

	int usbkbd_status = mloop_evcmd_get_hostkbd_status();

	LOG("usbkbd_status = %d", usbkbd_status);



	QDict* objData = qdict_new();
	qdict_put(objData, "host_keyboard_onoff", qint_from_int((int64_t )usbkbd_status));

	QDict* objMsg = qdict_new();
	qobject_incref(QOBJECT(objData));

	qdict_put(objMsg, "type", qstring_from_str(ECS_MSG_STARTINFO_ANS));
	qdict_put(objMsg, "result", qstring_from_str("success"));
	qdict_put(objMsg, "data", objData);

	QString *json;
	json = qobject_to_json(QOBJECT(objMsg));

	assert(json != NULL);

	qstring_append_chr(json, '\n');
	const char* snddata = qstring_get_str(json);

	LOG("<< startinfo json str = %s", snddata);

	send_to_client(clii->client_fd, snddata);

	QDECREF(json);
	QDECREF(objData);
	QDECREF(objMsg);
}

void control_host_keyboard_onoff_req(ECS_Client *clii, QDict* data)
{
	int64_t is_on = qdict_get_int(data, "is_on");
	onoff_host_kbd(is_on);
}

void host_keyboard_onoff_ntf(int is_on)
{
	QDict* objMsg = qdict_new();

	qdict_put(objMsg, "type", qstring_from_str("host_keyboard_onoff_ntf"));
	qdict_put(objMsg, "ison", qbool_from_int((int64_t)is_on));

    QString *json;
    json =  qobject_to_json(QOBJECT(objMsg));

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    const char* snddata = qstring_get_str(json);

    LOG("<< json str = %s", snddata);

	send_to_all_client(snddata, strlen(snddata));

	QDECREF(json);

	QDECREF(objMsg);
}
