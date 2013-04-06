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

#include "hw/virtio.h"

VirtIODevice *virtio_evdi_init(DeviceState *dev);

void virtio_evdi_exit(VirtIODevice *vdev);

#ifdef __cplusplus
}
#endif


#endif /* MARU_VIRTIO_EVDI_H_ */
