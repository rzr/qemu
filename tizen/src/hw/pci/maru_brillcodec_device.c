/*
 * Virtual Codec Device
 *
 * Copyright (c) 2013 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "qemu/main-loop.h"

#include "hw/maru_device_ids.h"
#include "util/osutil.h"
#include "maru_brillcodec.h"
#include "debug_ch.h"

/* define debug channel */
MULTI_DEBUG_CHANNEL(qemu, brillcodec);

#define CODEC_DEVICE_NAME   "codec-pci"
#define CODEC_DEVICE_THREAD "codec-workthread"
#define CODEC_VERSION               2
#define CODEC_REG_SIZE              (256)
#define DEFAULT_WORKER_THREAD_CNT   8

enum codec_io_cmd {
    CODEC_CMD_API_INDEX             = 0x28,
    CODEC_CMD_CONTEXT_INDEX         = 0x2C,
    CODEC_CMD_DEVICE_MEM_OFFSET     = 0x34,
    CODEC_CMD_GET_THREAD_STATE      = 0x38,
    CODEC_CMD_GET_CTX_FROM_QUEUE    = 0x3C,
    CODEC_CMD_GET_DATA_FROM_QUEUE   = 0x40,
    CODEC_CMD_RELEASE_CONTEXT       = 0x44,
    CODEC_CMD_GET_VERSION           = 0x50,
    CODEC_CMD_GET_ELEMENT           = 0x54,
    CODEC_CMD_GET_CONTEXT_INDEX     = 0x58,
};

enum thread_state {
    CODEC_TASK_START    = 0,
    CODEC_TASK_END      = 0x1f,
};

static void maru_brill_codec_threads_create(MaruBrillCodecState *s)
{
    int index;
    QemuThread *pthread = NULL;

    TRACE("enter: %s\n", __func__);

    pthread = g_malloc(sizeof(QemuThread) * s->worker_thread_cnt);
    if (!pthread) {
        ERR("failed to allocate threadpool memory.\n");
        return;
    }

    qemu_cond_init(&s->threadpool.cond);
    qemu_mutex_init(&s->threadpool.mutex);

    s->is_thread_running = true;

    qemu_mutex_lock(&s->context_mutex);
    s->idle_thread_cnt = 0;
    qemu_mutex_unlock(&s->context_mutex);

    for (index = 0; index < s->worker_thread_cnt; index++) {
        qemu_thread_create(&pthread[index], CODEC_DEVICE_THREAD,
            maru_brill_codec_threads, (void *)s, QEMU_THREAD_JOINABLE);
    }

    s->threadpool.threads = pthread;

    TRACE("leave: %s\n", __func__);
}

static void maru_brill_codec_get_cpu_cores(MaruBrillCodecState *s)
{
    s->worker_thread_cnt = get_number_of_processors();
    if (s->worker_thread_cnt < DEFAULT_WORKER_THREAD_CNT) {
        s->worker_thread_cnt = DEFAULT_WORKER_THREAD_CNT;
    }

    TRACE("number of threads: %d\n", s->worker_thread_cnt);
}

static void maru_brill_codec_bh_callback(void *opaque)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;

    TRACE("enter: %s\n", __func__);

    qemu_mutex_lock(&s->context_queue_mutex);
    if (!QTAILQ_EMPTY(&codec_wq)) {
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("raise irq\n");
        pci_set_irq(&s->dev, 1);
        s->irq_raised = 1;
    } else {
        qemu_mutex_unlock(&s->context_queue_mutex);
        TRACE("codec_wq is empty!!\n");
    }

    TRACE("leave: %s\n", __func__);
}

static uint64_t maru_brill_codec_read(void *opaque,
                                        hwaddr addr,
                                        unsigned size)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;
    uint64_t ret = 0;

    switch (addr) {
    case CODEC_CMD_GET_THREAD_STATE:
        qemu_mutex_lock(&s->context_queue_mutex);
        if (s->irq_raised) {
            ret = CODEC_TASK_END;
            pci_set_irq(&s->dev, 0);
            s->irq_raised = 0;
        }
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("get thread_state. ret: %d\n", ret);
        break;

    case CODEC_CMD_GET_CTX_FROM_QUEUE:
    {
        DeviceMemEntry *head = NULL;

        qemu_mutex_lock(&s->context_queue_mutex);
        head = QTAILQ_FIRST(&codec_wq);
        if (head) {
            ret = head->ctx_id;
            QTAILQ_REMOVE(&codec_wq, head, node);
            entry[ret] = head;
            TRACE("get a elem from codec_wq. 0x%x\n", head);
        } else {
            ret = 0;
        }
        qemu_mutex_unlock(&s->context_queue_mutex);

        TRACE("get a head from a writequeue. head: %x\n", ret);
    }
        break;

    case CODEC_CMD_GET_VERSION:
        ret = CODEC_VERSION;
        TRACE("codec version: %d\n", ret);
        break;

    case CODEC_CMD_GET_ELEMENT:
        ret = maru_brill_codec_query_list(s);
        break;

    case CODEC_CMD_GET_CONTEXT_INDEX:
        ret = maru_brill_codec_get_context_index(s);
        TRACE("get context index: %d\n", ret);
        break;

    default:
        ERR("no avaiable command for read. %d\n", addr);
    }

    return ret;
}

static void maru_brill_codec_write(void *opaque, hwaddr addr,
                                    uint64_t value, unsigned size)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)opaque;

    switch (addr) {
    case CODEC_CMD_API_INDEX:
        TRACE("set codec_cmd value: %d\n", value);
        s->ioparam.api_index = value;
        maru_brill_codec_wakeup_threads(s, value);
        break;

    case CODEC_CMD_CONTEXT_INDEX:
        TRACE("set context_index value: %d\n", value);
        s->ioparam.ctx_index = value;
        break;

    case CODEC_CMD_DEVICE_MEM_OFFSET:
        TRACE("set mem_offset value: 0x%x\n", value);
        s->ioparam.mem_offset = value;
        break;

    case CODEC_CMD_RELEASE_CONTEXT:
    {
        int ctx_id = (int32_t)value;

        if (CONTEXT(s, ctx_id).occupied_thread) {
            CONTEXT(s, ctx_id).requested_close = true;
            INFO("make running thread to handle deinit\n");
        } else {
            maru_brill_codec_release_context(s, ctx_id);
        }
    }
        break;

    case CODEC_CMD_GET_DATA_FROM_QUEUE:
        maru_brill_codec_pop_writequeue(s, (uint32_t)value);
        break;

    default:
        ERR("no available command for write. %d\n", addr);
    }
}

static const MemoryRegionOps maru_brill_codec_mmio_ops = {
    .read = maru_brill_codec_read,
    .write = maru_brill_codec_write,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int maru_brill_codec_initfn(PCIDevice *dev)
{
    MaruBrillCodecState *s = DO_UPCAST(MaruBrillCodecState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    INFO("device initialization.\n");
    pci_config_set_interrupt_pin(pci_conf, 1);

    memory_region_init_ram(&s->vram, OBJECT(s), "maru_brill_codec.vram", CODEC_MEM_SIZE);
    s->vaddr = (uint8_t *)memory_region_get_ram_ptr(&s->vram);

    memory_region_init_io(&s->mmio, OBJECT(s), &maru_brill_codec_mmio_ops, s,
                        "maru_brill_codec.mmio", CODEC_REG_SIZE);

    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    qemu_mutex_init(&s->context_mutex);
    qemu_mutex_init(&s->context_queue_mutex);
    qemu_mutex_init(&s->ioparam_queue_mutex);

    maru_brill_codec_get_cpu_cores(s);
    maru_brill_codec_threads_create(s);

    // register a function to qemu bottom-halves to switch context.
    s->codec_bh = qemu_bh_new(maru_brill_codec_bh_callback, s);

    return 0;
}

static void maru_brill_codec_exitfn(PCIDevice *dev)
{
    MaruBrillCodecState *s = DO_UPCAST(MaruBrillCodecState, dev, dev);
    INFO("device exit\n");

    qemu_mutex_destroy(&s->context_mutex);
    qemu_mutex_destroy(&s->context_queue_mutex);
    qemu_mutex_destroy(&s->ioparam_queue_mutex);

    qemu_bh_delete(s->codec_bh);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);
}

static void maru_brill_codec_reset(DeviceState *d)
{
    MaruBrillCodecState *s = (MaruBrillCodecState *)d;
    INFO("device reset\n");

    s->irq_raised = 0;

    memset(&s->context, 0, sizeof(CodecContext) * CODEC_CONTEXT_MAX);
    memset(&s->ioparam, 0, sizeof(CodecParam));
}

static void maru_brill_codec_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = maru_brill_codec_initfn;
    k->exit = maru_brill_codec_exitfn;
    k->vendor_id = PCI_VENDOR_ID_TIZEN;
    k->device_id = PCI_DEVICE_ID_VIRTUAL_BRILL_CODEC;
    k->class_id = PCI_CLASS_OTHERS;
    dc->reset = maru_brill_codec_reset;
    dc->desc = "Virtual new codec device for Tizen emulator";
}

static TypeInfo codec_device_info = {
    .name          = CODEC_DEVICE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MaruBrillCodecState),
    .class_init    = maru_brill_codec_class_init,
};

static void codec_register_types(void)
{
    type_register_static(&codec_device_info);
}

type_init(codec_register_types)
