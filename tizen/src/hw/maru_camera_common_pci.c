/*
 * Common implementation of MARU Virtual Camera device by PCI bus.
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>

#include "qemu-common.h"
#include "cpu-common.h"

#include "pci.h"
#include "pci_ids.h"
#include "maru_device_ids.h"

#include "maru_camera_common.h"
#include "tizen/src/debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, camera_pci);

#define MARU_PCI_CAMERA_DEVICE_NAME     "maru_camera_pci"

#define MARUCAM_MEM_SIZE    (4 * 1024 * 1024)   /* 4MB */
#define MARUCAM_REG_SIZE    (256)               /* 64 * 4Byte */

/*
 *  I/O functions
 */
static inline uint32_t
marucam_mmio_read(void *opaque, target_phys_addr_t offset)
{
    uint32_t ret = 0;
    MaruCamState *state = (MaruCamState*)opaque;

    switch (offset & 0xFF) {
    case MARUCAM_CMD_ISR:
        qemu_mutex_lock(&state->thread_mutex);
        ret = state->isr;
        if (ret != 0) {
            qemu_irq_lower(state->dev.irq[2]);
            state->isr = 0;
        }
        qemu_mutex_unlock(&state->thread_mutex);
        break;
    case MARUCAM_CMD_G_DATA:
        ret = state->param->stack[state->param->top++];
        break;
    case MARUCAM_CMD_OPEN:
    case MARUCAM_CMD_CLOSE:
    case MARUCAM_CMD_START_PREVIEW:
    case MARUCAM_CMD_STOP_PREVIEW:
    case MARUCAM_CMD_S_PARAM:
    case MARUCAM_CMD_G_PARAM:
    case MARUCAM_CMD_ENUM_FMT:
    case MARUCAM_CMD_TRY_FMT:
    case MARUCAM_CMD_S_FMT:
    case MARUCAM_CMD_G_FMT:
    case MARUCAM_CMD_QCTRL:
    case MARUCAM_CMD_S_CTRL:
    case MARUCAM_CMD_G_CTRL:
    case MARUCAM_CMD_ENUM_FSIZES:
    case MARUCAM_CMD_ENUM_FINTV:
        ret = state->param->errCode;
        state->param->errCode = 0;
        break;
    default:
        ERR("Not supported command: 0x%x\n", offset);
        ret = EINVAL;
        break;
    }
    return ret;
}

static inline void
marucam_mmio_write(void *opaque, target_phys_addr_t offset, uint32_t value)
{
    MaruCamState *state = (MaruCamState*)opaque;
    
    switch(offset & 0xFF) {
    case MARUCAM_CMD_OPEN:
        marucam_device_open(state);
        break;
    case MARUCAM_CMD_CLOSE:
        marucam_device_close(state);
        break;
    case MARUCAM_CMD_START_PREVIEW:
        marucam_device_start_preview(state);
        break;
    case MARUCAM_CMD_STOP_PREVIEW:
        marucam_device_stop_preview(state);
        memset(state->vaddr, 0, MARUCAM_MEM_SIZE);
        break;
    case MARUCAM_CMD_S_PARAM:
        marucam_device_s_param(state);
        break;
    case MARUCAM_CMD_G_PARAM:
        marucam_device_g_param(state);
        break;
    case MARUCAM_CMD_ENUM_FMT:
        marucam_device_enum_fmt(state);
        break;
    case MARUCAM_CMD_TRY_FMT:
        marucam_device_try_fmt(state);
        break;
    case MARUCAM_CMD_S_FMT:
        marucam_device_s_fmt(state);
        break;
    case MARUCAM_CMD_G_FMT:
        marucam_device_g_fmt(state);
        break;
    case MARUCAM_CMD_QCTRL:
        marucam_device_qctrl(state);
        break;
    case MARUCAM_CMD_S_CTRL:
        marucam_device_s_ctrl(state);
        break;
    case MARUCAM_CMD_G_CTRL:
        marucam_device_g_ctrl(state);
        break;
    case MARUCAM_CMD_ENUM_FSIZES:
        marucam_device_enum_fsizes(state);
        break;
    case MARUCAM_CMD_ENUM_FINTV:
        marucam_device_enum_fintv(state);
        break;
    case MARUCAM_CMD_S_DATA:
        state->param->stack[state->param->top++] = value;
        break;
    case MARUCAM_CMD_DATACLR:
        memset(state->param, 0, sizeof(MaruCamParam));
        break;
    case MARUCAM_CMD_REQFRAME:
        qemu_mutex_lock(&state->thread_mutex);
        state->req_frame = value + 1;
        qemu_mutex_unlock(&state->thread_mutex);
        break;
    default:
        ERR("Not supported command: 0x%x\n", offset);
        break;
    }
}

static const MemoryRegionOps maru_camera_mmio_ops = {
    .old_mmio = {
        .read = {
            marucam_mmio_read,
            marucam_mmio_read,
            marucam_mmio_read,
        },
        .write = {
            marucam_mmio_write,
            marucam_mmio_write,
            marucam_mmio_write,
        },
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

/*
 *  QEMU bottom half funtion
 */
static void marucam_tx_bh(void *opaque)
{
    MaruCamState *state = (MaruCamState *)opaque;

    qemu_mutex_lock(&state->thread_mutex);
    if (state->isr) {
        qemu_irq_raise(state->dev.irq[2]);
    }
    qemu_mutex_unlock(&state->thread_mutex);
}

/*
 *  Initialization function
 */
static int marucam_initfn(PCIDevice *dev)
{
    MaruCamState *s = DO_UPCAST(MaruCamState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_interrupt_pin(pci_conf, 0x03);

    memory_region_init_ram(&s->vram, "marucamera.ram", MARUCAM_MEM_SIZE);
    s->vaddr = memory_region_get_ram_ptr(&s->vram);
    memset(s->vaddr, 0, MARUCAM_MEM_SIZE);

    memory_region_init_io (&s->mmio, &maru_camera_mmio_ops, s,
            "maru-camera-mmio", MARUCAM_REG_SIZE);
    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->vram);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    /* for worker thread */
    s->param = (MaruCamParam*)g_malloc0(sizeof(MaruCamParam));
    qemu_cond_init(&s->thread_cond);
    qemu_mutex_init(&s->thread_mutex);

    marucam_device_init(s);

    s->tx_bh = qemu_bh_new(marucam_tx_bh, s);
    INFO("[%s] camera device was initialized.\n", __func__);

    return 0;
}

/*
 *  Termination function
 */
static void marucam_exitfn(PCIDevice *pci_dev)
{
    MaruCamState *s =
        OBJECT_CHECK(MaruCamState, pci_dev, MARU_PCI_CAMERA_DEVICE_NAME);

    marucam_device_exit(s);
    g_free(s->param);
    qemu_cond_destroy(&s->thread_cond);
    qemu_mutex_destroy(&s->thread_mutex);

    memory_region_destroy(&s->vram);
    memory_region_destroy(&s->mmio);


    INFO("[%s] camera device was released.\n", __func__);
}

int maru_camera_pci_init(PCIBus *bus)
{
    INFO("[%s] camera device was initialized.\n", __func__);
    pci_create_simple(bus, -1, MARU_PCI_CAMERA_DEVICE_NAME);
    return 0;
}

static void maru_camera_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->no_hotplug = 1;
    k->init = marucam_initfn;
    k->exit = marucam_exitfn;
    k->vendor_id = PCI_VENDOR_ID_TIZEN;
    k->device_id = PCI_DEVICE_ID_VIRTUAL_CAMERA;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "MARU Virtual Camera device for Tizen emulator";
}

static TypeInfo maru_camera_info = {
    .name          = MARU_PCI_CAMERA_DEVICE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MaruCamState),
    .class_init    = maru_camera_pci_class_init,
};

static void maru_camera_pci_register_types(void)
{
    type_register_static(&maru_camera_info);
}

type_init(maru_camera_pci_register_types)
