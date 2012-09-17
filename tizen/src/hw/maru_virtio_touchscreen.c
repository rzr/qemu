/*
 * Maru Virtio Touchscreen Device
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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
#include "console.h"
#include "maru_virtio_touchscreen.h"
#include "maru_device_ids.h"
#include "mloop_event.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, touchscreen);


#define DEVICE_NAME "virtio-touchscreen"
#define MAX_TOUCH_EVENT_CNT 16

/* This structure must match the kernel definitions */
typedef struct EmulTouchEvent {
    uint16_t x, y, z;
    uint8_t state;
} EmulTouchEvent;

typedef struct TouchEventEntry {
    int index;
    EmulTouchEvent touch;

    QTAILQ_ENTRY(TouchEventEntry) node;
} TouchEventEntry;

static TouchEventEntry _events_buf[MAX_TOUCH_EVENT_CNT];
static QTAILQ_HEAD(, TouchEventEntry) events_queue =
    QTAILQ_HEAD_INITIALIZER(events_queue);

static unsigned int ringbuf_cnt; // _events_buf
static unsigned int queue_cnt; // events_queue


typedef struct TouchscreenState
{
    VirtIODevice vdev;
    VirtQueue *vq;

    DeviceState *qdev;
    QEMUPutMouseEntry *eh_entry;
} TouchscreenState;
TouchscreenState *ts;


/* lock for between communication thread and IO thread */
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;


void virtio_touchscreen_event(void *opaque, int x, int y, int z, int buttons_state)
{
    TouchEventEntry *entry = NULL;

    pthread_mutex_lock(&event_mutex);

    if (queue_cnt >= MAX_TOUCH_EVENT_CNT) {
        pthread_mutex_unlock(&event_mutex);
        INFO("full touch event queue, lose event\n", queue_cnt);
        return;
    }

    entry = &(_events_buf[ringbuf_cnt % MAX_TOUCH_EVENT_CNT]);
    ringbuf_cnt++;

    /* mouse event is copied into the queue */
    entry->index = ++queue_cnt; // 1 ~
    entry->touch.x = x;
    entry->touch.y = y;
    entry->touch.z = z;
    entry->touch.state = buttons_state;

    QTAILQ_INSERT_TAIL(&events_queue, entry, node);

    pthread_mutex_unlock(&event_mutex);

    mloop_evcmd_touch();

    INFO("touch event (%d) : x=%d, y=%d, z=%d, state=%d\n",
        entry->index, entry->touch.x, entry->touch.y, entry->touch.z, entry->touch.state);
}

static void maru_virtio_touchscreen_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    if (ts->eh_entry == NULL) {
        VirtQueueElement elem;
        int max_trkid = 0;

        virtqueue_pop(ts->vq, &elem);
        memcpy(&max_trkid, elem.in_sg[0].iov_base, sizeof(int));

        if (max_trkid > 0) {
            INFO("virtio touchscreen's maximum of tracking id = %d\n", max_trkid);

            /* register a event handler */
            ts->eh_entry = qemu_add_mouse_event_handler(virtio_touchscreen_event, ts, 1, "QEMU Virtio Touchscreen");
            qemu_activate_mouse_event_handler(ts->eh_entry);

            virtqueue_push(ts->vq, &elem, 0);
            virtio_notify(&(ts->vdev), ts->vq);
        } else {
            INFO("virtio touchscreen is not added to qemu mouse event handler\n");
        }
    }

}

void maru_virtio_touchscreen_notify(void)
{
    TouchEventEntry *entry = NULL;
    VirtQueueElement elem;

    INFO("maru_virtio_touchscreen_notify\n");

    /* get from virtio queue */
    virtqueue_pop(ts->vq, &elem);

    pthread_mutex_lock(&event_mutex);

    /* get from host event queue */
    entry = QTAILQ_FIRST(&events_queue);
    /* copy host event into virtio queue */
    memcpy(elem.in_sg[0].iov_base, &(entry->touch), sizeof(EmulTouchEvent));
    /* remove host event */
    QTAILQ_REMOVE(&events_queue, entry, node);
    queue_cnt--;

    pthread_mutex_unlock(&event_mutex);

    /* put into virtio queue */
    virtqueue_push(ts->vq, &elem, 0);
    /* notify to guest */
    virtio_notify(&(ts->vdev), ts->vq);
}

static uint32_t virtio_touchscreen_get_features(VirtIODevice *vdev, uint32_t request_features)
{
    // TODO:
    return request_features;
}

VirtIODevice *maru_virtio_touchscreen_init(DeviceState *dev)
{
    INFO("initialize the touchscreen device\n");

    ts = (TouchscreenState *)virtio_common_init(DEVICE_NAME,
        VIRTIO_ID_TOUCHSCREEN, 0 /*config_size*/, sizeof(TouchscreenState));

    if (ts == NULL) {
        ERR("failed to initialize the touchscreen device\n");
        return NULL;
    }

    ts->vdev.get_features = virtio_touchscreen_get_features;
    ts->vq = virtio_add_queue(&ts->vdev, 128, maru_virtio_touchscreen_handle);

    ts->qdev = dev;

    /* reset the event counter */
    queue_cnt = ringbuf_cnt = 0;

    return &(ts->vdev);
}

void maru_virtio_touchscreen_exit(VirtIODevice *vdev)
{
    INFO("exit the touchscreen device\n");

    if (ts->eh_entry) {
        qemu_remove_mouse_event_handler(ts->eh_entry);
    }

    virtio_cleanup(vdev);
}

