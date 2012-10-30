/*
 * Samsung Tizen Virtual PCI driver
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *  Vorobiov Stanislav <s.vorobiov@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "sysbus.h"
#include "pci.h"
#include "pci_host.h"
#include "exec-memory.h"

#define TIZEN_VPCI_VENDOR_ID 0x10ee
#define TIZEN_VPCI_DEVICE_ID 0x0300
#define TIZEN_VPCI_CLASS_ID  PCI_CLASS_PROCESSOR_CO

#define TIZEN_VPCI_IO_ADDR 0xC3000000
#define TIZEN_VPCI_IO_SIZE 0xFFFF

typedef struct {
    SysBusDevice busdev;
    qemu_irq irq[4];
    MemoryRegion mem_config;
    MemoryRegion iospace_alias_mem;
} TizenVPCIState;

static uint64_t tizen_vpci_io_read(void *opaque, target_phys_addr_t addr,
                                   unsigned size)
{
    switch (size) {
    case 1:
        return cpu_inb(addr);
    case 2:
        return cpu_inw(addr);
    case 4:
        return cpu_inl(addr);
    }
    assert(0);
}

static void tizen_vpci_io_write(void *opaque, target_phys_addr_t addr,
                                uint64_t data, unsigned size)
{
    switch (size) {
    case 1:
        cpu_outb(addr, data);
        return;
    case 2:
        cpu_outw(addr, data);
        return;
    case 4:
        cpu_outl(addr, data);
        return;
    }
    assert(0);
}

static const MemoryRegionOps tizen_vpci_io_ops = {
    .endianness = DEVICE_LITTLE_ENDIAN,
    .read = tizen_vpci_io_read,
    .write = tizen_vpci_io_write
};

static inline uint32_t tizen_vpci_config_addr(target_phys_addr_t addr)
{
    return addr & 0xffffff;
}

static void tizen_vpci_config_write(void *opaque, target_phys_addr_t addr,
                                 uint64_t val, unsigned size)
{
    pci_data_write(opaque, tizen_vpci_config_addr(addr), val, size);
}

static uint64_t tizen_vpci_config_read(void *opaque, target_phys_addr_t addr,
                                    unsigned size)
{
    uint32_t val;
    val = pci_data_read(opaque, tizen_vpci_config_addr(addr), size);
    return val;
}

static const MemoryRegionOps tizen_vpci_config_ops = {
    .read = tizen_vpci_config_read,
    .write = tizen_vpci_config_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int tizen_vpci_map_irq(PCIDevice *d, int irq_num)
{
    return irq_num;
}

static void tizen_vpci_set_irq(void *opaque, int irq_num, int level)
{
    qemu_irq *pic = opaque;

    qemu_set_irq(pic[irq_num], level);
}

static int tizen_vpci_init(SysBusDevice *dev)
{
    TizenVPCIState *s = FROM_SYSBUS(TizenVPCIState, dev);
    PCIBus *bus;
    int i;

    for (i = 0; i < 4; i++) {
        sysbus_init_irq(dev, &s->irq[i]);
    }

    /* On ARM, we only have MMIO no specific IO space from the CPU
     * perspective.  In theory we ought to be able to embed the PCI IO
     * memory region direction in the system memory space.  However,
     * if any of the IO BAR subregions use the old_portio mechanism,
     * that won't be processed properly unless accessed from the
     * system io address space.  This hack to bounce things via
     * system_io works around the problem until all the users of
     * old_portion are updated */

    memory_region_init_io(&s->iospace_alias_mem, &tizen_vpci_io_ops, s,
                          "tizen_vpci.io-alias", TIZEN_VPCI_IO_SIZE);
    memory_region_add_subregion(get_system_memory(), TIZEN_VPCI_IO_ADDR,
                                &s->iospace_alias_mem);

    bus = pci_register_bus(&dev->qdev, "pci",
                           tizen_vpci_set_irq, tizen_vpci_map_irq, s->irq,
                           get_system_memory(), get_system_io(),
                           0, 4);

    memory_region_init_io(&s->mem_config, &tizen_vpci_config_ops, bus,
                          "tizen-vpci-config", 0x1000000);
    sysbus_init_mmio(dev, &s->mem_config);

    pci_create_simple(bus, -1, "tizen_vpci_host");

    return 0;
}

static int tizen_vpci_host_init(PCIDevice *d)
{
    pci_set_word(d->config + PCI_STATUS,
         PCI_STATUS_66MHZ | PCI_STATUS_DEVSEL_FAST);
    return 0;
}

static void tizen_vpci_host_class_init(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = tizen_vpci_host_init;
    k->vendor_id = TIZEN_VPCI_VENDOR_ID;
    k->device_id = TIZEN_VPCI_DEVICE_ID;
    k->class_id = TIZEN_VPCI_CLASS_ID;
}

static TypeInfo tizen_vpci_host_info = {
    .name          = "tizen_vpci_host",
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIDevice),
    .class_init    = tizen_vpci_host_class_init,
};

static void tizen_vpci_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = tizen_vpci_init;
}

static TypeInfo tizen_vpci_info = {
    .name          = "tizen_vpci",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(TizenVPCIState),
    .class_init    = tizen_vpci_class_init,
};

static void tizen_vpci_register_types(void)
{
    type_register_static(&tizen_vpci_info);
    type_register_static(&tizen_vpci_host_info);
}

type_init(tizen_vpci_register_types)
