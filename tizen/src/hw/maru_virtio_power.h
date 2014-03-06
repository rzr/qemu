/*
 * Virtio Power Device
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

#ifndef MARU_VIRTIO_POWER_H_
#define MARU_VIRTIO_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_POWER "virtio-power-device"
#define VIRTIO_POWER(obj) \
        OBJECT_CHECK(VirtIOPOWER, (obj), TYPE_VIRTIO_POWER)

enum power_types {
    power_type_capacity = 0,
    power_type_charge_full,
    power_type_charge_now,
    power_type_max
};

typedef struct VirtIOPOWER {
    VirtIODevice    vdev;
    VirtQueue       *vq;
    DeviceState     *qdev;

    QEMUBH          *bh;
} VirtIOPOWER;

void set_power_capacity(int capacity);
int get_power_capacity(void);
void set_power_charge_full(int full);
int get_power_charge_full(void);
void set_power_charge_now(int now);
int get_power_charge_now(void);

#ifdef __cplusplus
}
#endif

#endif
