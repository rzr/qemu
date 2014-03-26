/*
 * Virtio Power Device
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

#include "maru_virtio_power.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-power);

#define POWER_DEVICE_NAME  "power_supply"
#define _MAX_BUF           1024
#define __MAX_BUF_POWER    32

static int capacity = 50;
static int charge_full = 0;
static int charge_now = 0;

VirtIOPOWER* vpower;

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

static void set_power_data (enum power_types type, char* data, int len)
{
    if (len < 0 || len > __MAX_BUF_POWER) {
        ERR("power data size is wrong.\n");
        return;
    }

    if (data == NULL) {
        ERR("power data is NULL.\n");
        return;
    }

    switch (type) {
        case power_type_capacity:
            sscanf(data, "%d", &capacity);
            break;
        case power_type_charge_full:
            sscanf(data, "%d", &charge_full);
            break;
        case power_type_charge_now:
            sscanf(data, "%d", &charge_now);
            break;
        default:
            return;
    }
}

static void get_power_data(enum power_types type, char* msg_info)
{
    if (msg_info == NULL) {
        return;
    }

    switch (type) {
        case power_type_capacity:
            sprintf(msg_info, "%d", capacity);
            break;
        case power_type_charge_full:
            sprintf(msg_info, "%d", charge_full);
            break;
        case power_type_charge_now:
            sprintf(msg_info, "%d", charge_now);
            break;
        default:
            return;
    }
}

void set_power_capacity(int level) {
    capacity = level;
}

int get_power_capacity(void) {
    return capacity;
}

void set_power_charge_full(int full){
    charge_full = full;
}

int get_power_charge_full(void){
    return charge_full;
}

void set_power_charge_now(int now){
    charge_now = now;
}

int get_power_charge_now(void){
    return charge_now;
}

static void answer_power_data_request(int type, char* data, VirtQueueElement *elem)
{
    msg_info* msginfo = (msg_info*) malloc(sizeof(msg_info));
    if (!msginfo) {
        ERR("msginfo is NULL!\n");
        return;
    }

    msginfo->req = request_answer;
    msginfo->type = type;
    get_power_data(type, msginfo->buf);

    TRACE("sending message: %s, type: %d, req: %d\n", msginfo->buf, msginfo->type, msginfo->req);

    memset(elem->in_sg[0].iov_base, 0, elem->in_sg[0].iov_len);
    memcpy(elem->in_sg[0].iov_base, msginfo, sizeof(struct msg_info));

    if (msginfo)
        free(msginfo);
}

static void handle_msg(struct msg_info *msg, VirtQueueElement *elem)
{
    unsigned int len = 0;

    if (msg == NULL) {
        ERR("msg info structure is NULL.\n");
        return;
    }

    if (msg->req == request_set) {
        set_power_data (msg->type, msg->buf, strlen(msg->buf));
    } else if (msg->req == request_get) {
        answer_power_data_request(msg->type, msg->buf, elem);
        len = sizeof(msg_info);
    }

    virtqueue_push(vpower->vq, elem, len);
    virtio_notify(&vpower->vdev, vpower->vq);
}

static void virtio_power_vq(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOPOWER *vpower = (VirtIOPOWER*)vdev;
    struct msg_info msg;
    VirtQueueElement elem;
    int index = 0;

    if (vpower->vq == NULL) {
        ERR("virt queue is not ready.\n");
        return;
    }

    if (!virtio_queue_ready(vpower->vq)) {
        ERR("virtqueue is not ready.");
        return;
    }

    if (virtio_queue_empty(vpower->vq)) {
        ERR("<< virtqueue is empty.\n");
        return;
    }

    while ((index = virtqueue_pop(vq, &elem))) {
        memset(&msg, 0x00, sizeof(msg));
        memcpy(&msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

        TRACE("handling msg from driver: %s, len: %d, type: %d, req: %d, index: %d\n", msg.buf, strlen(msg.buf), msg.type, msg.req, index);

        handle_msg(&msg, &elem);
    }
}

static int virtio_power_init(VirtIODevice *vdev)
{
    INFO("initialize virtio-power device\n");

    vpower = VIRTIO_POWER(vdev);

    virtio_init(vdev, POWER_DEVICE_NAME, VIRTIO_ID_POWER, 0);

    if (vpower == NULL) {
        ERR("failed to initialize power device\n");
        return -1;
    }

    vpower->vq = virtio_add_queue(&vpower->vdev, 64, virtio_power_vq);

    return 0;
}

static int virtio_power_exit(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    INFO("destroy power device\n");

    virtio_cleanup(vdev);

    return 0;
}


static void virtio_power_reset(VirtIODevice *vdev)
{
    TRACE("virtio_power_reset.\n");
}

static uint32_t virtio_power_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_power_get_features.\n");
    return 0;
}

static void virtio_power_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_power_exit;
    vdc->init = virtio_power_init;
    vdc->get_features = virtio_power_get_features;
    vdc->reset = virtio_power_reset;
}

static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_POWER,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOPOWER),
    .class_init = virtio_power_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

