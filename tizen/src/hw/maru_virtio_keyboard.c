/*
 * Virtio Keyboard Device
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  GiWoong Kim <giwoong.kim@samsung.com>
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

#include "maru_device_ids.h"
#include "maru_virtio_keyboard.h"
#include "tizen/src/debug_ch.h"


MULTI_DEBUG_CHANNEL(qemu, virtio-kbd);

VirtQueueElement elem;

static void virtio_keyboard_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)vdev;
    int index = 0;

    if (virtio_queue_empty(vkbd->vq)) {
        INFO("virtqueue is empty.\n");
        return;
    }

    /* Get a queue buffer which is written by guest side. */
    do {
        index = virtqueue_pop(vq, &elem);
        TRACE("virtqueue pop.\n");
    } while (index < VIRTIO_KBD_QUEUE_SIZE);
}

void virtio_keyboard_notify(void *opaque)
{
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)opaque;
    EmulKbdEvent *kbdevt;
    int written_cnt = 0;

    if (!vkbd) {
        ERR("VirtIOKeyboard is NULL.\n");
        return;
    }

    TRACE("[Enter] virtqueue notifier.\n");

    if (!virtio_queue_ready(vkbd->vq)) {
        INFO("virtqueue is not ready.\n");
        return;
    }

    if (vkbd->kbdqueue.rptr == VIRTIO_KBD_QUEUE_SIZE) {
        vkbd->kbdqueue.rptr = 0;
    }

    qemu_mutex_lock(&vkbd->event_mutex);
    written_cnt = vkbd->kbdqueue.wptr;

    while ((written_cnt--)) {
        kbdevt = &vkbd->kbdqueue.kbdevent[vkbd->kbdqueue.rptr];

	if (((EmulKbdEvent*)(elem.in_sg[vkbd->kbdqueue.rptr].iov_base))->code != 0) {
	    TRACE("FIXME: virtio queue is full.\n");
	}

        /* Copy keyboard data into guest side. */
        TRACE("copy: keycode %d, type %d, elem_index %d\n",
            kbdevt->code, kbdevt->type, vkbd->kbdqueue.rptr);
        memcpy(elem.in_sg[vkbd->kbdqueue.rptr].iov_base, kbdevt, sizeof(EmulKbdEvent));
        memset(kbdevt, 0x00, sizeof(EmulKbdEvent));

        if (vkbd->kbdqueue.wptr > 0) {
            vkbd->kbdqueue.wptr--;
            TRACE("written_cnt: %d, wptr: %d, qemu_index: %d\n", written_cnt, vkbd->kbdqueue.wptr, vkbd->kbdqueue.rptr);
        }

        vkbd->kbdqueue.rptr++;
        if (vkbd->kbdqueue.rptr == VIRTIO_KBD_QUEUE_SIZE) {
            vkbd->kbdqueue.rptr = 0;
        }
    }
    qemu_mutex_unlock(&vkbd->event_mutex);

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

    if (!virtio_queue_ready(vkbd->vq)) {
        INFO("virtqueue is not ready.\n");
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
            case 28:    /* KP_Enter */
                kbdevt.code = 96;
                break;
            case 29:    /* Right Ctrl */
                kbdevt.code = 97;
                break;
            case 56:    /* Right Alt */
                kbdevt.code = 100;
                break;
            case 71:    /* Home */
                kbdevt.code = 102;
                break;
            case 72:    /* Up */
                kbdevt.code = 103;
                break;
            case 73:    /* Page Up */
                kbdevt.code = 104;
                break;
            case 75:    /* Left */
                kbdevt.code = 105;
                break;
            case 77:    /* Right */
                kbdevt.code = 106;
                break;
            case 79:    /* End */
                kbdevt.code = 107;
                break;
            case 80:    /* Down */
                kbdevt.code = 108;
                break;
            case 81:    /* Page Down */
                kbdevt.code = 109;
                break;
            case 82:    /* Insert */
                kbdevt.code = 110;
                break;
            case 83:    /* Delete */
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

    qemu_mutex_lock(&vkbd->event_mutex);
    memcpy(&vkbd->kbdqueue.kbdevent[(*index)++], &kbdevt, sizeof(kbdevt));
    TRACE("event: keycode %d, type %d, index %d.\n",
        kbdevt.code, kbdevt.type, ((*index) - 1));

    vkbd->kbdqueue.wptr++;
    qemu_mutex_unlock(&vkbd->event_mutex);

    TRACE("[Leave] input_event handler. cnt:%d\n", vkbd->kbdqueue.wptr);

    qemu_bh_schedule(vkbd->bh);
}

static uint32_t virtio_keyboard_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_keyboard_get_features.\n");
    return 0;
}

static void virtio_keyboard_bh(void *opaque)
{
    virtio_keyboard_notify(opaque);
}

static int virtio_keyboard_device_init(VirtIODevice *vdev)
{
    VirtIOKeyboard *vkbd;
    DeviceState *qdev = DEVICE(vdev);
    vkbd = VIRTIO_KEYBOARD(vdev);

    INFO("initialize virtio-keyboard device\n");
    virtio_init(vdev, TYPE_VIRTIO_KEYBOARD, VIRTIO_ID_KEYBOARD, 0);

    if (vdev == NULL) {
        ERR("failed to initialize device\n");
        return -1;
    }

    memset(&vkbd->kbdqueue, 0x00, sizeof(vkbd->kbdqueue));
    vkbd->extension_key = 0;
    qemu_mutex_init(&vkbd->event_mutex);

    vkbd->vq = virtio_add_queue(vdev, 128, virtio_keyboard_handle);
    vkbd->qdev = qdev;

    /* bottom half */
    vkbd->bh = qemu_bh_new(virtio_keyboard_bh, vkbd);

    /* register keyboard handler */
    vkbd->eh_entry = qemu_add_kbd_event_handler(virtio_keyboard_event, vkbd);
 
    return 0;
}

static int virtio_keyboard_device_exit(DeviceState *qdev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(qdev);
    VirtIOKeyboard *vkbd = (VirtIOKeyboard *)vdev;

    INFO("destroy device\n");

    qemu_remove_kbd_event_handler(vkbd->eh_entry);

    if (vkbd->bh) {
        qemu_bh_delete(vkbd->bh);
    }

    qemu_mutex_destroy(&vkbd->event_mutex);

    virtio_cleanup(vdev);

    return 0;
}

static void virtio_keyboard_device_reset(VirtIODevice *vdev)
{
    VirtIOKeyboard *vkbd;
    vkbd = VIRTIO_KEYBOARD(vdev);

    INFO("reset keyboard device\n");
    vkbd->kbdqueue.rptr = 0;
    vkbd->kbdqueue.index = 0;
}

static void virtio_keyboard_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    dc->exit = virtio_keyboard_device_exit;
    vdc->init = virtio_keyboard_device_init;
    vdc->reset = virtio_keyboard_device_reset;
    vdc->get_features = virtio_keyboard_get_features;
}

static const TypeInfo virtio_keyboard_info = {
    .name = TYPE_VIRTIO_KEYBOARD,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOKeyboard),
    .class_init = virtio_keyboard_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_keyboard_info);
}

type_init(virtio_register_types)

