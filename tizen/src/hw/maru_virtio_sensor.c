/*
 * Virtio Sensor Device
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung Choi <jinhyung2.choi@samsung.com>
 *  Daiyoung Kim <daiyoung777.kim@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "hw/pci/pci.h"

#include "maru_device_ids.h"
#include "maru_virtio_sensor.h"
#include "debug_ch.h"
#include "../ecs.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-sensor);

#define SENSOR_DEVICE_NAME 	"sensor"
#define _MAX_BUF					1024


VirtIOSENSOR* vsensor;

typedef struct msg_info {
	char buf[_MAX_BUF];

	uint16_t type;
	uint16_t req;

	QTAILQ_ENTRY(msg_info) next;
} msg_info;


static QTAILQ_HEAD(msgInfoRecvHead , msg_info) sensor_msg_queue =
    QTAILQ_HEAD_INITIALIZER(sensor_msg_queue);

static pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void add_msg_queue(msg_info* msg)
{
	pthread_mutex_lock(&buf_mutex);
	QTAILQ_INSERT_TAIL(&sensor_msg_queue, msg, next);
	pthread_mutex_unlock(&buf_mutex);

	qemu_bh_schedule(vsensor->bh);
}

void req_sensor_data (enum sensor_types type, enum request_cmd req, char* data, int len)
{
	if (type >= sensor_type_max || (req != request_get && req != request_set)) {
		ERR("unavailable sensor type request.\n");
	}

	msg_info* msg = (msg_info*) malloc(sizeof(msg_info));
	if (!msg) {
		ERR("The allocation of msg_info is failed.\n");
		return;
	}

	memset(msg, 0, sizeof(msg_info));

	if (req == request_set) {
		if (len > _MAX_BUF) {
			ERR("The data is too big to send.\n");
			return;
		}
		memcpy(msg->buf, data, len);
	}

	msg->type = type;
	msg->req = req;

	add_msg_queue(msg);
}

static void flush_sensor_recv_queue(void)
{
	int index;

    if (unlikely(!virtio_queue_ready(vsensor->rvq))) {
        INFO("virtio queue is not ready\n");
        return;
    }

	if (unlikely(virtio_queue_empty(vsensor->rvq))) {
		INFO("virtqueue is empty\n");
		return;
	}

	pthread_mutex_lock(&buf_mutex);
	while (!QTAILQ_EMPTY(&sensor_msg_queue))
	{
		msg_info* msginfo = QTAILQ_FIRST(&sensor_msg_queue);
		if (!msginfo) {
			ERR("msginfo is NULL!\n");
			break;
		}

		INFO("sending message: %s, type: %d, req: %d\n", msginfo->buf, msginfo->type, msginfo->req);

		VirtQueueElement elem;
		index = virtqueue_pop(vsensor->rvq, &elem);
		if (index == 0)
			break;

		memcpy(elem.in_sg[0].iov_base, msginfo, sizeof(struct msg_info));

		virtqueue_push(vsensor->rvq, &elem, sizeof(msg_info));
		virtio_notify(&vsensor->vdev, vsensor->rvq);

		QTAILQ_REMOVE(&sensor_msg_queue, msginfo, next);
		if (msginfo)
			free(msginfo);
	}
	pthread_mutex_unlock(&buf_mutex);
}

static void virtio_sensor_recv(VirtIODevice *vdev, VirtQueue *vq)
{
	flush_sensor_recv_queue();
}

static void maru_sensor_bh(void *opaque)
{
	flush_sensor_recv_queue();
}

static int get_action(enum sensor_types type)
{
	int action = 0;

	switch (type) {
	case sensor_type_accel:
		action = ACTION_ACCEL;
		break;
	case sensor_type_gyro:
		action = ACTION_GYRO;
		break;
	case sensor_type_mag:
		action = ACTION_MAG;
		break;
	case sensor_type_light:
		action = ACTION_LIGHT;
		break;
	case sensor_type_proxi:
		action = ACTION_PROXI;
		break;
	default:
		break;
	}

	return action;
}

static void send_to_ecs(struct msg_info* msg)
{
	int buf_len;
	char data_len [2];
	char group [1] = { GROUP_STATUS };
	char action [1];
	int message_len = 0;

	char* ecs_message = NULL;

	buf_len = strlen(msg->buf);
	message_len =  buf_len + 14;

	ecs_message = (char*) malloc(message_len + 1);
	if (!ecs_message)
		return;

	memset(ecs_message, 0, message_len + 1);

	data_len[0] = buf_len;
	action[0] = get_action(msg->type);

	memcpy(ecs_message, MESSAGE_TYPE_SENSOR, 10);
	memcpy(ecs_message + 10, &data_len, 2);
	memcpy(ecs_message + 12, &group, 1);
	memcpy(ecs_message + 13, &action, 1);
	memcpy(ecs_message + 14, msg->buf, buf_len);

	INFO("ntf_to_injector- bufnum: %s, group: %s, action: %s, data: %s\n", data_len, group, action, msg->buf);

	ntf_to_injector(ecs_message, message_len);

	if (ecs_message)
		free(ecs_message);
}

static void virtio_sensor_send(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtIOSENSOR *vsensor = (VirtIOSENSOR*)vdev;
	struct msg_info msg;
    VirtQueueElement elem;
	int index = 0;

    if (virtio_queue_empty(vsensor->svq)) {
        INFO("<< virtqueue is empty.\n");
        return;
    }

    while ((index = virtqueue_pop(vq, &elem))) {

		memset(&msg, 0x00, sizeof(msg));
		memcpy(&msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

		INFO("send to ecs: %s, len: %d, type: %d, req: %d\n", msg.buf, strlen(msg.buf), msg.type, msg.req);
		send_to_ecs(&msg);
    }

	virtqueue_push(vq, &elem, sizeof(VirtIOSENSOR));
	virtio_notify(&vsensor->vdev, vq);
}


static int virtio_sensor_init(VirtIODevice *vdev)
{
    INFO("initialize virtio-sensor device\n");

    vsensor = VIRTIO_SENSOR(vdev);

    virtio_init(vdev, SENSOR_DEVICE_NAME, VIRTIO_ID_SENSOR, 0);

    if (vsensor == NULL) {
        ERR("failed to initialize sensor device\n");
        return -1;
    }

    vsensor->rvq = virtio_add_queue(&vsensor->vdev, 64, virtio_sensor_recv);
    vsensor->svq = virtio_add_queue(&vsensor->vdev, 64, virtio_sensor_send);

    vsensor->bh = qemu_bh_new(maru_sensor_bh, vsensor);

    return 0;
}

static int virtio_sensor_exit(DeviceState *dev)
{
	VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    INFO("destroy sensor device\n");

	if (vsensor->bh)
		qemu_bh_delete(vsensor->bh);

    virtio_cleanup(vdev);

    return 0;
}


static void virtio_sensor_reset(VirtIODevice *vdev)
{
    TRACE("virtio_sensor_reset.\n");
}

static uint32_t virtio_sensor_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_sensor_get_features.\n");
    return 0;
}


static void virtio_sensor_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_sensor_exit;
    vdc->init = virtio_sensor_init;
    vdc->get_features = virtio_sensor_get_features;
    vdc->reset = virtio_sensor_reset;
}



static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_SENSOR,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOSENSOR),
    .class_init = virtio_sensor_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

