/*
 * Virtio Jack Device
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <pthread.h>

#include "hw/pci/pci.h"

#include "maru_device_ids.h"
#include "debug_ch.h"

#include "maru_virtio_jack.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-jack);

#define JACK_DEVICE_NAME  "jack"
#define _MAX_BUF          1024
#define __MAX_BUF_JACK    32

static int charger_online = 0;
static int earjack_online = 0;
static int earkey_online = 0;
static int hdmi_online = 0;
static int usb_online = 0;

VirtIOJACK* vjack;

typedef struct msg_info {
    char buf[_MAX_BUF];

    uint16_t type;
    uint16_t req;
} msg_info;

enum request_cmd {
    request_get = 0,
    request_set,
    request_answer
};

void set_jack_charger(int online){
    charger_online = online;
}

int get_jack_charger(void) {
    return charger_online;
}

void set_jack_usb(int online){
    usb_online = online;
}

int get_jack_usb(void) {
    return usb_online;
}

static void set_jack_data (enum jack_types type, char* data, int len)
{
    if (len < 0 || len > __MAX_BUF_JACK) {
        ERR("jack data size is wrong.\n");
        return;
    }

    if (data == NULL) {
        ERR("jack data is NULL.\n");
        return;
    }

    switch (type) {
        case jack_type_charger:
            sscanf(data, "%d", &charger_online);
            break;
        case jack_type_earjack:
            sscanf(data, "%d", &earjack_online);
            break;
        case jack_type_earkey:
            sscanf(data, "%d", &earkey_online);
            break;
        case jack_type_hdmi:
            sscanf(data, "%d", &hdmi_online);
            break;
        case jack_type_usb:
            sscanf(data, "%d", &usb_online);
            break;
        default:
            return;
    }
}

static void get_jack_data(enum jack_types type, char* msg_info)
{
    if (msg_info == NULL) {
        return;
    }

    switch (type) {
        case jack_type_charger:
            sprintf(msg_info, "%d", charger_online);
            break;
        case jack_type_earjack:
            sprintf(msg_info, "%d", earjack_online);
            break;
        case jack_type_earkey:
            sprintf(msg_info, "%d", earkey_online);
            break;
        case jack_type_hdmi:
            sprintf(msg_info, "%d", hdmi_online);
            break;
        case jack_type_usb:
            sprintf(msg_info, "%d", usb_online);
            break;
        default:
            return;
    }
}

static void answer_jack_data_request(int type, char* data, VirtQueueElement *elem)
{
    msg_info* msginfo = (msg_info*) malloc(sizeof(msg_info));
    if (!msginfo) {
        ERR("msginfo is NULL!\n");
        return;
    }

    msginfo->req = request_answer;
    msginfo->type = type;
    get_jack_data(type, msginfo->buf);

    INFO("sending message: %s, type: %d, req: %d\n", msginfo->buf, msginfo->type, msginfo->req);

    memset(elem->in_sg[0].iov_base, 0, elem->in_sg[0].iov_len);
    memcpy(elem->in_sg[0].iov_base, msginfo, sizeof(struct msg_info));

    if (msginfo)
        free(msginfo);
}

static void handle_msg(struct msg_info *msg, VirtQueueElement *elem)
{
    unsigned int len = 0;

    if (msg == NULL) {
        INFO("msg info structure is NULL.\n");
        return;
    }

    if (msg->req == request_set) {
        set_jack_data (msg->type, msg->buf, strlen(msg->buf));
    } else if (msg->req == request_get) {
        answer_jack_data_request(msg->type, msg->buf, elem);
        len = sizeof(msg_info);
    }

    virtqueue_push(vjack->vq, elem, len);
    virtio_notify(&vjack->vdev, vjack->vq);
}

static void virtio_jack_vq(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOJACK *vjack = (VirtIOJACK*)vdev;
    struct msg_info msg;
    VirtQueueElement elem;
    int index = 0;

    if (vjack->vq == NULL) {
        ERR("virt queue is not ready.\n");
        return;
    }

    if (!virtio_queue_ready(vjack->vq)) {
        ERR("virtqueue is not ready.");
        return;
    }

    if (virtio_queue_empty(vjack->vq)) {
        ERR("<< virtqueue is empty.\n");
        return;
    }

    while ((index = virtqueue_pop(vq, &elem))) {
        memset(&msg, 0x00, sizeof(msg));
        memcpy(&msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

        INFO("handling msg from driver: %s, len: %d, type: %d, req: %d, index: %d\n", msg.buf, strlen(msg.buf), msg.type, msg.req, index);

        handle_msg(&msg, &elem);
    }
}

static int virtio_jack_init(VirtIODevice *vdev)
{
    INFO("initialize virtio-jack device\n");

    vjack = VIRTIO_JACK(vdev);

    virtio_init(vdev, JACK_DEVICE_NAME, VIRTIO_ID_JACK, 0);

    if (vjack == NULL) {
        ERR("failed to initialize jack device\n");
        return -1;
    }

    vjack->vq = virtio_add_queue(&vjack->vdev, 64, virtio_jack_vq);

    return 0;
}

static int virtio_jack_exit(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    INFO("destroy jack device\n");

    virtio_cleanup(vdev);

    return 0;
}


static void virtio_jack_reset(VirtIODevice *vdev)
{
    TRACE("virtio_jack_reset.\n");
}

static uint32_t virtio_jack_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_jack_get_features.\n");
    return 0;
}

static void virtio_jack_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_jack_exit;
    vdc->init = virtio_jack_init;
    vdc->get_features = virtio_jack_get_features;
    vdc->reset = virtio_jack_reset;
}

static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_JACK,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOJACK),
    .class_init = virtio_jack_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

