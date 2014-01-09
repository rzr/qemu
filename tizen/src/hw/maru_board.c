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
#include "vigs/vigs_device.h"
#include "work_queue.h"
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

static bool has_pci_info = true;

MemoryRegion *global_ram_memory;

MemoryRegion *get_ram_memory(void)
{
    return global_ram_memory;
}

/* maru specialized device init */
static void maru_device_init(void)
{
    PCIBus *pci_bus = (PCIBus *) object_resolve_path_type("", TYPE_PCI_BUS, NULL);

#if defined(CONFIG_LINUX)
    XInitThreads();
    Display *display = XOpenDisplay(0);
    if (!display && !enable_spice) {
        fprintf(stderr, "Cannot open X display\n");
        exit(1);
    }
#else
    void *display = NULL;
#endif
    struct work_queue *render_queue = NULL;
    struct winsys_interface *vigs_wsi = NULL;

    pci_maru_overlay_init(pci_bus);
    pci_maru_brightness_init(pci_bus);
    maru_brill_codec_pci_device_init(pci_bus);

    if (enable_vigs || enable_yagl) {
        render_queue = work_queue_create();
    }

    if (enable_vigs) {
        PCIDevice *pci_dev = pci_create(pci_bus, -1, "vigs");
        qdev_prop_set_ptr(&pci_dev->qdev, "display", display);
        qdev_prop_set_ptr(&pci_dev->qdev, "render_queue", render_queue);
        qdev_init_nofail(&pci_dev->qdev);
        vigs_wsi = DO_UPCAST(VIGSDevice, pci_dev, pci_dev)->wsi;
    }

    if (enable_yagl) {
        PCIDevice *pci_dev = pci_create(pci_bus, -1, "yagl");
        qdev_prop_set_ptr(&pci_dev->qdev, "display", display);
        qdev_prop_set_ptr(&pci_dev->qdev, "render_queue", render_queue);
        if (vigs_wsi &&
            (strcmp(yagl_backend, "vigs") == 0) &&
            (strcmp(vigs_backend, "gl") == 0)) {
            qdev_prop_set_ptr(&pci_dev->qdev, "winsys_gl_interface", vigs_wsi);
        }
        qdev_init_nofail(&pci_dev->qdev);
    }
}

extern void pc_init_pci(QEMUMachineInitArgs *args);
static void maru_x86_board_init(QEMUMachineInitArgs *args)
{
    pc_init_pci(args);

    has_pci_info = false;
    maru_device_init();
}

static QEMUMachine maru_x86_machine = {
    PC_DEFAULT_MACHINE_OPTIONS,
    .name = "maru-x86-machine",
    .alias = "maru-x86-machine",
    .desc = "Maru Board (x86)",
    .init = maru_x86_board_init,
    .hot_add_cpu = pc_hot_add_cpu,
};

static void maru_machine_init(void)
{
    qemu_register_machine(&maru_x86_machine);
}

machine_init(maru_machine_init);
