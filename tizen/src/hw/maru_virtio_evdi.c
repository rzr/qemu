/*
 * Virtio EmulatorVirtualDeviceInterface Device
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  DaiYoung Kim <daiyoung777.kim.hwang@samsung.com>
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
#include "maru_virtio_evdi.h"
#include "debug_ch.h"
#include "../ecs.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-evdi);

#define EVDI_DEVICE_NAME "virtio-evdi"

enum {
	IOTYPE_INPUT = 0,
	IOTYPE_OUTPUT = 1
};


#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif


VirtIOEVDI* vio_evdi;


//

typedef struct MsgInfo
{
	msg_info info;
	QTAILQ_ENTRY(MsgInfo) next;
}MsgInfo;

static QTAILQ_HEAD(MsgInfoRecvHead , MsgInfo) evdi_recv_msg_queue =
    QTAILQ_HEAD_INITIALIZER(evdi_recv_msg_queue);

//

typedef struct EvdiBuf {
    VirtQueueElement elem;

    QTAILQ_ENTRY(EvdiBuf) next;
} EvdiBuf;

static QTAILQ_HEAD(EvdiMsgHead , EvdiBuf) evdi_in_queue =
    QTAILQ_HEAD_INITIALIZER(evdi_in_queue);


static pthread_mutex_t recv_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

bool send_to_evdi(const uint32_t route, char* data, const uint32_t len)
{
	int size;
	int left = len;
	int count = 0;
	char* readptr = data;

	while (left > 0)
	{
		MsgInfo* _msg = (MsgInfo*) malloc(sizeof(MsgInfo));
		if (!_msg)
			return false;

		memset(&_msg->info, 0, sizeof(msg_info));

		size = min(left, __MAX_BUF_SIZE);
		memcpy(_msg->info.buf, readptr, size);
		readptr += size;
		_msg->info.use = size;
		_msg->info.index = count;

		pthread_mutex_lock(&recv_buf_mutex);

		QTAILQ_INSERT_TAIL(&evdi_recv_msg_queue, _msg, next);

		pthread_mutex_unlock(&recv_buf_mutex);

		left -= size;
		count ++;
	}

	qemu_bh_schedule(vio_evdi->bh);

	return true;
}


static void flush_evdi_recv_queue(void)
{
	int index;

    if (unlikely(!virtio_queue_ready(vio_evdi->rvq))) {
        INFO("virtio queue is not ready\n");
        return;
    }

	if (unlikely(virtio_queue_empty(vio_evdi->rvq))) {
		TRACE("virtqueue is empty\n");
		return;
	}


	pthread_mutex_lock(&recv_buf_mutex);

	while (!QTAILQ_EMPTY(&evdi_recv_msg_queue))
	{
		 MsgInfo* msginfo = QTAILQ_FIRST(&evdi_recv_msg_queue);
		 if (!msginfo)
			 break;

		 VirtQueueElement elem;
		 index = virtqueue_pop(vio_evdi->rvq, &elem);
		 if (index == 0)
		 {
			 //ERR("unexpected empty queue");
			 break;
		 }

		 //INFO(">> virtqueue_pop. index: %d, out_num : %d, in_num : %d\n", index, elem.out_num, elem.in_num);

		 memset(elem.in_sg[0].iov_base, 0, elem.in_sg[0].iov_len);
		 memcpy(elem.in_sg[0].iov_base, &msginfo->info, sizeof(struct msg_info));

		 //INFO(">> send to guest count = %d, use = %d, msg = %s, iov_len = %d \n",
				// ++g_cnt, msginfo->info.use, msginfo->info.buf, elem.in_sg[0].iov_len);

		 virtqueue_push(vio_evdi->rvq, &elem, sizeof(msg_info));
		 virtio_notify(&vio_evdi->vdev, vio_evdi->rvq);

		 QTAILQ_REMOVE(&evdi_recv_msg_queue, msginfo, next);
		 if (msginfo)
			 free(msginfo);
	}

	pthread_mutex_unlock(&recv_buf_mutex);

}


static void virtio_evdi_recv(VirtIODevice *vdev, VirtQueue *vq)
{
	flush_evdi_recv_queue();
}

static void virtio_evdi_send(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtIOEVDI *vevdi = (VirtIOEVDI *)vdev;
    int index = 0;
    struct msg_info _msg;

    if (virtio_queue_empty(vevdi->svq)) {
        INFO("<< virtqueue is empty.\n");
        return;
    }

    VirtQueueElement elem;

    while ((index = virtqueue_pop(vq, &elem))) {

		//INFO("<< virtqueue pop. index: %d, out_num : %d, in_num : %d\n", index,  elem.out_num, elem.in_num);

		if (index == 0) {
			INFO("<< virtqueue break\n");
			break;
		}

		//INFO("<< use=%d, iov_len = %d\n", _msg.use, elem.out_sg[0].iov_len);

		memset(&_msg, 0x00, sizeof(_msg));
		memcpy(&_msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

		//INFO("<< recv from guest len = %d, msg = %s \n", _msg.use, _msg.buf);

		send_injector_ntf(_msg.buf, _msg.use);
    }

	virtqueue_push(vq, &elem, sizeof(VirtIOEVDI));
	virtio_notify(&vio_evdi->vdev, vq);
}

static uint32_t virtio_evdi_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_evdi_get_features.\n");
    return 0;
}

static void maru_evdi_bh(void *opaque)
{
	flush_evdi_recv_queue();
}

static int virtio_evdi_init(VirtIODevice *vdev)
{
    INFO("initialize evdi device\n");

    vio_evdi = VIRTIO_EVDI(vdev);

    virtio_init(vdev, EVDI_DEVICE_NAME, VIRTIO_ID_EVDI, 0);

    if (vio_evdi == NULL) {
        ERR("failed to initialize evdi device\n");
        return -1; //need any guide for return value
    }

    vio_evdi->rvq = virtio_add_queue(&vio_evdi->vdev, 256, virtio_evdi_recv);
    vio_evdi->svq = virtio_add_queue(&vio_evdi->vdev, 256, virtio_evdi_send);

    vio_evdi->bh = qemu_bh_new(maru_evdi_bh, vio_evdi);

    return 0;
}

static int virtio_evdi_exit(DeviceState *dev)
{
	VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    INFO("destroy evdi device\n");

    if (vio_evdi->bh) {
            qemu_bh_delete(vio_evdi->bh);
        }

    virtio_cleanup(vdev);

    return 0;
}

static void virtio_evdi_reset(VirtIODevice *vdev)
{
    TRACE("virtio_evdi_reset.\n");
}


static void virtio_evdi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_evdi_exit;
    vdc->init = virtio_evdi_init;
    vdc->get_features = virtio_evdi_get_features;
    vdc->reset = virtio_evdi_reset;
}



static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_EVDI,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOEVDI),
    .class_init = virtio_evdi_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)
