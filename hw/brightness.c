/* 
 * Qemu brightness emulator
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

#define PCI_VENDOR_ID_SAMSUNG				0x144d
#define PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS    0x1014
#define QEMU_DEV_NAME						"brightness"

#define BRIGHTNESS_MEM_SIZE			(4 * 1024)		/* 4KB */
#define BRIGHTNESS_REG_SIZE			256

#define BRIGHTNESS_MIN				(0)
#define BRIGHTNESS_MAX				(10)

typedef struct BrightnessState {
    PCIDevice dev;
    
    ram_addr_t	offset;
    int brightness_mmio_io_addr;

    uint32_t ioport_addr;	// guest addr
    uint32_t mmio_addr;		// guest addr
} BrightnessState;

enum {
	BRIGHTNESS_LEVEL	= 0x00,
};

//uint8_t* brightness_ptr;	// pointer in qemu space
uint32_t brightness_level = 10;

//#define DEBUG_BRIGHTNESS

#if defined (DEBUG_BRIGHTNESS)
#  define DEBUG_PRINT(x) do { printf x ; } while (0)
#else
#  define DEBUG_PRINT(x)
#endif

static uint32_t brightness_reg_read(void *opaque, target_phys_addr_t addr)
{
    switch (addr & 0xFF) {
    case BRIGHTNESS_LEVEL:
    	DEBUG_PRINT(("brightness_reg_read: brightness_level = %d\n", brightness_level));
        return brightness_level;

    default:
        fprintf(stderr, "wrong brightness register read - addr : %d\n", addr);
    }

    return 0;
}

static void brightness_reg_write(void *opaque, target_phys_addr_t addr, uint32_t val)
{
	DEBUG_PRINT(("brightness_reg_write: addr = %d, val = %d\n", addr, val));

	if (val < BRIGHTNESS_MIN || val > BRIGHTNESS_MAX) {
		fprintf(stderr, "brightness_reg_write: Invalide brightness level.\n");
	}

    switch (addr & 0xFF) {
    case BRIGHTNESS_LEVEL:
    	brightness_level = val;
        return;
    default:
        fprintf(stderr, "wrong brightness register write - addr : %d\n", addr);
    }
}

static CPUReadMemoryFunc * const brightness_reg_readfn[3] = {
	brightness_reg_read,
	brightness_reg_read,
	brightness_reg_read,
};

static CPUWriteMemoryFunc * const brightness_reg_writefn[3] = {
	brightness_reg_write,
	brightness_reg_write,
	brightness_reg_write,
};

static void brightness_ioport_map(PCIDevice *dev, int region_num,
			       pcibus_t addr, pcibus_t size, int type)
{
	//BrightnessState *s = DO_UPCAST(BrightnessState, dev, dev);

    //cpu_register_physical_memory(addr, BRIGHTNESS_MEM_SIZE,
	//			 s->offset);

    //s->ioport_addr = addr;
}

static void brightness_mmio_map(PCIDevice *dev, int region_num,
			       pcibus_t addr, pcibus_t size, int type)
{
	BrightnessState *s = DO_UPCAST(BrightnessState, dev, dev);
    
    cpu_register_physical_memory(addr, BRIGHTNESS_REG_SIZE,
                                 s->brightness_mmio_io_addr);
    
    s->mmio_addr = addr;
}

static int brightness_initfn(PCIDevice *dev)
{
	BrightnessState *s = DO_UPCAST(BrightnessState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_SAMSUNG);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);

    //s->offset = qemu_ram_alloc(NULL, "brightness", BRIGHTNESS_MEM_SIZE);
    //brightness_ptr = qemu_get_ram_ptr(s->offset);

    s->brightness_mmio_io_addr = cpu_register_io_memory(brightness_reg_readfn,
                                             brightness_reg_writefn, s,
                                             DEVICE_LITTLE_ENDIAN);
 
    /* setup memory space */
    /* memory #0 device memory (overlay surface) */
    /* memory #1 memory-mapped I/O */
    //pci_register_bar(&s->dev, 0, BRIGHTNESS_MEM_SIZE,
    //		PCI_BASE_ADDRESS_SPACE_IO, brightness_ioport_map);

    pci_register_bar(&s->dev, 1, BRIGHTNESS_REG_SIZE,
                     PCI_BASE_ADDRESS_SPACE_MEMORY, brightness_mmio_map);

    return 0;
}

/* external interface */
int pci_get_brightness(void)
{
	return brightness_level;
}

int pci_brightness_init(PCIBus *bus)
{
    pci_create_simple(bus, -1, QEMU_DEV_NAME);
    return 0;
}

static PCIDeviceInfo brightness_info = {
    .qdev.name    = QEMU_DEV_NAME,
    .qdev.size    = sizeof(BrightnessState),
    .no_hotplug   = 1,
    .init         = brightness_initfn,
};

static void brightness_register(void)
{
    pci_qdev_register(&brightness_info);
}

device_init(brightness_register);
