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

#include "virtio-gl.h"

static void virtio_gl_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtQueueElement elem;
    int nkill = 1;

    TRACE("virtio_gl_handle called.\n");

    while (virtqueue_pop(vq, &elem)) {
        struct d_hdr *hdr = (struct d_hdr *)elem.out_sg[0].iov_base;
        ProcessStruct *process;
        int i, remain;
        int ret = 0;

        if (!elem.out_num) {
            ERR("Bad packet\n");
            virtqueue_push(vq, &elem, ret);
            virtio_notify(vdev, vq);
            return;
        }

        process = vmgl_get_process(hdr->pid);

        if (hdr->rq_l) {
            if (hdr->rrq_l) {
                if (process->rq) { /* Usually only the quit packet... */
                    free(process->rq);
                    free(process->rrq);
                }
                process->rq  = process->rq_p  = g_malloc(hdr->rq_l);
                process->rrq = process->rrq_p = g_malloc(hdr->rrq_l);
                process->rq_l  = hdr->rq_l - SIZE_OUT_HEADER;
                process->rrq_l = hdr->rrq_l;
#ifdef DEBUG_GLIO
                process->sum = hdr->sum;
#endif
            }

            i = 0;
            remain = process->rq_l - (process->rq_p - process->rq);
            while (remain && i < elem.out_num) {
                char *src = (char *)elem.out_sg[i].iov_base;
                int ilen = elem.out_sg[i].iov_len;
                int len = remain;

                if (i == 0) {
                    src += SIZE_OUT_HEADER;
                    ilen -= SIZE_OUT_HEADER;
                }

                if (len > ilen) {
                    len = ilen;
                }

                memcpy(process->rq_p, src, len);
                process->rq_p += len;
                remain -= len;
                i++;
            }

            if (process->rq_p >= process->rq + process->rq_l) {
#ifdef DEBUG_GLIO
                int sum = 0;
                for (i = 0; i < process->rq_l; i++) {
                    sum += process->rq[i];
                }
                if (sum != process->sum) {
                    ERR("Checksum fail\n");
                }
#endif

                *(int *)process->rrq = nkill = decode_call_int(process,
                        process->rq,      /* command_buffer */
                        process->rq_l,    /* cmd buffer length */
                        process->rrq + SIZE_IN_HEADER);    /* return buffer */

                g_free(process->rq);
                process->rq = NULL;

#ifdef DEBUG_GLIO
                sum = 0;
                for (i = SIZE_IN_BUFFER; i < process->rrq_l; i++) {
                    sum += process->rrq[i];
                }
                *(int *)(process->rrq + sizeof(int)) = sum;
#endif

            }
        }

        if (hdr->rrq_l && process->rq == NULL) {
            i = 0;
            remain = process->rrq_l - (process->rrq_p - process->rrq);
            while (remain && i < elem.in_num) {
                char *dst = elem.in_sg[i].iov_base;
                int len = remain;
                int ilen = elem.in_sg[i].iov_len;

                if (len > ilen) {
                    len = ilen;
                }

                memcpy(dst, process->rrq_p, len);
                process->rrq_p += len;
                remain -= len;
                ret += len;
                i++;
            }

            if (remain <= 0) {
                g_free(process->rrq);
                if (!nkill) {
                    gl_disconnect(process);
                }
            }
        }

        virtqueue_push(vq, &elem, ret);
        virtio_notify(vdev, vq);
    }
}

static uint32_t virtio_gl_get_features(VirtIODevice *vdev, uint32_t f)
{
    return 0;
}

/*
static void virtio_gl_save(QEMUFile *f, void *opaque)
{
    VirtIOGL *s = opaque;

    virtio_save(&s->vdev, f);
}

static int virtio_gl_load(QEMUFile *f, void *opaque, int version_id)
{
    VirtIOGL *s = opaque;

    if (version_id != 1) {
        return -EINVAL;
    }

    virtio_load(&s->vdev, f);
    return 0;
}
*/

static void virtio_gl_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGL *s = VIRTIO_GL(vdev);
    /* DeviceState *qdev = DEVICE(vdev); */
    if (s == NULL) {
        ERR("Failed to initialize virtio gl device.\n");
        return;
    }

    virtio_init(vdev, TYPE_VIRTIO_GL, VIRTIO_ID_GL, 0);

    /*
    register_savevm(qdev, TYPE_VIRTIO_GL, -1, 1,
                    virtio_gl_save, virtio_gl_load, s);
    */
    s->vq = virtio_add_queue(vdev, 128, virtio_gl_handle);
}

static void virtio_gl_device_unrealize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    INFO("destroy virtio-gl device\n");

    virtio_cleanup(vdev);
}

static void virtio_gl_class_init(ObjectClass *klass, void *data)
{
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->unrealize = virtio_gl_device_unrealize;
    vdc->realize = virtio_gl_device_realize;
    vdc->get_features = virtio_gl_get_features;
}

static const TypeInfo virtio_gl_info = {
    .name = TYPE_VIRTIO_GL,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOGL),
    .class_init = virtio_gl_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_gl_info);
}

type_init(virtio_register_types)

