/*
 * Virtio NFC Device
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Munkyu Im <munkyu.im@samsung.com>
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

#include <pthread.h>

#include "maru_device_ids.h"
#include "maru_virtio_nfc.h"
#include "debug_ch.h"
#include "../ecs.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-nfc);

#define NFC_DEVICE_NAME "virtio-nfc"


enum {
	IOTYPE_INPUT = 0,
	IOTYPE_OUTPUT = 1
};


#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define MAX_BUF_SIZE    4096


VirtIONFC* vio_nfc;


typedef unsigned int CSCliSN;

typedef struct msg_info {
	char buf[MAX_BUF_SIZE];

	uint32_t route;
	uint32_t use;
	uint16_t count;
	uint16_t index;

	CSCliSN cclisn;
}msg_info;

//

typedef struct MsgInfo
{
	msg_info info;
	QTAILQ_ENTRY(MsgInfo) next;
}MsgInfo;

static QTAILQ_HEAD(MsgInfoRecvHead , MsgInfo) nfc_recv_msg_queue =
    QTAILQ_HEAD_INITIALIZER(nfc_recv_msg_queue);


static QTAILQ_HEAD(MsgInfoSendHead , MsgInfo) nfc_send_msg_queue =
    QTAILQ_HEAD_INITIALIZER(nfc_send_msg_queue);


//

typedef struct NFCBuf {
    VirtQueueElement elem;

    QTAILQ_ENTRY(NFCBuf) next;
} NFCBuf;

static QTAILQ_HEAD(NFCMsgHead , NFCBuf) nfc_in_queue =
    QTAILQ_HEAD_INITIALIZER(nfc_in_queue);

static int count = 0;

static pthread_mutex_t recv_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

bool send_to_nfc(enum request_cmd_nfc req, char* data, const uint32_t len)
{
    MsgInfo* _msg = (MsgInfo*) malloc(sizeof(MsgInfo));
    if (!_msg)
        return false;

    memset(&_msg->info, 0, sizeof(msg_info));

    memcpy(_msg->info.buf, data, len);
    _msg->info.use = len;
    _msg->info.index = count++;
    _msg->info.route = req;
    pthread_mutex_lock(&recv_buf_mutex);

    QTAILQ_INSERT_TAIL(&nfc_recv_msg_queue, _msg, next);

    pthread_mutex_unlock(&recv_buf_mutex);

	qemu_bh_schedule(vio_nfc->bh);

	return true;
}


static int g_cnt = 0;

static void flush_nfc_recv_queue(void)
{
	int index;

    if (unlikely(!virtio_queue_ready(vio_nfc->rvq))) {
        INFO("virtio queue is not ready\n");
        return;
    }

	if (unlikely(virtio_queue_empty(vio_nfc->rvq))) {
		TRACE("virtqueue is empty\n");
		return;
	}


	pthread_mutex_lock(&recv_buf_mutex);

	while (!QTAILQ_EMPTY(&nfc_recv_msg_queue))
	{
		 MsgInfo* msginfo = QTAILQ_FIRST(&nfc_recv_msg_queue);
		 if (!msginfo)
			 break;

		 VirtQueueElement elem;
		 index = virtqueue_pop(vio_nfc->rvq, &elem);
		 if (index == 0)
		 {
			 //ERR("unexpected empty queue");
			 break;
		 }

		 INFO(">> virtqueue_pop. index: %d, out_num : %d, in_num : %d\n", index, elem.out_num, elem.in_num);

		 memcpy(elem.in_sg[0].iov_base, &msginfo->info, sizeof(struct msg_info));

		 INFO(">> send to guest count = %d, use = %d, msg = %s, iov_len = %d \n",
				 ++g_cnt, msginfo->info.use, msginfo->info.buf, elem.in_sg[0].iov_len);

		 virtqueue_push(vio_nfc->rvq, &elem, sizeof(msg_info));
		 virtio_notify(&vio_nfc->vdev, vio_nfc->rvq);

		 QTAILQ_REMOVE(&nfc_recv_msg_queue, msginfo, next);
		 if (msginfo)
			 free(msginfo);
	}

	pthread_mutex_unlock(&recv_buf_mutex);

}


static void virtio_nfc_recv(VirtIODevice *vdev, VirtQueue *vq)
{
	flush_nfc_recv_queue();
}

static void send_to_ecs(struct msg_info* msg)
{
	int buf_len;
	char data_len [2];
	char group [1] = { 15 }; 
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
	action[0] = 0;

	memcpy(ecs_message, "nfc", 10);
	memcpy(ecs_message + 10, &data_len, 2);
	memcpy(ecs_message + 12, &group, 1);
	memcpy(ecs_message + 13, &action, 1);
	memcpy(ecs_message + 14, msg->buf, buf_len);

	INFO("ntf_to_injector- bufnum: %s, group: %s, action: %s, data: %s\n", data_len, group, action, msg->buf);

	ntf_to_injector(ecs_message, message_len);

	if (ecs_message)
		free(ecs_message);
}

static void virtio_nfc_send(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtIONFC *vnfc = (VirtIONFC *)vdev;
    int index = 0;
    struct msg_info _msg;

    if (virtio_queue_empty(vnfc->svq)) {
        INFO("<< virtqueue is empty.\n");
        return;
    }

    VirtQueueElement elem;

    while ((index = virtqueue_pop(vq, &elem))) {

		INFO("<< virtqueue pop. index: %d, out_num : %d, in_num : %d\n", index,  elem.out_num, elem.in_num);

		if (index == 0) {
			INFO("<< virtqueue break\n");
			break;
		}

		INFO("<< use=%d, iov_len = %d\n", _msg.use, elem.out_sg[0].iov_len);

		memset(&_msg, 0x00, sizeof(_msg));
		memcpy(&_msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

		INFO("<< recv from guest len = %d, msg = %s \n", _msg.use, _msg.buf);

        send_to_ecs(&_msg);

    }

	virtqueue_push(vq, &elem, sizeof(VirtIONFC));
	virtio_notify(&vio_nfc->vdev, vq);
}

static uint32_t virtio_nfc_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_nfc_get_features.\n");
    return 0;
}

static void maru_nfc_bh(void *opaque)
{
	flush_nfc_recv_queue();
}

static int virtio_nfc_init(VirtIODevice* vdev)
{
    INFO("initialize nfc device\n");
    vio_nfc = VIRTIO_NFC(vdev);

    virtio_init(vdev, NFC_DEVICE_NAME, VIRTIO_ID_NFC, 0);

    if (vio_nfc == NULL) {
        ERR("failed to initialize nfc device\n");
        return -1;
    }

    vio_nfc->rvq = virtio_add_queue(&vio_nfc->vdev, 256, virtio_nfc_recv);
    vio_nfc->svq = virtio_add_queue(&vio_nfc->vdev, 256, virtio_nfc_send);

    vio_nfc->bh = qemu_bh_new(maru_nfc_bh, vio_nfc);

    return 0;
}

static int virtio_nfc_exit(DeviceState* dev)
{
    INFO("destroy nfc device\n");
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    if (vio_nfc->bh) {
            qemu_bh_delete(vio_nfc->bh);
        }

    virtio_cleanup(vdev);

    return 0;
}

static void virtio_nfc_reset(VirtIODevice *vdev)
{
    TRACE("virtio_sensor_reset.\n");
}


static void virtio_nfc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_nfc_exit;
    vdc->init = virtio_nfc_init;
    vdc->get_features = virtio_nfc_get_features;
    vdc->reset = virtio_nfc_reset;
}



static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_NFC,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIONFC),
    .class_init = virtio_nfc_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

