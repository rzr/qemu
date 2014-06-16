/*
 * Virtio Virtual Modem Device
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Sooyoung Ha <yoosah.ha@samsung.com>
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
 *  Sangho Park <sangho1206.park@samsung.com>
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

#ifndef MARU_VIRTIO_VMODEM_H_
#define MARU_VIRTIO_VMODEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio/virtio.h"

typedef struct VirtIOVirtualModem{
    VirtIODevice    vdev;
    VirtQueue       *rvq;
    VirtQueue       *svq;
    DeviceState     *qdev;

    QemuMutex mutex;
    QEMUBH *bh;
} VirtIOVModem;

#define TYPE_VIRTIO_VMODEM "virtio-vmodem-device"
#define VIRTIO_VMODEM(obj) \
        OBJECT_CHECK(VirtIOVModem, (obj), TYPE_VIRTIO_VMODEM)

bool send_to_vmodem(const uint32_t route, char *data, const uint32_t len);

#ifdef __cplusplus
}
#endif


#endif /* MARU_VIRTIO_VMODEM_H_ */
