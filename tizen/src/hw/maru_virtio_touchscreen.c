/*
 * Maru Virtual Virtio Touchscreen emulation
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
#include "maru_virtio_touchscreen.h"
#include "maru_pci_ids.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, touchscreen);


#define DEVICE_NAME "virtio-touchscreen"
#define MAX_TOUCH_EVENT_CNT 128

/* This structure must match the kernel definitions */
typedef struct EmulTouchState {
    uint16_t x, y, z;
    uint8_t state;
} EmulTouchState;

typedef struct TouchEventEntry {
    int index;
    EmulTouchState touch;

    QTAILQ_ENTRY(TouchEventEntry) node;
} TouchEventEntry;

static TouchEventEntry _events_buf[MAX_TOUCH_EVENT_CNT];
static QTAILQ_HEAD(, TouchEventEntry) events_queue =
    QTAILQ_HEAD_INITIALIZER(events_queue);

static unsigned int ringbuf_cnt; // _events_buf
static unsigned int queue_cnt; // events_queue


typedef struct TouchscreenState
{
    VirtIODevice *vdev;
    VirtQueue *vq;

    // TODO:
    unsigned int *queue_cnt;
} TouchscreenState;
TouchscreenState touchscreen_state;

typedef struct VirtIOTouchscreen
{
    VirtIODevice vdev;
    VirtQueue *vq;
    DeviceState *qdev;

    TouchscreenState *dev_state;
} VirtIOTouchscreen;


// lock for between communication thread and IO thread
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

void maru_mouse_to_touchevent(int x, int y, int z, int buttons_state)
{
    TouchEventEntry *te = NULL;

    pthread_mutex_lock(&event_mutex);

    if (queue_cnt >= MAX_TOUCH_EVENT_CNT) {
        pthread_mutex_unlock(&event_mutex);
        INFO("full touch event queue, lose event\n", queue_cnt);
        return;
    }

    te = &(_events_buf[ringbuf_cnt % MAX_TOUCH_EVENT_CNT]);
    ringbuf_cnt++;

    /* mouse event is copied into the queue */
    te->index = ++queue_cnt; // 1 ~
    te->touch.x = x;
    te->touch.y = y;
    te->touch.z = z;
    te->touch.state = buttons_state;

    QTAILQ_INSERT_TAIL(&events_queue, te, node);

    pthread_mutex_unlock(&event_mutex);

    TRACE("touch event (%d) : x=%d, y=%d, z=%d, state=%d\n",
            te->index, te->touch.x, te->touch.y, te->touch.z, te->touch.state);

    // TODO:

    virtio_notify(touchscreen_state.vdev, touchscreen_state.vq);
}

static void maru_virtio_touchscreen_handle(VirtIODevice *vdev, VirtQueue *vq)
{
}

static uint32_t virtio_touchscreen_get_features(VirtIODevice *vdev, uint32_t request_features)
{
    // TODO:
    return request_features;
}

VirtIODevice *maru_virtio_touchscreen_init(DeviceState *dev)
{
    VirtIOTouchscreen *s = NULL;

    INFO("initialize the touchscreen device\n");
    s = (VirtIOTouchscreen *)virtio_common_init(DEVICE_NAME,
        VIRTIO_ID_TOUCHSCREEN, 0 /*check*/, sizeof(VirtIOTouchscreen));

    if (s == NULL) {
        ERR("failed to initialize the touchscreen device\n");
        return NULL;
    }

    s->vdev.get_features = virtio_touchscreen_get_features;
    s->vq = virtio_add_queue(&s->vdev, 128 /*check*/, maru_virtio_touchscreen_handle);

    s->qdev = dev;

    touchscreen_state.vdev = &(s->vdev);
    touchscreen_state.vq = s->vq;
    s->dev_state = &touchscreen_state;
    /* reset the event counter */
    queue_cnt = ringbuf_cnt = 0;

    return &s->vdev;
}

void maru_virtio_touchscreen_exit(VirtIODevice *vdev)
{
    //VirtIOTouchscreen *s = DO_UPCAST(VirtIOTouchscreen, vdev, vdev);

    virtio_cleanup(vdev);
}

#if 0 // moved to virtio-pci.c
static void virtio_register_touchscreen(void)
{
    pci_qdev_register(virtio_info);
}

device_init(virtio_register_touchscreen);
#endif
