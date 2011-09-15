/* 
 * Qemu virtio example 
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * Authors:
 *  Dongkyun Yun  dk77.yun@samsung.com
 *
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").      
 * You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the lice    nse agreement 
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties about the suitability 
 * of the software, either express or implied, including but not limited to the implied warranties of merchantability,     fitness for 
 * a particular purpose, or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee as 
 * a result of using, modifying or distributing this software or its derivatives.
 */

#include "hw.h"
#include "virtio.h"
#include "qemu-log.h"
#include <sys/time.h>

/* fixme: move to virtio-ex.h */
#define VIRTIO_ID_EX 10

/* Enable debug messages. */
#define VIRTIO_EX_DEBUG

#if defined(VIRTIO_EX_DEBUG)
#define logout(fmt, ...) \
	fprintf(stdout, "[virtio][%s][%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define logout(fmt, ...) ((void)0)
#endif


typedef struct VirtIOEX
{
	VirtIODevice vdev;
	VirtQueue *vq;
} VirtIOEX;

#define SIZE_OUT_HEADER (4*3)
#define SIZE_IN_HEADER 4

static int log_dump(char *buffer, int size)
{
	int i;
	unsigned char *ptr = (unsigned char*)buffer;

	logout("buffer[%p] size[%d] \n", buffer, size);
	logout("DATA BEGIN -------------- \n");

	for(i=0; i < size; i++)
	{
		fprintf(stdout, " %02x ", ptr[i]);
		if(i!=0 && (i+1)%64 == 0) {
			fprintf(stdout, "\n");
		}
		if(ptr[i] == 0x00){
			fprintf(stdout, "\n");
			break;
		}
	}
	logout("DATA END  -------------- \n");
	return 0;
}

static void virtio_ex_handle(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtQueueElement elem;

	logout("\n");

	while(virtqueue_pop(vq, &elem)) {
		int ret = 0;
		char *buffer, *ptr;

		logout("elem.out_sg[0].iov_len(%d) elem.in_sg[0].iov_len(%d) \n"
				, elem.out_sg[0].iov_len, elem.in_sg[0].iov_len);

		buffer = malloc(elem.out_sg[0].iov_len);
		ptr = buffer;
		memcpy(ptr, (char *)elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

		log_dump(buffer, elem.out_sg[0].iov_len);
		/* procedure */

		/* ping pong */
		ptr = buffer;
		memcpy((char *)elem.in_sg[0].iov_base, ptr, elem.in_sg[0].iov_len);

		ret = elem.in_sg[0].iov_len;

		free(buffer);

		virtqueue_push(vq, &elem, ret);
		virtio_notify(vdev, vq);
	}
}

static uint32_t virtio_ex_get_features(VirtIODevice *vdev, uint32_t f)
{
	logout("\n");
	return 0;
}

static void virtio_ex_save(QEMUFile *f, void *opaque)
{
	VirtIOEX *s = opaque;

	logout("\n");

	virtio_save(&s->vdev, f);
}

static int virtio_ex_load(QEMUFile *f, void *opaque, int version_id)
{
	VirtIOEX *s = opaque;
	logout("\n");

	if (version_id != 1)
		return -EINVAL;

	virtio_load(&s->vdev, f);
	return 0;
}

VirtIODevice *virtio_ex_init(DeviceState *dev)
{
	VirtIOEX *s = NULL;

	logout("\n");

	s = (VirtIOEX *)virtio_common_init("virtio-ex",
			VIRTIO_ID_EX,
			0, sizeof(VirtIOEX));
	if (!s)
		return NULL;

	s->vdev.get_features = virtio_ex_get_features;

	s->vq = virtio_add_queue(&s->vdev, 128, virtio_ex_handle);

	register_savevm(dev, "virtio-ex", -1, 1, virtio_ex_save, virtio_ex_load, s);

	return &s->vdev;
}

