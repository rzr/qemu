/*
 * TIZEN ARM base board
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd.
 *
 * Based on maru_board.c, exynos4210.c and exynos4210_boards.c
 *
 * Author:
 *  Evgeny Voevodin <e.voevodin@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include "boards.h"
#include "arm-misc.h"
#include "sysbus.h"
#include "pci.h"
#include "maru_arm.h"
#include "i2c.h"
#include "exec-memory.h"

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
    #undef PRINT_DEBUG
    #define  PRINT_DEBUG(fmt, args...) \
        do { \
            fprintf(stderr, "  [%s:%d]   "fmt, __func__, __LINE__, ##args); \
        } while (0)
#else
    #define  PRINT_DEBUG(fmt, args...)  do {} while (0)
#endif

#ifndef DEBUG_LOG_PATH
#define DEBUG_LOG_PATH              "./debug.log"
#endif

#define EXYNOS4210_WM8994_ADDR      0x1A

int codec_init(PCIBus *bus);
int maru_camera_pci_init(PCIBus *bus);

static int maru_arm_board_id = 0xF3B;
static int maru_arm_board_smp_bootreg_addr = EXYNOS4210_SECOND_CPU_BOOTREG;
static unsigned long maru_arm_board_ram_size = 0x40000000;

static struct arm_boot_info maru_arm_board_binfo = {
    .loader_start     = EXYNOS4210_BASE_BOOT_ADDR,
    .smp_loader_start = EXYNOS4210_SMP_BOOT_ADDR,
    .nb_cpus          = EXYNOS4210_NCPUS,
    .write_secondary_boot = maru_arm_write_secondary,
};

static Exynos4210State *maru_arm_board_init_common(
        const char *kernel_filename,
        const char *kernel_cmdline,
        const char *initrd_filename)
{
    maru_arm_board_binfo.ram_size = maru_arm_board_ram_size;
    maru_arm_board_binfo.board_id = maru_arm_board_id;
    maru_arm_board_binfo.smp_bootreg_addr = maru_arm_board_smp_bootreg_addr;
    maru_arm_board_binfo.kernel_filename = kernel_filename;
    maru_arm_board_binfo.initrd_filename = initrd_filename;
    maru_arm_board_binfo.kernel_cmdline = kernel_cmdline;
    maru_arm_board_binfo.gic_cpu_if_addr =
                        EXYNOS4210_SMP_PRIVATE_BASE_ADDR + 0x100;

    PRINT_DEBUG("\n ram_size: %luMiB [0x%08lx]\n"
            " kernel_filename: %s\n"
            " kernel_cmdline: %s\n"
            " initrd_filename: %s\n",
            exynos4_board_ram_size[board_type] / 1048576,
            exynos4_board_ram_size[board_type],
            kernel_filename,
            kernel_cmdline,
            initrd_filename);

    return maru_arm_init(get_system_memory(), maru_arm_board_ram_size);
}

static void maru_arm_machine_init(ram_addr_t ram_size,
                        const char *boot_device,
                        const char *kernel_filename,
                        const char *kernel_cmdline,
                        const char *initrd_filename,
                        const char *cpu_model)
{
    Exynos4210State *s;
    DeviceState *dev, *i2c_dev;
    PCIBus *pci_bus;

    s = maru_arm_board_init_common(kernel_filename,
                kernel_cmdline, initrd_filename);

    /* WM8994 */
    i2c_dev = i2c_create_slave(s->i2c_if[1], "wm8994", EXYNOS4210_WM8994_ADDR);

    /* Audio */
    dev = qdev_create(s->i2s_bus[0], "exynos4210.audio");
    qdev_prop_set_ptr(dev, "wm8994", i2c_dev);
    qdev_init_nofail(dev);

    /* PCI config */
    dev = qdev_create(NULL, "tizen_vpci");
    s->vpci_bus = sysbus_from_qdev(dev);
    qdev_init_nofail(dev);
    sysbus_mmio_map(s->vpci_bus, 0, EXYNOS4210_VPCI_CFG_BASE_ADDR);
    sysbus_connect_irq(s->vpci_bus, 0, s->irq_table[exynos4210_get_irq(38, 0)]);
    sysbus_connect_irq(s->vpci_bus, 1, s->irq_table[exynos4210_get_irq(38, 1)]);
    sysbus_connect_irq(s->vpci_bus, 2, s->irq_table[exynos4210_get_irq(38, 2)]);
    sysbus_connect_irq(s->vpci_bus, 3, s->irq_table[exynos4210_get_irq(38, 3)]);
    pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci");

    pci_create_simple(pci_bus, -1, "pci-ohci");
    maru_camera_pci_init(pci_bus);
    codec_init(pci_bus);

    arm_load_kernel(first_cpu, &maru_arm_board_binfo);
}

static void maru_common_init(ram_addr_t ram_size,
                        const char *boot_device,
                        const char *kernel_filename,
                        const char *kernel_cmdline,
                        const char *initrd_filename,
                        const char *cpu_model)
{
// prepare for universal virtual board...
#if defined(TARGET_I386)
#elif defined(TARGET_ARM)
#endif
}
static void maru_arm_board_init(ram_addr_t ram_size,
                        const char *boot_device,
                        const char *kernel_filename,
                        const char *kernel_cmdline,
                        const char *initrd_filename,
                        const char *cpu_model)
{
    maru_arm_machine_init(ram_size, boot_device, kernel_filename,
                        kernel_cmdline, initrd_filename, cpu_model);
    maru_common_init(ram_size, boot_device, kernel_filename,
                        kernel_cmdline, initrd_filename, cpu_model);
}

static QEMUMachine maru_arm_machine = {
    .name = "maru-arm-machine",
    .desc = "maru board(ARM)",
    .init = maru_arm_board_init,
    .max_cpus = 255,
};

static void maru_machine_init(void)
{
#if defined(TARGET_ARM)
    qemu_register_machine(&maru_arm_machine);
#else
#error
#endif
}

machine_init(maru_machine_init);
