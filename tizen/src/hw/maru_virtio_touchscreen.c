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

/*
 * touch event queue
 */
typedef struct TouchEventEntry {
    unsigned int index;
    EmulTouchEvent touch;

    QTAILQ_ENTRY(TouchEventEntry) node;
} TouchEventEntry;

/* the maximum number of touch event that can be put into a queue */
#define MAX_TOUCH_EVENT_CNT 64

static TouchEventEntry _events_buf[MAX_TOUCH_EVENT_CNT];
static QTAILQ_HEAD(, TouchEventEntry) events_queue =
    QTAILQ_HEAD_INITIALIZER(events_queue);

static unsigned int event_ringbuf_cnt; /* _events_buf */
static unsigned int event_queue_cnt; /* events_queue */

/*
 * VirtQueueElement queue
 */
typedef struct ElementEntry {
    unsigned int index;
    unsigned int sg_index;
    VirtQueueElement elem;

    QTAILQ_ENTRY(ElementEntry) node;
} ElementEntry;

static ElementEntry _elem_buf[10];
static QTAILQ_HEAD(, ElementEntry) elem_queue =
    QTAILQ_HEAD_INITIALIZER(elem_queue);

static unsigned int elem_ringbuf_cnt; /* _elem_buf */
static unsigned int elem_queue_cnt; /* elem_queue */


TouchscreenState *ts;

/* lock for between communication thread and IO thread */
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t elem_mutex = PTHREAD_MUTEX_INITIALIZER;


void virtio_touchscreen_event(void *opaque, int x, int y, int z, int buttons_state)
{
    TouchEventEntry *entry = NULL;

    if (unlikely(event_queue_cnt >= MAX_TOUCH_EVENT_CNT)) {
        INFO("full touch event queue, lose event\n", event_queue_cnt);

        mloop_evcmd_touch();
        return;
    }

    entry = &(_events_buf[event_ringbuf_cnt % MAX_TOUCH_EVENT_CNT]);
    event_ringbuf_cnt++;

    /* mouse event is copied into the queue */
    entry->touch.x = x;
    entry->touch.y = y;
    entry->touch.z = z;
    entry->touch.state = buttons_state;

    pthread_mutex_lock(&event_mutex);

    entry->index = ++event_queue_cnt; // 1 ~

    QTAILQ_INSERT_TAIL(&events_queue, entry, node);

    pthread_mutex_unlock(&event_mutex);

    /* call maru_virtio_touchscreen_notify */
    mloop_evcmd_touch();

    TRACE("touch event (%d) : x=%d, y=%d, z=%d, state=%d\n",
        entry->index, entry->touch.x, entry->touch.y, entry->touch.z, entry->touch.state);
}

static void maru_virtio_touchscreen_handle(VirtIODevice *vdev, VirtQueue *vq)
{
#if 0 /* not used yet */
    if (ts->eh_entry == NULL) {
        void *vbuf = NULL;
        VirtQueueElement elem;
        int max_trkid = 0;

        virtqueue_pop(ts->vq, &elem);
        vbuf = elem.in_sg[0].iov_base;
        memcpy(&max_trkid, vbuf, sizeof(max_trkid));

        if (max_trkid > 0) {
            INFO("virtio touchscreen's maximum of tracking id = %d\n", max_trkid);

            /* register a event handler */
            ts->eh_entry = qemu_add_mouse_event_handler(
                virtio_touchscreen_event, ts, 1, "QEMU Virtio Touchscreen");
            qemu_activate_mouse_event_handler(ts->eh_entry);

            //TODO:
            virtqueue_push(ts->vq, &elem, 0);
            virtio_notify(&(ts->vdev), ts->vq);
        } else {
            INFO("virtio touchscreen is not added to qemu mouse event handler\n");
        }
    }
#endif

    int sg_index = 0;
    ElementEntry *elem_entry = NULL;

    if (unlikely(virtio_queue_empty(ts->vq))) {
        TRACE("virtqueue is empty\n");
        return;
    }

    while (true) {
        elem_entry = &(_elem_buf[elem_ringbuf_cnt % 10]);
        elem_ringbuf_cnt++;

        sg_index = virtqueue_pop(ts->vq, &elem_entry->elem);
        if (sg_index == 0) {
            elem_ringbuf_cnt--;
            break;
        } else if (sg_index < 0) {
            ERR("virtqueue is broken\n");
            elem_ringbuf_cnt--;
            return;
        }

        pthread_mutex_lock(&elem_mutex);

        elem_entry->index = ++elem_queue_cnt;
        elem_entry->sg_index = (unsigned int)sg_index;

        /* save VirtQueueElement */
        QTAILQ_INSERT_TAIL(&elem_queue, elem_entry, node);

        if (ts->waitBuf == true) {
            ts->waitBuf = false;
            mloop_evcmd_touch(); // call maru_virtio_touchscreen_notify
        }

        pthread_mutex_unlock(&elem_mutex);
    }
}

void maru_virtio_touchscreen_notify(void)
{
    ElementEntry *elem_entry = NULL;
    unsigned int ii = 0;

    TRACE("maru_virtio_touchscreen_notify\n");

    if (unlikely(!virtio_queue_ready(ts->vq))) {
        ERR("virtio queue is not ready\n");
        return;
    }

    while (true) {
        if (event_queue_cnt == 0) {
            TRACE("no event\n");
            break;
        } else if (elem_queue_cnt == 0) {
            TRACE("no buffer\n");

            pthread_mutex_lock(&elem_mutex);
            /* maybe next time */
            ts->waitBuf = true;
            pthread_mutex_unlock(&elem_mutex);
            break;
        }

        elem_entry = QTAILQ_FIRST(&elem_queue);

        if (elem_entry->sg_index > 0) {
            TouchEventEntry *event_entry = NULL;
            VirtQueueElement *elem = NULL;
            void *vbuf = NULL;

            elem = &elem_entry->elem;
            vbuf = elem->in_sg[elem_entry->sg_index - 1].iov_base;

            /* get touch event from host queue */
            event_entry = QTAILQ_FIRST(&events_queue);

            TRACE("touch(%d) : x=%d, y=%d, z=%d, state=%d | \
                event_queue_cnt=%d, elem.index=%d, elem.in_num=%d, sg_index=%d\n",
                event_entry->index, event_entry->touch.x, event_entry->touch.y,
                event_entry->touch.z, event_entry->touch.state,
                event_queue_cnt, elem->index, elem->in_num, elem_entry->sg_index);

            /* copy event into virtio buffer */
            memcpy(vbuf, &(event_entry->touch), sizeof(event_entry->touch));

            pthread_mutex_lock(&event_mutex);

            /* remove host event */
            QTAILQ_REMOVE(&events_queue, event_entry, node);
            event_queue_cnt--;

            pthread_mutex_unlock(&event_mutex);

            /* put buffer into virtio queue */
            virtqueue_fill(ts->vq, elem, sizeof(EmulTouchEvent), ii++);
        }

        pthread_mutex_lock(&elem_mutex);

        QTAILQ_REMOVE(&elem_queue, elem_entry, node);
        elem_queue_cnt--;

        pthread_mutex_unlock(&elem_mutex);
    }

    if (ii != 0) {
        /* signal other side */
        virtqueue_flush(ts->vq, ii);
        /* notify to guest */
        virtio_notify(&(ts->vdev), ts->vq);
    }
}

static uint32_t virtio_touchscreen_get_features(
    VirtIODevice *vdev, uint32_t request_features)
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
    ts->vq = virtio_add_queue(&ts->vdev, 64, maru_virtio_touchscreen_handle);

    ts->qdev = dev;

    /* reset the counters */
    event_queue_cnt = event_ringbuf_cnt = 0;
    elem_queue_cnt = elem_ringbuf_cnt = 0;

    ts->waitBuf = false;

#if 1
    /* register a event handler */
    ts->eh_entry = qemu_add_mouse_event_handler(
        virtio_touchscreen_event, ts, 1, "QEMU Virtio Touchscreen");
    qemu_activate_mouse_event_handler(ts->eh_entry);
    INFO("virtio touchscreen is added to qemu mouse event handler\n");
#endif

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

