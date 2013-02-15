/*
 * Virtio EmulatorStatusMedium Device
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
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
#include "maru_virtio_esm.h"
#include "skin/maruskin_server.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-esm);

#define VIRTIO_ESM_DEVICE_NAME "virtio-guest-status-medium"

struct progress_info {
	uint16_t percentage;
};

typedef struct VirtIOEmulatorStatusMedium {
    VirtIODevice    vdev;
    VirtQueue       *vq;
    DeviceState     *qdev;
} VirtIO_ESM;

static VirtQueueElement elem;
struct progress_info progress;

static void virtio_esm_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIO_ESM *vesm = (VirtIO_ESM *)vdev;
    int index = 0;

    TRACE("virtqueue handler.\n");
    if (virtio_queue_empty(vesm->vq)) {
        INFO("virtqueue is empty.\n");
        return;
    }

    // Get a queue buffer.
    index = virtqueue_pop(vq, &elem);
    TRACE("virtqueue pop. index: %d\n", index);

    TRACE("virtio element out number : %d\n", elem.out_num);
    if (elem.out_num != 1) {
        ERR("virtio element out number is wierd.");
    }
    else {
        TRACE("caramis elem.out_sg[0].iov_len : %x\n", elem.out_sg[0].iov_len);
        TRACE("caramis elem.out_sg[0].iov_base : %x\n", elem.out_sg[0].iov_base);
        if (elem.out_sg[0].iov_len != 2) {
            ERR("out lenth is wierd.");
        }
        else {
            progress.percentage = *((uint16_t*)elem.out_sg[0].iov_base);
            INFO("boot up progress is [%u] percent done.\n", progress.percentage);
            // notify to skin
            notify_booting_progress(progress.percentage);
        }
    }

    // There is no data to copy into guest.
    virtqueue_push(vesm->vq, &elem, 0);
    virtio_notify(&vesm->vdev, vesm->vq);
}

static uint32_t virtio_esm_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_esm_get_features.\n");
    return 0;
}

VirtIODevice *virtio_esm_init(DeviceState *dev)
{
    VirtIO_ESM *vesm;
    INFO("initialize virtio-esm device\n");

    vesm = (VirtIO_ESM *)virtio_common_init("virtio-esm",
            VIRTIO_ID_ESM, 0, sizeof(VirtIO_ESM));
    if (vesm == NULL) {
        ERR("failed to initialize device\n");
        return NULL;
    }

    vesm->vdev.get_features = virtio_esm_get_features;
    vesm->vq = virtio_add_queue(&vesm->vdev, 1, virtio_esm_handle);
    vesm->qdev = dev;

    return &vesm->vdev;
}

void virtio_esm_exit(VirtIODevice *vdev)
{
    INFO("destroy device\n");

    virtio_cleanup(vdev);
}

