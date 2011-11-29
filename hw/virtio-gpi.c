/* 
 * Qemu virtio general purpose 
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

#include "debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, gpi);

/* fixme: move to virtio-gpi.h */
#define VIRTIO_ID_GPI 20

/* Enable debug messages. */
//#define VIRTIO_GPI_DEBUG

#if defined(VIRTIO_GPI_DEBUG)

static int log_dump(char *buffer, int size)
{
	TRACE("buffer[%p] size[%d] \n", buffer, size);

	int i;
	unsigned char *ptr = (unsigned char*)buffer;

	TRACE("DATA BEGIN -------------- \n");

	for(i=0; i < size; i++)
	{
		TRACE("[%d] %02x(%c) ",i,  ptr[i], ptr[i]);
	}

	TRACE("DATA END  -------------- \n");
	return 0;
}

#else
#define log_dump(fmt, ...) ((void)0)
#endif

int call_gpi(int pid, int call_num, char *in_args, int args_len, char *r_buffer, int r_length);

#define SIZE_GPI_HEADER (4*4)

typedef struct VirtIOGPI
{
	VirtIODevice vdev;
	VirtQueue *vq;
} VirtIOGPI;

static void virtio_gpi_handle(VirtIODevice *vdev, VirtQueue *vq)
{
	int i, ret = 0;
	int pid, length, r_length, ftn_num;
	char *buffer = NULL;
	char *r_buffer = NULL;
	char *ptr = NULL;
	int  *i_buffer = NULL;

	VirtQueueElement elem;

	while(virtqueue_pop(vq, &elem)) 
	{
		pid			= ((int*)elem.out_sg[0].iov_base)[0];
		length		= ((int*)elem.out_sg[0].iov_base)[1];
		r_length	= ((int*)elem.out_sg[0].iov_base)[2];
		ftn_num		= ((int*)elem.out_sg[0].iov_base)[3];

		TRACE("pid(%d) length(%d) r_length(%d) ftn num(%d) \n"
				, pid, length, r_length, ftn_num);

		if(length < SIZE_GPI_HEADER){
			ERR("wrong protocal \n");
			goto done;
		}

		/* alloc */
		buffer = qemu_mallocz(length);
		r_buffer = qemu_mallocz(r_length);

		i_buffer = (int*)buffer;

		/* get whole data */
		i = 0;
		ptr = buffer;
		while(length){
			int next = length;
			if(next > elem.out_sg[i].iov_len)
				next = elem.out_sg[i].iov_len;
			memcpy(ptr, (char *)elem.out_sg[i].iov_base, next);
			ptr += next;
			ret += next;
			i++;
			length -= next;
		}
		log_dump(buffer, ((int*)elem.out_sg[0].iov_base)[1]);

		/* procedure */
		call_gpi(i_buffer[0],					/* pid */
				ftn_num,						/* call function number */
				buffer + SIZE_GPI_HEADER,		/* command_buffer */
				i_buffer[1] - SIZE_GPI_HEADER,	/* cmd buffer length */
				r_buffer, 						/* return buffer length */
				r_length);						/* return buffer */

		log_dump(r_buffer, r_length);

		if(r_length)
			ret = r_length;

		/* put whole data */
		i = 0;
		ptr = r_buffer;
		while(r_length) {
			int next = r_length;
			if(next > elem.in_sg[i].iov_len)
				next = elem.in_sg[i].iov_len;
			memcpy(elem.in_sg[i].iov_base, ptr, next);
			ptr += next;
			r_length -= next;
			i++;
		}

		/* free */
		qemu_free(buffer);
		qemu_free(r_buffer);

done:
		virtqueue_push(vq, &elem, ret);
		virtio_notify(vdev, vq);
	}

	return;
}

static uint32_t virtio_gpi_get_features(VirtIODevice *vdev, uint32_t f)
{
	TRACE("virtio gpi get features: %x \n", f);
	return 0;
}

static void virtio_gpi_save(QEMUFile *f, void *opaque)
{
	VirtIOGPI *s = opaque;

	TRACE("virtio gpi save \n");

	virtio_save(&s->vdev, f);
}

static int virtio_gpi_load(QEMUFile *f, void *opaque, int version_id)
{
	VirtIOGPI *s = opaque;
	TRACE("virtio gpi load \n");

	if (version_id != 1)
		return -EINVAL;

	virtio_load(&s->vdev, f);
	return 0;
}

VirtIODevice *virtio_gpi_init(DeviceState *dev)
{
	VirtIOGPI *s = NULL;

	TRACE("initialize \n");

	s = (VirtIOGPI *)virtio_common_init("virtio-gpi",
			VIRTIO_ID_GPI,
			0, sizeof(VirtIOGPI));
	if (!s)
		return NULL;

	s->vdev.get_features = virtio_gpi_get_features;

	s->vq = virtio_add_queue(&s->vdev, 128, virtio_gpi_handle);

	register_savevm(dev, "virtio-gpi", -1, 1, virtio_gpi_save, virtio_gpi_load, s);

	return &s->vdev;
}

