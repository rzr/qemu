/*
 * Maru Virtio Touchscreen Device
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd All Rights Reserved
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


#ifndef MARU_TOUCHSCREEN_H_
#define MARU_TOUCHSCREEN_H_

#include "ui/console.h"
#include "hw/virtio/virtio.h"

typedef struct TouchscreenState
{
    VirtIODevice vdev;
    /* simply a queue into which buffers are posted
    by the guest for consumption by the host */
    VirtQueue *vq;
    bool waitBuf;

    QEMUBH *bh;
    DeviceState *qdev;
    QEMUPutMouseEntry *eh_entry;
} TouchscreenState;

/* This structure must match the kernel definitions */
typedef struct EmulTouchEvent {
    uint16_t x, y, z;
    uint8_t state;
} EmulTouchEvent;

VirtIODevice *maru_virtio_touchscreen_init(DeviceState *dev);
void maru_virtio_touchscreen_exit(VirtIODevice *vdev);


void virtio_touchscreen_event(void *opaque, int x, int y, int z, int buttons_state);
void maru_virtio_touchscreen_notify(void);

#endif /* MARU_TOUCHSCREEN_H_ */
