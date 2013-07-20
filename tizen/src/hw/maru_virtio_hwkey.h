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


#ifndef MARU_HWKEY_H_
#define MARU_HWKEY_H_

#include "ui/console.h"
#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_HWKEY "virtio-hwkey-device"
#define VIRTIO_HWKEY(obj) \
        OBJECT_CHECK(VirtIOHWKey, (obj), TYPE_VIRTIO_HWKEY)


typedef struct VirtIOHWKey
{
    VirtIODevice vdev;
    /* simply a queue into which buffers are posted
    by the guest for consumption by the host */
    VirtQueue *vq;

    QEMUBH *bh;
    DeviceState *qdev;
} VirtIOHWKey;

/* This structure must match the kernel definitions */
typedef struct EmulHWKeyEvent {
    uint8_t event_type;
    uint32_t keycode;
} EmulHWKeyEvent;


VirtIODevice *maru_virtio_hwkey_init(DeviceState *dev);
void maru_virtio_hwkey_exit(VirtIODevice *vdev);

void maru_hwkey_event(int event_type, int keycode);
void maru_virtio_hwkey_notify(void);

#endif /* MARU_HWKEY_H_ */
