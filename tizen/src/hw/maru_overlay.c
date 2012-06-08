/* 
 * Maru overlay device for VGA
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * Hyunjun Son <hj79.son@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
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
#include "pc.h"
#include "pci.h"
#include "maru_pci_ids.h"
#include "maru_overlay.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, maru_overlay);

#define QEMU_DEV_NAME           "MARU_OVERLAY"

#define OVERLAY_MEM_SIZE	(8192 * 1024)	// 4MB(overlay0) + 4MB(overlay1)
#define OVERLAY_REG_SIZE	256
#define OVERLAY1_REG_OFFSET	(OVERLAY_REG_SIZE / 2)


enum {
	OVERLAY_POWER    = 0x00,
	OVERLAY_POSITION = 0x04,	// left top position
	OVERLAY_SIZE     = 0x08,	// width and height
};

uint8_t* overlay_ptr;

uint8_t overlay0_power;
uint16_t overlay0_left;
uint16_t overlay0_top;
uint16_t overlay0_width;
uint16_t overlay0_height;

uint8_t overlay1_power;
uint16_t overlay1_left;
uint16_t overlay1_top;
uint16_t overlay1_width;
uint16_t overlay1_height;

typedef struct OverlayState {
    PCIDevice       dev;

    MemoryRegion    mem_addr;
    MemoryRegion    mmio_addr;

} OverlayState;


static uint64_t overlay_reg_read(void *opaque, target_phys_addr_t addr, unsigned size)
{
    switch (addr) {
    case OVERLAY_POWER:
        return overlay0_power;
        break;
    case OVERLAY_SIZE:
        return overlay0_left | overlay0_top << 16;
        break;
    case OVERLAY_POSITION:
        return overlay0_width | overlay0_height << 16;
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_POWER:
        return overlay1_power;
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_SIZE:
        return overlay1_left | overlay1_top << 16;
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_POSITION:
        return overlay1_width | overlay1_height << 16;
        break;
    default:
        ERR("wrong overlay register read - addr : %d\n", (int)addr);
        break;
    }

    return 0;
}

static void overlay_reg_write(void *opaque, target_phys_addr_t addr, uint64_t val, unsigned size)
{
    switch (addr) {
    case OVERLAY_POWER:
        overlay0_power = val;
        if( !overlay0_power ) {
        	// clear the last overlay area.
        	memset( overlay_ptr, 0x00, ( OVERLAY_MEM_SIZE / 2 ) );
        }
        break;
    case OVERLAY_POSITION:
        overlay0_left = val & 0xFFFF;
        overlay0_top = val >> 16;
        break;
    case OVERLAY_SIZE:
        overlay0_width = val & 0xFFFF;
        overlay0_height = val >> 16;
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_POWER:
        overlay1_power = val;
        if( !overlay1_power ) {
        	// clear the last overlay area.
        	memset( overlay_ptr + OVERLAY1_REG_OFFSET , 0x00, ( OVERLAY_MEM_SIZE / 2 ) );
        }
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_POSITION:
        overlay1_left = val & 0xFFFF;
        overlay1_top = val >> 16;
        break;
    case OVERLAY1_REG_OFFSET + OVERLAY_SIZE:
        overlay1_width = val & 0xFFFF;
        overlay1_height = val >> 16;
        break;
    default:
        ERR("wrong overlay register write - addr : %d\n", (int)addr);
        break;
    }
}

static const MemoryRegionOps overlay_mmio_ops = {
    .read = overlay_reg_read,
    .write = overlay_reg_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int overlay_initfn(PCIDevice *dev)
{
    OverlayState *s = DO_UPCAST(OverlayState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_TIZEN);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_OVERLAY);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);

    memory_region_init_ram(&s->mem_addr, "maru_overlay.ram", OVERLAY_MEM_SIZE);
    overlay_ptr = memory_region_get_ram_ptr(&s->mem_addr);

    memory_region_init_io (&s->mmio_addr, &overlay_mmio_ops, s, "maru_overlay_mmio", OVERLAY_REG_SIZE);

    /* setup memory space */
    /* memory #0 device memory (overlay surface) */
    /* memory #1 memory-mapped I/O */
    pci_register_bar(&s->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->mem_addr);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio_addr);

    return 0;
}

DeviceState *pci_maru_overlay_init(PCIBus *bus)
{
    return &pci_create_simple(bus, -1, QEMU_DEV_NAME)->qdev;
}

static void overlay_classinit(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->no_hotplug = 1;
    k->init = overlay_initfn;
}

static TypeInfo overlay_info = {
    .name          = QEMU_DEV_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(OverlayState),
    .class_init    = overlay_classinit,
};

static void overlay_register_types(void)
{
    type_register_static(&overlay_info);
}

type_init(overlay_register_types);
