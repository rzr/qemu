/* 
 * Qemu overlay emulator for VGA
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * Authors:
 *  Yuyeon Oh  yuyeon.oh@samsung.com
 *
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").  
 * You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the license agreement 
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties about the suitability 
 * of the software, either express or implied, including but not limited to the implied warranties of merchantability, fitness for 
 * a particular purpose, or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee as 
 * a result of using, modifying or distributing this software or its derivatives.
 */

#include "pc.h"
#include "pci.h"
#include "pci_ids.h"

#define PCI_VENDOR_ID_SAMSUNG		0x144d
#define PCI_DEVICE_ID_VIRTUAL_OVERLAY   0x1010

#define OVERLAY_MEM_SIZE	(8192 * 1024)	// 4MB(overlay0) + 4MB(overlay1)
#define OVERLAY_REG_SIZE	256
#define OVERLAY1_REG_OFFSET	(OVERLAY_REG_SIZE / 2)


enum {
	OVERLAY_POWER    = 0x00,
	OVERLAY_POSITION = 0x04,	// left top position
	OVERLAY_SIZE     = 0x08,	// width and height
};

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

uint8_t* overlay_ptr;	// pointer in qemu space

typedef struct OverlayState {
    PCIDevice dev;
    
    ram_addr_t	vram_offset;

    int overlay_mmio;

    uint32_t mem_addr;	// guest addr
    uint32_t mmio_addr;	// guest addr
} OverlayState;

static uint32_t overlay_reg_read(void *opaque, target_phys_addr_t addr)
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
        fprintf(stderr, "wrong overlay register read - addr : %d\n", addr);
    }


    return 0;
}

static void overlay_reg_write(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    switch (addr) {
    case OVERLAY_POWER:
        overlay0_power = val;
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
        fprintf(stderr, "wrong overlay register write - addr : %d\n", addr);
    }
}

static CPUReadMemoryFunc * const overlay_reg_readfn[3] = {
    overlay_reg_read,
    overlay_reg_read,
    overlay_reg_read,
};

static CPUWriteMemoryFunc * const overlay_reg_writefn[3] = {
    overlay_reg_write,
    overlay_reg_write,
    overlay_reg_write,
};

static void overlay_mem_map(PCIDevice *dev, int region_num,
			       pcibus_t addr, pcibus_t size, int type)
{
    OverlayState *s = DO_UPCAST(OverlayState, dev, dev);

    cpu_register_physical_memory(addr, OVERLAY_MEM_SIZE,
				 s->vram_offset);

    s->mem_addr = addr;
}

static void overlay_mmio_map(PCIDevice *dev, int region_num,
			       pcibus_t addr, pcibus_t size, int type)
{
    OverlayState *s = DO_UPCAST(OverlayState, dev, dev);
    
    cpu_register_physical_memory(addr, OVERLAY_REG_SIZE,
                                 s->overlay_mmio);
    
    s->mmio_addr = addr;
}

static int overlay_initfn(PCIDevice *dev)
{
    OverlayState *s = DO_UPCAST(OverlayState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_OVERLAY);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);

    s->vram_offset = qemu_ram_alloc(NULL, "overlay.vram", OVERLAY_MEM_SIZE);
    overlay_ptr = qemu_get_ram_ptr(s->vram_offset);

    s->overlay_mmio = cpu_register_io_memory(overlay_reg_readfn,
                                             overlay_reg_writefn, s,
                                             DEVICE_LITTLE_ENDIAN);
 
    /* setup memory space */
    /* memory #0 device memory (overlay surface) */
    /* memory #1 memory-mapped I/O */
    pci_register_bar(&s->dev, 0, OVERLAY_MEM_SIZE,
		     PCI_BASE_ADDRESS_MEM_PREFETCH, overlay_mem_map);

    pci_register_bar(&s->dev, 1, OVERLAY_REG_SIZE,
                     PCI_BASE_ADDRESS_SPACE_MEMORY, overlay_mmio_map);

    return 0;
}

int pci_overlay_init(PCIBus *bus)
{
    pci_create_simple(bus, -1, "overlay");
    return 0;
}

static PCIDeviceInfo overlay_info = {
    .qdev.name    = "overlay",
    .qdev.size    = sizeof(OverlayState),
    .no_hotplug   = 1,
    .init         = overlay_initfn,
};

static void overlay_register(void)
{
    pci_qdev_register(&overlay_info);
}

device_init(overlay_register);
