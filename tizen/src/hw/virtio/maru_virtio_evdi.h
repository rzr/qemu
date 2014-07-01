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

/* device protocol */

#define __MAX_BUF_SIZE	1024

enum
{
	route_qemu = 0,
	route_control_server = 1,
	route_monitor = 2,
	route_ij = 3
};

typedef unsigned int CSCliSN;

typedef struct msg_info {
	char buf[__MAX_BUF_SIZE];

	uint32_t route;
	uint32_t use;
	uint16_t count;
	uint16_t index;

	CSCliSN cclisn;
}msg_info;

/* device protocol */

typedef struct VirtIOEVDI{
    VirtIODevice    vdev;
    VirtQueue       *rvq;
    VirtQueue		*svq;
    DeviceState     *qdev;

    QEMUBH *bh;
} VirtIOEVDI;



#define TYPE_VIRTIO_EVDI "virtio-evdi-device"
#define VIRTIO_EVDI(obj) \
        OBJECT_CHECK(VirtIOEVDI, (obj), TYPE_VIRTIO_EVDI)

//VirtIODevice *virtio_evdi_init(DeviceState *dev);

//void virtio_evdi_exit(VirtIODevice *vdev);
bool send_to_evdi(const uint32_t route, char* data, const uint32_t len);

#ifdef __cplusplus
}
#endif


#endif /* MARU_VIRTIO_EVDI_H_ */
