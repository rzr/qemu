/*
 * TIZEN base board
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * DongKyun Yun
 * DoHyung Hong
 * Hyunjun Son
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
 * x86 board from pc_piix.c...
 * add some TIZEN-speciaized device...
 */

#include <glib.h>

#include "hw/hw.h"
#include "hw/i386/pc.h"
#include "hw/i386/apic.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_ids.h"
#include "hw/usb.h"
#include "net/net.h"
#include "hw/boards.h"
#include "hw/ide.h"
#include "sysemu/kvm.h"
#include "hw/kvm/clock.h"
#include "sysemu/sysemu.h"
#include "hw/sysbus.h"
#include "hw/cpu/icc_bus.h"
#include "sysemu/arch_init.h"
#include "sysemu/blockdev.h"
#include "hw/i2c/smbus.h"
#include "hw/xen/xen.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "hw/acpi/acpi.h"
#include "cpu.h"
#ifdef CONFIG_XEN
#  include <xen/hvm/hvm_info_table.h>
#endif

#include "maru_common.h"
#include "guest_debug.h"
#include "maru_pm.h"
#include "maru_brightness.h"
#include "maru_overlay.h"
#if defined(__linux__)
#include <X11/Xlib.h>
#endif
#include "vigs_device.h"
extern int enable_yagl;
extern const char *yagl_backend;
extern int enable_vigs;
extern const char *vigs_backend;
extern int enable_spice;

#define MAX_IDE_BUS 2

int codec_init(PCIBus *bus);
int maru_brill_codec_pci_device_init(PCIBus *bus);

static const int ide_iobase[MAX_IDE_BUS] = { 0x1f0, 0x170 };
static const int ide_iobase2[MAX_IDE_BUS] = { 0x3f6, 0x376 };
static const int ide_irq[MAX_IDE_BUS] = { 14, 15 };

static bool has_pvpanic = true;
static bool has_pci_info = true;

MemoryRegion *global_ram_memory;

MemoryRegion *get_ram_memory(void)
{
    return global_ram_memory;
}

static void maru_x86_machine_init(MemoryRegion *system_memory,
                     MemoryRegion *system_io,
                     ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename,
                     const char *kernel_cmdline,
                     const char *initrd_filename,
                     const char *cpu_model,
                     int pci_enabled,
                     int kvmclock_enabled)
{
    int i;
    ram_addr_t below_4g_mem_size, above_4g_mem_size;
    PCIBus *pci_bus;
    ISABus *isa_bus;
    PCII440FXState *i440fx_state;
    int piix3_devfn = -1;
    qemu_irq *cpu_irq;
    qemu_irq *gsi;
    qemu_irq *i8259;
    qemu_irq *smi_irq;
    GSIState *gsi_state;
    DriveInfo *hd[MAX_IDE_BUS * MAX_IDE_DEVS];
    BusState *idebus[MAX_IDE_BUS];
    ISADevice *rtc_state;
    ISADevice *floppy;
    MemoryRegion *ram_memory;
    MemoryRegion *pci_memory;
    MemoryRegion *rom_memory;
    DeviceState *icc_bridge;
    void *fw_cfg = NULL;
    PcGuestInfo *guest_info;

#if defined(__linux__)
    Display *display = XOpenDisplay(0);
    if (!display && !enable_spice) {
        fprintf(stderr, "Cannot open X display\n");
        exit(1);
    }
#else
    void *display = NULL;
#endif
    struct winsys_interface *vigs_wsi = NULL;

    if (xen_enabled() && xen_hvm_init() != 0) {
        fprintf(stderr, "xen hardware virtual machine initialisation failed\n");
        exit(1);
    }

    icc_bridge = qdev_create(NULL, TYPE_ICC_BRIDGE);
    object_property_add_child(qdev_get_machine(), "icc-bridge",
                              OBJECT(icc_bridge), NULL);

    pc_cpus_init(cpu_model, icc_bridge);
    pc_acpi_init("acpi-dsdt.aml");

    if (kvmclock_enabled) {
        kvmclock_create();
    }

    if (ram_size >= QEMU_BELOW_4G_RAM_END ) {
        above_4g_mem_size = ram_size - QEMU_BELOW_4G_RAM_END;
        below_4g_mem_size = QEMU_BELOW_4G_RAM_END;
    } else {
        above_4g_mem_size = 0;
        below_4g_mem_size = ram_size;
    }

    if (pci_enabled) {
        pci_memory = g_new(MemoryRegion, 1);
        memory_region_init(pci_memory, NULL, "pci", INT64_MAX);
        rom_memory = pci_memory;
    } else {
        pci_memory = NULL;
        rom_memory = system_memory;
    }

    guest_info = pc_guest_info_init(below_4g_mem_size, above_4g_mem_size);
    guest_info->has_pci_info = has_pci_info;
    guest_info->isapc_ram_fw = !pci_enabled;

    /* allocate ram and load rom/bios */
    if (!xen_enabled()) {
	// W/A for allocate larger continuous heap.
        // see vl.c
        if(preallocated_ptr != NULL) {
            g_free(preallocated_ptr);
        }
	//
        fw_cfg = pc_memory_init(system_memory,
                       kernel_filename, kernel_cmdline, initrd_filename,
                       below_4g_mem_size, above_4g_mem_size,
                       rom_memory, &ram_memory, guest_info);
    }

    // for ramdump...
    global_ram_memory = ram_memory;

    gsi_state = g_malloc0(sizeof(*gsi_state));
    if (kvm_irqchip_in_kernel()) {
        kvm_pc_setup_irq_routing(pci_enabled);
        gsi = qemu_allocate_irqs(kvm_pc_gsi_handler, gsi_state,
                                 GSI_NUM_PINS);
    } else {
        gsi = qemu_allocate_irqs(gsi_handler, gsi_state, GSI_NUM_PINS);
    }

    if (pci_enabled) {
        pci_bus = i440fx_init(&i440fx_state, &piix3_devfn, &isa_bus, gsi,
                              system_memory, system_io, ram_size,
                              below_4g_mem_size,
                              0x100000000ULL - below_4g_mem_size,
                              above_4g_mem_size,
                              pci_memory, ram_memory);
    } else {
        pci_bus = NULL;
        i440fx_state = NULL;
        isa_bus = isa_bus_new(NULL, system_io);
        no_hpet = 1;
    }
    isa_bus_irqs(isa_bus, gsi);

    if (kvm_irqchip_in_kernel()) {
        i8259 = kvm_i8259_init(isa_bus);
    } else if (xen_enabled()) {
        i8259 = xen_interrupt_controller_init();
    } else {
        cpu_irq = pc_allocate_cpu_irq();
        i8259 = i8259_init(isa_bus, cpu_irq[0]);
    }

    for (i = 0; i < ISA_NUM_IRQS; i++) {
        gsi_state->i8259_irq[i] = i8259[i];
    }
    if (pci_enabled) {
        ioapic_init_gsi(gsi_state, "i440fx");
    }
    qdev_init_nofail(icc_bridge);

    pc_register_ferr_irq(gsi[13]);

    pc_vga_init(isa_bus, pci_enabled ? pci_bus : NULL);
    if (xen_enabled()) {
        pci_create_simple(pci_bus, -1, "xen-platform");
    }

    /* init basic PC hardware */
    pc_basic_device_init(isa_bus, gsi, &rtc_state, &floppy, xen_enabled());

    pc_nic_init(isa_bus, pci_bus);

    ide_drive_get(hd, MAX_IDE_BUS);
    if (pci_enabled) {
        PCIDevice *dev;
        if (xen_enabled()) {
            dev = pci_piix3_xen_ide_init(pci_bus, hd, piix3_devfn + 1);
        } else {
            dev = pci_piix3_ide_init(pci_bus, hd, piix3_devfn + 1);
        }
        idebus[0] = qdev_get_child_bus(&dev->qdev, "ide.0");
        idebus[1] = qdev_get_child_bus(&dev->qdev, "ide.1");
    } else {
        for(i = 0; i < MAX_IDE_BUS; i++) {
            ISADevice *dev;
            dev = isa_ide_init(isa_bus, ide_iobase[i], ide_iobase2[i],
                               ide_irq[i],
                               hd[MAX_IDE_DEVS * i], hd[MAX_IDE_DEVS * i + 1]);
            idebus[i] = qdev_get_child_bus(DEVICE(dev), "ide.0");
        }
    }

    pc_cmos_init(below_4g_mem_size, above_4g_mem_size, boot_device,
                 floppy, idebus[0], idebus[1], rtc_state);

    if (pci_enabled && usb_enabled(false)) {
        pci_create_simple(pci_bus, piix3_devfn + 2, "piix3-usb-uhci");
    }

    if (pci_enabled && acpi_enabled) {
        i2c_bus *smbus;

        smi_irq = qemu_allocate_irqs(pc_acpi_smi_interrupt, first_cpu, 1);
        /* TODO: Populate SPD eeprom data.  */
        smbus = piix4_pm_init(pci_bus, piix3_devfn + 3, 0xb100,
                              gsi[9], *smi_irq,
                              kvm_enabled(), fw_cfg);
        smbus_eeprom_init(smbus, 8, NULL, 0);
    }

    if (pci_enabled) {
        pc_pci_device_init(pci_bus);
    }

    if (has_pvpanic) {
        pvpanic_init(isa_bus);
    }

    /* maru specialized device init */
    if (pci_enabled) {
        codec_init(pci_bus);
        pci_maru_overlay_init(pci_bus);
        pci_maru_brightness_init(pci_bus);
//        maru_brill_codec_pci_device_init(pci_bus);
    }

    if (enable_vigs) {
        PCIDevice *pci_dev = pci_create(pci_bus, -1, "vigs");
        qdev_prop_set_ptr(&pci_dev->qdev, "display", display);
        qdev_init_nofail(&pci_dev->qdev);
        vigs_wsi = DO_UPCAST(VIGSDevice, pci_dev, pci_dev)->wsi;
    }

    if (enable_yagl) {
        PCIDevice *pci_dev = pci_create(pci_bus, -1, "yagl");
        qdev_prop_set_ptr(&pci_dev->qdev, "display", display);
        if (vigs_wsi &&
            (strcmp(yagl_backend, "vigs") == 0) &&
            (strcmp(vigs_backend, "gl") == 0)) {
            qdev_prop_set_ptr(&pci_dev->qdev, "winsys_gl_interface", vigs_wsi);
        }
        qdev_init_nofail(&pci_dev->qdev);
    }
}

static void maru_x86_board_init(QEMUMachineInitArgs *args)
{
    has_pci_info = false;

    ram_addr_t ram_size = args->ram_size;
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;
    const char *kernel_cmdline = args->kernel_cmdline;
    const char *initrd_filename = args->initrd_filename;
    const char *boot_device = args->boot_device;
    maru_x86_machine_init(get_system_memory(),
             get_system_io(),
             ram_size, boot_device,
             kernel_filename, kernel_cmdline,
             initrd_filename, cpu_model, 1, 1);
}

static QEMUMachine maru_x86_machine = {
    .name = "maru-x86-machine",
    .alias = "maru-x86-machine",
    .desc = "Maru Board (x86)",
    .init = maru_x86_board_init,
    .hot_add_cpu = pc_hot_add_cpu,
    .max_cpus = 255,
    DEFAULT_MACHINE_OPTIONS,
};

static void maru_machine_init(void)
{
    qemu_register_machine(&maru_x86_machine);
}

machine_init(maru_machine_init);
