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

#include "maru_device_ids.h"
#include "maru_virtio_evdi.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-evdi);

#define VIRTIO_EVDI_DEVICE_NAME "virtio-evdi"

#define __MAX_BUF_SIZE	1024

enum {
	IOTYPE_INPUT = 0,
	IOTYPE_OUTPUT = 1
};

struct msg_info {
	char buf[__MAX_BUF_SIZE];
	uint32_t use;
};

#if 0
typedef struct VirtIOEVDI{
    VirtIODevice    vdev;
    VirtQueue       *rvq;
    VirtQueue		*svq;
    DeviceState     *qdev;

    QEMUBH *bh;
} VirtIOEVDI;
#endif

VirtIOEVDI* vio_evdi;

static int g_cnt = 0;

static void virtio_evdi_recv(VirtIODevice *vdev, VirtQueue *vq)
{
    int index = 0;

    struct msg_info _msg;

    INFO(">> evdirecv : virtio_evdi_recv\n");

	if (unlikely(virtio_queue_empty(vio_evdi->rvq))) {
		INFO(">> evdirecv : virtqueue is empty\n");
		return;
	}

    VirtQueueElement elem;

    while ((index = virtqueue_pop(vq, &elem))) {

    	INFO(">> evdirecv : virtqueue_pop. index: %d\n", index);
    	INFO(">> evdirecv : element out_num : %d, in_num : %d\n", elem.out_num, elem.in_num);

    	if (index == 0) {
    		INFO("evdirecv : virtqueue break\n");
			break;
		}

    	INFO(">> evdirecv : received use = %d, iov_len = %d\n", _msg.use, elem.in_sg[0].iov_len);

    	memcpy(&_msg, elem.in_sg[0].iov_base,  elem.in_sg[0].iov_len);


    	if (g_cnt < 10)
    	{
			memset(&_msg, 0x00, sizeof(_msg));
			sprintf(_msg.buf, "test_%d\n", g_cnt++);
			memcpy(elem.in_sg[0].iov_base, &_msg, sizeof(struct msg_info));

			INFO(">> evdirecv : send to guest msg use = %d, msg = %s, iov_len = %d \n",
					_msg.use, _msg.buf, elem.in_sg[0].iov_len);


			virtqueue_push(vq, &elem, sizeof(VirtIOEVDI));
			virtio_notify(&vio_evdi->vdev, vq);
    	}
	}

    INFO("enf of virtio_evdi_recv\n");
}


static void virtio_evdi_send(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtIOEVDI *vevdi = (VirtIOEVDI *)vdev;
    int index = 0;
    struct msg_info _msg;

    INFO("<< evdisend : virtio_evdi_send \n");
    if (virtio_queue_empty(vevdi->svq)) {
        INFO("<< evdisend : virtqueue is empty.\n");
        return;
    }

    VirtQueueElement elem;

    while ((index = virtqueue_pop(vq, &elem))) {

		INFO("<< evdisend : virtqueue pop. index: %d\n", index);
		INFO("<< evdisend : element out_num : %d, in_num : %d\n", elem.out_num, elem.in_num);

		if (index == 0) {
			INFO("<< evdisend : virtqueue break\n");
			break;
		}

		INFO("<< evdisend : use=%d, iov_len = %d\n", _msg.use, elem.out_sg[0].iov_len);

		memset(&_msg, 0x00, sizeof(_msg));
		memcpy(&_msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

		INFO("<< evdisend : recv from guest len = %d, msg = %s \n", _msg.use, _msg.buf);
    }

	virtqueue_push(vq, &elem, sizeof(VirtIOEVDI));
	virtio_notify(&vio_evdi->vdev, vq);
}

static void maru_virtio_evdi_notify(void)
{
	TRACE("nothing to do.\n");
}

static uint32_t virtio_evdi_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_evdi_get_features.\n");
    return 0;
}

static void maru_evdi_bh(void *opaque)
{
    maru_virtio_evdi_notify();
}

VirtIODevice *virtio_evdi_init(DeviceState *dev)
{
    INFO("initialize evdi device\n");

    vio_evdi = (VirtIOEVDI *)virtio_common_init(VIRTIO_EVDI_DEVICE_NAME,
            VIRTIO_ID_EVDI, 0, sizeof(VirtIOEVDI));
    if (vio_evdi == NULL) {
        ERR("failed to initialize evdi device\n");
        return NULL;
    }

    vio_evdi->vdev.get_features = virtio_evdi_get_features;
    vio_evdi->rvq = virtio_add_queue(&vio_evdi->vdev, 256, virtio_evdi_recv);
    vio_evdi->svq = virtio_add_queue(&vio_evdi->vdev, 256, virtio_evdi_send);
    vio_evdi->qdev = dev;

    vio_evdi->bh = qemu_bh_new(maru_evdi_bh, vio_evdi);

    return &vio_evdi->vdev;
}

void virtio_evdi_exit(VirtIODevice *vdev)
{
    INFO("destroy evdi device\n");

    if (vio_evdi->bh) {
            qemu_bh_delete(vio_evdi->bh);
        }

    virtio_cleanup(vdev);
}

