/*
 * Maru Device IDs
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * DongKyun Yun
 * DoHyung Hong
 * Hyunjun Son
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


#ifndef MARU_DEVICE_IDS_H_
#define MARU_DEVICE_IDS_H_


/* PCI */
#define PCI_VENDOR_ID_TIZEN              0xC9B5
#define PCI_DEVICE_ID_VIRTUAL_OVERLAY    0x1010
#define PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS 0x1014
#define PCI_DEVICE_ID_VIRTUAL_CAMERA     0x1018
#define PCI_DEVICE_ID_VIRTUAL_CODEC      0x101C
/* Device ID 0x1000 through 0x103F inclusive is a virtio device */
#define PCI_DEVICE_ID_VIRTIO_TOUCHSCREEN 0x101D
#define PCI_DEVICE_ID_VIRTIO_KEYBOARD	 0x1020
#define PCI_DEVICE_ID_VIRTIO_ESM         0x1024

/* Virtio */
/*
+----------------------+--------------------+---------------+
| Subsystem Device ID  |   Virtio Device    | Specification |
+----------------------+--------------------+---------------+
+----------------------+--------------------+---------------+
|          1           |   network card     |  Appendix C   |
+----------------------+--------------------+---------------+
|          2           |   block device     |  Appendix D   |
+----------------------+--------------------+---------------+
|          3           |      console       |  Appendix E   |
+----------------------+--------------------+---------------+
|          4           |  entropy source    |  Appendix F   |
+----------------------+--------------------+---------------+
|          5           | memory ballooning  |  Appendix G   |
+----------------------+--------------------+---------------+
|          6           |     ioMemory       |       -       |
+----------------------+--------------------+---------------+
|          7           |       rpmsg        |  Appendix H   |
+----------------------+--------------------+---------------+
|          8           |     SCSI host      |  Appendix I   |
+----------------------+--------------------+---------------+
|          9           |   9P transport     |       -       |
+----------------------+--------------------+---------------+
|         10           |   mac80211 wlan    |       -       |
+----------------------+--------------------+---------------+
*/
#define VIRTIO_ID_TOUCHSCREEN   11
#define VIRTIO_ID_KEYBOARD      12
#define VIRTIO_ID_ESM           13

#endif /* MARU_DEVICE_IDS_H_ */
