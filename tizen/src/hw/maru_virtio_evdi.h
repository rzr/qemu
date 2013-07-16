/*
 * maru_virtio_evdi.h
 *
 *  Created on: 2013. 3. 30.
 *      Author: dykim
 */

#ifndef MARU_VIRTIO_EVDI_H_
#define MARU_VIRTIO_EVDI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_EVDI "virtio-evdi-device"
#define VIRTIO_EVDI(obj) \
        OBJECT_CHECK(VirtIOEVDI, (obj), TYPE_VIRTIO_EVDI)

typedef struct VirtIOEVDI {
    VirtIODevice    vdev;
    VirtQueue       *rvq;
    VirtQueue		*svq;
    DeviceState     *qdev;

    QEMUBH *bh;
} VirtIOEVDI;

VirtIODevice *virtio_evdi_init(DeviceState *dev);

void virtio_evdi_exit(VirtIODevice *vdev);

#ifdef __cplusplus
}
#endif


#endif /* MARU_VIRTIO_EVDI_H_ */
