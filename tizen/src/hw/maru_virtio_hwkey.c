/*
 * Maru Virtio HW Key Device
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  GiWoong Kim <giwoong.kim@samsung.com>
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
#include "emul_state.h"
#include "maru_virtio_hwkey.h"
#include "maru_device_ids.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, hwkey);

#define DEVICE_NAME "virtio-hwkey"
#define MAX_BUF_COUNT 64
static int vqidx = 0;

/*
 * HW key event queue
 */
typedef struct HwKeyEventEntry {
    unsigned int index;
    EmulHWKeyEvent hwkey;

    QTAILQ_ENTRY(HwKeyEventEntry) node;
} HwKeyEventEntry;

/* the maximum number of HW key event that can be put into a queue */
#define MAX_HWKEY_EVENT_CNT 64

static HwKeyEventEntry _events_buf[MAX_HWKEY_EVENT_CNT];
static QTAILQ_HEAD(, HwKeyEventEntry) events_queue =
    QTAILQ_HEAD_INITIALIZER(events_queue);

static unsigned int event_ringbuf_cnt; /* _events_buf */
static unsigned int event_queue_cnt; /* events_queue */

/*
 * VirtQueueElement queue
 */
typedef struct ElementEntry {
    unsigned int el_index;
    unsigned int sg_index;
    VirtQueueElement elem;

    QTAILQ_ENTRY(ElementEntry) node;
} ElementEntry;

static QTAILQ_HEAD(, ElementEntry) elem_queue =
    QTAILQ_HEAD_INITIALIZER(elem_queue);

static unsigned int elem_ringbuf_cnt; /* _elem_buf */
static unsigned int elem_queue_cnt; /* elem_queue */

VirtIOHWKey *vhk;
VirtQueueElement elem_vhk;

/* lock for between communication thread and IO thread */
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

void maru_hwkey_event(int event_type, int keycode)
{
    HwKeyEventEntry *entry = NULL;

    if (unlikely(event_queue_cnt >= MAX_HWKEY_EVENT_CNT)) {
        INFO("full hwkey event queue, lose event\n", event_queue_cnt);

        qemu_bh_schedule(vhk->bh);
        return;
    }

    entry = &(_events_buf[event_ringbuf_cnt % MAX_HWKEY_EVENT_CNT]);
    event_ringbuf_cnt++;

    /* hwkey event is copied into the queue */
    entry->hwkey.keycode = keycode;
    entry->hwkey.event_type = event_type;

    pthread_mutex_lock(&event_mutex);

    entry->index = ++event_queue_cnt; // 1 ~

    QTAILQ_INSERT_TAIL(&events_queue, entry, node);

    pthread_mutex_unlock(&event_mutex);

    /* call maru_virtio_hwkey_notify */
    qemu_bh_schedule(vhk->bh);

    TRACE("hwkey event (%d) : keycode=%d, event_type=%d\n",
        entry->index, entry->hwkey.keycode, entry->hwkey.event_type);
}

static void maru_virtio_hwkey_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    int virt_sg_index = 0;

    TRACE("maru_virtio_hwkey_handle\n");

    if (unlikely(virtio_queue_empty(vhk->vq))) {
        TRACE("virtqueue is empty\n");
        return;
    }
    /* Get a queue buffer which is written by guest side. */
    do {
        virt_sg_index = virtqueue_pop(vq, &elem_vhk);
        TRACE("virtqueue pop.\n");
    } while (virt_sg_index < MAX_BUF_COUNT);
}

void maru_virtio_hwkey_notify(void)
{
    HwKeyEventEntry *event_entry = NULL;

    TRACE("maru_virtio_hwkey_notify\n");

    if (unlikely(!virtio_queue_ready(vhk->vq))) {
        ERR("virtio queue is not ready\n");
        return;
    }

    while (true) {
        if (event_queue_cnt == 0) {
            TRACE("no event\n");
            break;
        }

        /* get hwkey event from host queue */
        event_entry = QTAILQ_FIRST(&events_queue);

        printf("keycode=%d, event_type=%d, event_queue_cnt=%d, vqidx=%d\n",
              event_entry->hwkey.keycode, event_entry->hwkey.event_type,
              event_queue_cnt, vqidx);

        /* copy event into virtio buffer */
        memcpy(elem_vhk.in_sg[vqidx++].iov_base, &(event_entry->hwkey), sizeof(EmulHWKeyEvent));
        if (vqidx == MAX_BUF_COUNT) {
            vqidx = 0;
        }

        virtqueue_push(vhk->vq, &elem_vhk, sizeof(EmulHWKeyEvent));
        virtio_notify(&vhk->vdev, vhk->vq);

        pthread_mutex_lock(&event_mutex);

        /* remove host event */
        QTAILQ_REMOVE(&events_queue, event_entry, node);
        event_queue_cnt--;

        pthread_mutex_unlock(&event_mutex);
    }
}

static uint32_t virtio_hwkey_get_features(
    VirtIODevice *vdev, uint32_t request_features)
{
    // TODO:
    return request_features;
}

static void maru_hwkey_bh(void *opaque)
{
    maru_virtio_hwkey_notify();
}

static int virtio_hwkey_device_init(VirtIODevice *vdev)
{
    INFO("initialize the hwkey device\n");
    DeviceState *qdev = DEVICE(vdev);
    vhk = VIRTIO_HWKEY(vdev);

    virtio_init(vdev, TYPE_VIRTIO_HWKEY, VIRTIO_ID_HWKEY, 0);

    if (vdev == NULL) {
        ERR("failed to initialize the hwkey device\n");
        return -1;
    }

    vhk->vq = virtio_add_queue(vdev, MAX_BUF_COUNT, maru_virtio_hwkey_handle);

    vhk->qdev = qdev;

    /* reset the counters */
    event_queue_cnt = event_ringbuf_cnt = 0;
    elem_queue_cnt = elem_ringbuf_cnt = 0;

    /* bottom-half */
    vhk->bh = qemu_bh_new(maru_hwkey_bh, vhk);

    return 0;
}

static int virtio_hwkey_device_exit(DeviceState *qdev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(qdev);

    INFO("exit the hwkey device\n");

    if (vhk->bh) {
        qemu_bh_delete(vhk->bh);
    }

    virtio_cleanup(vdev);

    pthread_mutex_destroy(&event_mutex);

    return 0;
}

static void virtio_hwkey_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_hwkey_device_exit;
    vdc->init = virtio_hwkey_device_init;
    vdc->get_features = virtio_hwkey_get_features;
}

static const TypeInfo virtio_hwkey_info = {
    .name = TYPE_VIRTIO_HWKEY,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOHWKey),
    .class_init = virtio_hwkey_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_hwkey_info);
}

type_init(virtio_register_types)

