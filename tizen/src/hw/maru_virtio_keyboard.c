/*
 * Virtio Keyboard Device
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
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

#include "console.h"
#include "mloop_event.h"
#include "maru_device_ids.h"
#include "maru_virtio_keyboard.h"
#include "tizen/src/debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-kbd);

#define VIRTIO_KBD_DEVICE_NAME "virtio-keyboard"
#define VIRTIO_KBD_QUEUE_SIZE  10

typedef struct EmulKbdEvent {
    uint16_t code;
    uint16_t type;
} EmulKbdEvent;

typedef struct VirtIOKbdQueue {
    EmulKbdEvent kbdevent[VIRTIO_KBD_QUEUE_SIZE];
    int index;
    int rptr, wptr;
} VirtIOKbdQueue;

typedef struct VirtIOKeyboard {
    VirtIODevice    vdev;
    VirtQueue       *vq;
    DeviceState     *qdev;
    uint16_t        extension_key;

    VirtIOKbdQueue  kbdqueue;
    QemuMutex       event_mutex;
} VirtIOKeyboard;

VirtQueueElement elem;

static void virtio_keyboard_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)vdev;
    int index = 0;

    TRACE("virtqueue handler.\n");
    if (virtio_queue_empty(vkbd->vq)) {
        INFO("virtqueue is empty.\n");
        return;
    }

    /* Get a queue buffer which is written by guest side. */
    do {
        index = virtqueue_pop(vq, &elem);
        TRACE("virtqueue pop. index: %d\n", index);
    } while(index < VIRTIO_KBD_QUEUE_SIZE);
}

void virtio_keyboard_notify(void *opaque)
{
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)opaque;
    EmulKbdEvent *kbdevt;
    int index = 0;
    int written_cnt = 0;
    int *rptr = NULL;

    if (!vkbd) {
        ERR("VirtIOKeyboard is NULL.\n");
        return;
    }

    // TODO : need to lock??
    qemu_mutex_lock(&vkbd->event_mutex);
    written_cnt = vkbd->kbdqueue.wptr;
    TRACE("[Enter] virtqueue notifier. %d\n", written_cnt);
    qemu_mutex_unlock(&vkbd->event_mutex);
    if (written_cnt < 0) {
        TRACE("there is no input data to copy to guest.\n");
        return;
    }
    rptr = &vkbd->kbdqueue.rptr;

    if (!virtio_queue_ready(vkbd->vq)) {
        INFO("virtqueue is not ready.\n");
        return;
    }

    if (vkbd->kbdqueue.rptr == VIRTIO_KBD_QUEUE_SIZE) {
        *rptr = 0;
    }

    while ((written_cnt--)) {
        index = *rptr;
        kbdevt = &vkbd->kbdqueue.kbdevent[index];

        /* Copy keyboard data into guest side. */
        TRACE("copy: keycode %d, type %d, index %d\n", kbdevt->code, kbdevt->type, index);
        memcpy(elem.in_sg[index].iov_base, kbdevt, sizeof(EmulKbdEvent));
        memset(kbdevt, 0x00, sizeof(EmulKbdEvent));

        qemu_mutex_lock(&vkbd->event_mutex);
        if(vkbd->kbdqueue.wptr > 0) {
            vkbd->kbdqueue.wptr--;
        }
        qemu_mutex_unlock(&vkbd->event_mutex);

        (*rptr)++;
        if (*rptr == VIRTIO_KBD_QUEUE_SIZE) {
            *rptr = 0;
        }
    }

    virtqueue_push(vkbd->vq, &elem, sizeof(EmulKbdEvent));
    virtio_notify(&vkbd->vdev, vkbd->vq);

    TRACE("[Leave] virtqueue notifier.\n");
}

static void virtio_keyboard_event(void *opaque, int keycode)
{
    EmulKbdEvent kbdevt;
    int *index = NULL;
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)opaque;

    if (!vkbd) {
        ERR("VirtIOKeyboard is NULL.\n");
        return;
    }

    index = &(vkbd->kbdqueue.index);
    TRACE("[Enter] input_event handler. cnt %d\n", vkbd->kbdqueue.wptr);

    if (*index < 0) {
        ERR("keyboard queue is overflow.\n");
        return;
    }

    if (*index == VIRTIO_KBD_QUEUE_SIZE) {
        *index = 0;
    }

    if (keycode < 0xe0) {
        if (vkbd->extension_key) {
            switch (keycode & 0x7f) {
            case 71:    // Home
                kbdevt.code = 102;
                break;
            case 72:    // Up
                kbdevt.code = 103;
                break;
            case 73:    // Page Up
                kbdevt.code = 104;
                break;
            case 75:    // Left
                kbdevt.code = 105;
                break;
            case 77:    // Right
                kbdevt.code = 106;
                break;
            case 79:    // End
                kbdevt.code = 107;
                break;
            case 80:    // Down
                kbdevt.code = 108;
                break;
            case 81:    // Page Down
                kbdevt.code = 109;
                break;
            case 82:    // Insert
                kbdevt.code = 110;
                break;
            case 83:    // Delete
                kbdevt.code = 111;
                break;
            default:
                WARN("There is no keymap for this keycode %d.\n", keycode);
            }
            vkbd->extension_key = 0;
        } else {
            kbdevt.code = keycode & 0x7f;
        }

        if (!(keycode & 0x80)) {
            kbdevt.type = 1;    /* KEY_PRESSED */
        } else {
            kbdevt.type = 0;    /* KEY_RELEASED */
        }
    } else {
        TRACE("Extension key.\n");
        kbdevt.code = keycode;
        vkbd->extension_key = 1;
    }

    memcpy(&vkbd->kbdqueue.kbdevent[(*index)++], &kbdevt, sizeof(kbdevt));
    TRACE("event: keycode %d, type %d, index %d.\n",
        kbdevt.code, kbdevt.type, ((*index) - 1));

    qemu_mutex_lock(&vkbd->event_mutex);
    vkbd->kbdqueue.wptr++;
    qemu_mutex_unlock(&vkbd->event_mutex);

    TRACE("[Leave] input_event handler. cnt:%d\n", vkbd->kbdqueue.wptr);
    mloop_evcmd_keyboard(vkbd);
}

static uint32_t virtio_keyboard_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_keyboard_get_features.\n");
    return 0;
}

VirtIODevice *virtio_keyboard_init(DeviceState *dev)
{
    VirtIOKeyboard *vkbd;
    INFO("initialize virtio-keyboard device\n");

    vkbd = (VirtIOKeyboard *)virtio_common_init(VIRTIO_KBD_DEVICE_NAME,
            VIRTIO_ID_KEYBOARD, 0, sizeof(VirtIOKeyboard));
    if (vkbd == NULL) {
        ERR("failed to initialize device\n");
        return NULL;
    }

    memset(&vkbd->kbdqueue, 0x00, sizeof(vkbd->kbdqueue));
    vkbd->extension_key = 0;
//    vkbd->attached = 1;
    qemu_mutex_init(&vkbd->event_mutex);


    vkbd->vdev.get_features = virtio_keyboard_get_features;
    vkbd->vq = virtio_add_queue(&vkbd->vdev, 64, virtio_keyboard_handle);
    vkbd->qdev = dev;

    /* register keyboard handler */
    qemu_add_kbd_event_handler(virtio_keyboard_event, vkbd);

    return &vkbd->vdev;
}

void virtio_keyboard_exit(VirtIODevice *vdev)
{
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)vdev;
    INFO("destroy device\n");

//    vkbd->attached = 0;
    virtio_cleanup(vdev);
}
