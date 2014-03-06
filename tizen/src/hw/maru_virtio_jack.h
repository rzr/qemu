/*
 * Virtio Jack Device
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  Daiyoung Kim    <daiyoung777.kim@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
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

#ifndef MARU_VIRTIO_JACK_H_
#define MARU_VIRTIO_JACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_JACK "virtio-jack-device"
#define VIRTIO_JACK(obj) \
        OBJECT_CHECK(VirtIOJACK, (obj), TYPE_VIRTIO_JACK)

enum jack_types {
    jack_type_charger = 0,
    jack_type_earjack,
    jack_type_earkey,
    jack_type_hdmi,
    jack_type_usb,
    jack_type_max
};

typedef struct VirtIOJACK {
    VirtIODevice    vdev;
    VirtQueue       *vq;
    DeviceState     *qdev;

    QEMUBH          *bh;
} VirtIOJACK;

void set_jack_charger(int online);
int get_jack_charger(void);

void set_jack_usb(int online);
int get_jack_usb(void);

#ifdef __cplusplus
}
#endif

#endif
