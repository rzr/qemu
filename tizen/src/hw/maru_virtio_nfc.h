/*
 * Virtio NFC Device
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  DaiYoung Kim <munkyu.im@samsung.com>
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

#ifndef MARU_VIRTIO_NFC_H_
#define MARU_VIRTIO_NFC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio.h"


/* device protocol */

#define __MAX_BUF_SIZE	1024

VirtIODevice *virtio_nfc_init(DeviceState *dev);

void virtio_nfc_exit(VirtIODevice *vdev);
bool send_to_nfc(enum request_cmd req, char* data, const uint32_t len);

#ifdef __cplusplus
}
#endif


#endif /* MARU_VIRTIO_NFC_H_ */
