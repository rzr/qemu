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

typedef struct EmulKbdEvent
{
	uint16_t code;
	uint16_t type;
} EmulKbdEvent;

typedef struct VirtIOKeyboard
{
    VirtIODevice    vdev;
    VirtQueue       *vq;
    DeviceState     *qdev;
	EmulKbdEvent	kbdevent;
} VirtIOKeyboard;

VirtQueueElement elem;

static void virtio_keyboard_handle (VirtIODevice *vdev, VirtQueue *vq)
{
	VirtIOKeyboard *vkbd = (VirtIOKeyboard *)vdev;

	INFO("virtqueue keyboard handler.\n");
	if (virtio_queue_empty(vkbd->vq)) {
		INFO("virtio_keyboard: virtqueue is empty.\n");
		return;
	}

	/* Get a queue buffer which is written by guest side. */
	virtqueue_pop(vq, &elem);
}

void virtio_keyboard_notify (void *opaque)
{
	VirtIOKeyboard *vkbd = (VirtIOKeyboard *)opaque;
	int len = 0;

	len = sizeof(EmulKbdEvent);

	INFO("virtio_keyboard_notify: code:%d, type:%d\n",
		vkbd->kbdevent.code, vkbd->kbdevent.type);

	if (!virtio_queue_ready(vkbd->vq)) {
		INFO("virtio_keyboard: virtqueue is not ready.\n");
		return;
	}

	/* Copy keyboard data into guest side. */
	memcpy(elem.in_sg[0].iov_base, &vkbd->kbdevent, len);

	virtqueue_push(vkbd->vq, &elem, len);
	virtio_notify(&vkbd->vdev, vkbd->vq);
}

static void virtio_keyboard_event (void *opaque, int keycode)
{
	VirtIOKeyboard *vkbd = opaque;

	if (!(keycode & 0x80)) {
		vkbd->kbdevent.type = 1;	/* KEY_PRESSED */
	} else {
		vkbd->kbdevent.type = 0;	/* KEY_RELEASED */
	}
	vkbd->kbdevent.code = keycode & 0x7f;

	INFO("virito_keycode_event: keycode:%d, type:%d\n",
		vkbd->kbdevent.code, vkbd->kbdevent.type);
	
	mloop_evcmd_keyboard(vkbd);
}

static uint32_t virtio_keyboard_get_features (VirtIODevice *vdev,
      		                                  uint32_t request_feature)
{
	INFO("virito_keycode_get_features.\n");
    return 0;
}

VirtIODevice *virtio_keyboard_init(DeviceState *dev)
{
    VirtIOKeyboard *kbd;
    INFO("initialize virtio keyboard device\n");

    kbd = (VirtIOKeyboard *)virtio_common_init(VIRTIO_KBD_DEVICE_NAME,
	        VIRTIO_ID_KEYBOARD, 0, sizeof(VirtIOKeyboard));

    if (kbd == NULL) {
        ERR("failed to initialize the touchscreen device\n");
        return NULL;
    }

    kbd->vdev.get_features = virtio_keyboard_get_features;
    kbd->vq = virtio_add_queue(&kbd->vdev, 64, virtio_keyboard_handle);
    kbd->qdev = dev;

	/* register keyboard handler */
	qemu_add_kbd_event_handler(virtio_keyboard_event, kbd);

    return &kbd->vdev;
}

void virtio_keyboard_exit(VirtIODevice *vdev)
{
    INFO("destroy virtio keyboard device\n");
    virtio_cleanup(vdev);
}
