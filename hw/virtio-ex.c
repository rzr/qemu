/*
 *  Virtio GL Device
 *
 *  Copyright (c) 2010 Intel Corporation
 *  Written by:
 *    Ian Molton <ian.molton@collabora.co.uk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

static void virtio_ex_handle(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtQueueElement elem;

	logout("\n");

	while(virtqueue_pop(vq, &elem)) {
		int i;
		int length = ((int*)elem.out_sg[0].iov_base)[1];
		int r_length = ((int*)elem.out_sg[0].iov_base)[2];
		int ret = 0;
		char *buffer, *ptr;
		char *r_buffer = NULL;
		int *i_buffer;

		if(length < SIZE_OUT_HEADER || r_length < SIZE_IN_HEADER)
			goto done;

		buffer = malloc(length);
		r_buffer = malloc(r_length);
		ptr = buffer;
		i_buffer = (int*)buffer;

		i = 0;
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

#if 0
		*(int*)r_buffer = decode_call_int(i_buffer[0], /* pid */
				buffer + SIZE_OUT_HEADER, /* command_buffer */
				i_buffer[1] - SIZE_OUT_HEADER, /* cmd buffer length */
				r_buffer + SIZE_IN_HEADER);    /* return buffer */
#endif


		free(buffer);

		if(r_length)
			ret = r_length;
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

		free(r_buffer);
done:
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

