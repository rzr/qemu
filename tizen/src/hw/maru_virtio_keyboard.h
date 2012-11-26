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

#ifndef VIRTIO_KEYBOARD_H_
#define VIRTIO_KEYBOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw.h"
#include "virtio.h"

VirtIODevice *virtio_keyboard_init(DeviceState *dev);

void virtio_keyboard_exit(VirtIODevice *vdev);

void virtio_keyboard_notify(void *opaque);

#ifdef __cplusplus
}
#endif

#endif /* VIRTIO_KEYBOARD_H_ */
