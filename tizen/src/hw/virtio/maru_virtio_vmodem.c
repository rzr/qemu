/*
 * Virtio Virtual Modem Device
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Sooyoung Ha <yoosah.ha@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
 *  Sangho Park <sangho1206.park@samsung.com>
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

#include "hw/maru_device_ids.h"
#include "maru_virtio_vmodem.h"
#include "maru_virtio_evdi.h"
#include "debug_ch.h"
#include "ecs/ecs.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-vmodem);

#define VMODEM_DEVICE_NAME "virtio-vmodem"

enum {
    IOTYPE_INPUT = 0,
    IOTYPE_OUTPUT = 1
};

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

VirtIOVModem *vio_vmodem;

typedef struct MsgInfo
{
    msg_info info;
    QTAILQ_ENTRY(MsgInfo) next;
}MsgInfo;

static QTAILQ_HEAD(MsgInfoRecvHead , MsgInfo) vmodem_recv_msg_queue =
    QTAILQ_HEAD_INITIALIZER(vmodem_recv_msg_queue);

typedef struct EvdiBuf {
    VirtQueueElement elem;

    QTAILQ_ENTRY(EvdiBuf) next;
} EvdiBuf;

static QTAILQ_HEAD(EvdiMsgHead , EvdiBuf) vmodem_in_queue =
    QTAILQ_HEAD_INITIALIZER(vmodem_in_queue);

bool send_to_vmodem(const uint32_t route, char *data, const uint32_t len)
{
    int size;
    int left = len;
    int count = 0;
    char *readptr = data;

    while (left > 0) {
        MsgInfo *_msg = (MsgInfo*) malloc(sizeof(MsgInfo));
        if (!_msg) {
            ERR("malloc failed\n");
            return false;
        }

        memset(&_msg->info, 0, sizeof(msg_info));

        size = min(left, __MAX_BUF_SIZE);
        memcpy(_msg->info.buf, readptr, size);
        readptr += size;
        _msg->info.use = size;
        _msg->info.index = count;

        qemu_mutex_lock(&vio_vmodem->mutex);

        QTAILQ_INSERT_TAIL(&vmodem_recv_msg_queue, _msg, next);

        qemu_mutex_unlock(&vio_vmodem->mutex);

        left -= size;
        count ++;
    }

    qemu_bh_schedule(vio_vmodem->bh);

    return true;
}

static void flush_vmodem_recv_queue(void)
{
    int index;

    if (unlikely(!virtio_queue_ready(vio_vmodem->rvq))) {
        ERR("virtio queue is not ready\n");
        return;
    }

    if (unlikely(virtio_queue_empty(vio_vmodem->rvq))) {
        ERR("virtqueue is empty\n");
        return;
    }

    qemu_mutex_lock(&vio_vmodem->mutex);

    while (!QTAILQ_EMPTY(&vmodem_recv_msg_queue))
    {
        MsgInfo *msginfo = QTAILQ_FIRST(&vmodem_recv_msg_queue);
        if (!msginfo)
            break;

        VirtQueueElement elem;
        index = virtqueue_pop(vio_vmodem->rvq, &elem);
        if (index == 0) {
            break;
        }

        memset(elem.in_sg[0].iov_base, 0, elem.in_sg[0].iov_len);
        memcpy(elem.in_sg[0].iov_base, &msginfo->info, sizeof(struct msg_info));

        virtqueue_push(vio_vmodem->rvq, &elem, sizeof(msg_info));
        virtio_notify(&vio_vmodem->vdev, vio_vmodem->rvq);

        QTAILQ_REMOVE(&vmodem_recv_msg_queue, msginfo, next);
        if (msginfo)
            free(msginfo);
    }
    qemu_mutex_unlock(&vio_vmodem->mutex);
}


static void virtio_vmodem_recv(VirtIODevice *vdev, VirtQueue *vq)
{
    flush_vmodem_recv_queue();
}

static void virtio_vmodem_send(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOVModem *vvmodem = (VirtIOVModem *)vdev;
    int index = 0;
    struct msg_info _msg;

    if (virtio_queue_empty(vvmodem->svq)) {
        ERR("virtqueue is empty.\n");
        return;
    }

    VirtQueueElement elem;

    while ((index = virtqueue_pop(vq, &elem))) {
        memset(&_msg, 0x00, sizeof(_msg));
        memcpy(&_msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

        TRACE("vmodem send to ecp.\n");
        send_injector_ntf(_msg.buf, _msg.use);
    }

    virtqueue_push(vq, &elem, sizeof(VirtIOVModem));
    virtio_notify(&vio_vmodem->vdev, vq);
}

static uint32_t virtio_vmodem_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_vmodem_get_features.\n");
    return 0;
}

static void maru_vmodem_bh(void *opaque)
{
    flush_vmodem_recv_queue();
}

static void virtio_vmodem_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    vio_vmodem = VIRTIO_VMODEM(vdev);

    if (vio_vmodem == NULL) {
        ERR("failed to initialize vmodem device\n");
        return;
    }

    virtio_init(vdev, TYPE_VIRTIO_VMODEM, VIRTIO_ID_VMODEM, 0); //VMODEM_DEVICE_NAME
    qemu_mutex_init(&vio_vmodem->mutex);

    vio_vmodem->rvq = virtio_add_queue(&vio_vmodem->vdev, 256, virtio_vmodem_recv);
    vio_vmodem->svq = virtio_add_queue(&vio_vmodem->vdev, 256, virtio_vmodem_send);
    vio_vmodem->qdev = dev;

    vio_vmodem->bh = qemu_bh_new(maru_vmodem_bh, vio_vmodem);

    INFO("finish the vmodem device initialization.\n");
}

static void virtio_vmodem_unrealize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    INFO("destroy vmodem device\n");

    if (vio_vmodem->bh) {
        qemu_bh_delete(vio_vmodem->bh);
    }

    qemu_mutex_destroy(&vio_vmodem->mutex);
    virtio_cleanup(vdev);
}

static void virtio_vmodem_reset(VirtIODevice *vdev)
{
    INFO("virtio_vmodem_reset.\n");
}


static void virtio_vmodem_class_init(ObjectClass *klass, void *data)
{
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = virtio_vmodem_realize;
    vdc->unrealize = virtio_vmodem_unrealize;
    vdc->get_features = virtio_vmodem_get_features;
    vdc->reset = virtio_vmodem_reset;
}



static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_VMODEM,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOVModem),
    .class_init = virtio_vmodem_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)
