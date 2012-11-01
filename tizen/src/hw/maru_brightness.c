/*
 * Maru brightness device for VGA
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
#ifdef TARGET_ARM
#include "console.h"
#endif
#include "pci.h"
#include "maru_device_ids.h"
#include "maru_brightness.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, maru_brightness);

#define QEMU_DEV_NAME           "MARU_BRIGHTNESS"

#define BRIGHTNESS_MEM_SIZE    (4 * 1024)    /* 4KB */
#define BRIGHTNESS_REG_SIZE    256

typedef struct BrightnessState {
    PCIDevice       dev;
    ram_addr_t      vram_offset;
    MemoryRegion    mmio_addr;
} BrightnessState;

enum {
    BRIGHTNESS_LEVEL = 0x00,
    BRIGHTNESS_OFF = 0x04,
};

uint32_t brightness_level = BRIGHTNESS_MAX;
uint32_t brightness_off;

/* level : 1 ~ 100, interval : 1 or 2 */
/* skip 100 level, set to default alpha */
uint8_t brightness_tbl[] = {100, /* level 0 : for dimming */
/* level 01 ~ 10 */         106, 108, 109, 111, 112, 114, 115, 117, 118, 120,
/* level 11 ~ 20 */         121, 123, 124, 126, 127, 129, 130, 132, 133, 175,
/* level 21 ~ 30 */         136, 138, 139, 141, 142, 144, 145, 147, 148, 150,
/* level 31 ~ 40 */         151, 153, 154, 156, 157, 159, 160, 162, 163, 165,
/* level 41 ~ 50 */         166, 168, 169, 171, 172, 174, 175, 177, 178, 180,
/* level 51 ~ 60 */         181, 183, 184, 186, 187, 189, 190, 192, 193, 195,
/* level 61 ~ 70 */         196, 198, 199, 201, 202, 204, 205, 207, 208, 210,
/* level 71 ~ 80 */         211, 213, 214, 216, 217, 219, 220, 222, 223, 225,
/* level 81 ~ 90 */         226, 228, 229, 231, 232, 234, 235, 237, 238, 240,
/* level 91 ~ 99 */         241, 243, 244, 246, 247, 249, 250, 252, 253};

static uint64_t brightness_reg_read(void *opaque,
                                    target_phys_addr_t addr,
                                    unsigned size)
{
    switch (addr & 0xFF) {
    case BRIGHTNESS_LEVEL:
        INFO("brightness_reg_read: brightness_level = %d\n", brightness_level);
        return brightness_level;
    case BRIGHTNESS_OFF:
        INFO("brightness_reg_read: brightness_off = %d\n", brightness_off);
        return brightness_off;
    default:
        ERR("wrong brightness register read - addr : %d\n", (int)addr);
        break;
    }

    return 0;
}

static void brightness_reg_write(void *opaque,
                                 target_phys_addr_t addr,
                                 uint64_t val,
                                 unsigned size)
{
    switch (addr & 0xFF) {
    case BRIGHTNESS_LEVEL:
#if BRIGHTNESS_MIN > 0
        if (val < BRIGHTNESS_MIN || val > BRIGHTNESS_MAX) {
#else
        if (val > BRIGHTNESS_MAX) {
#endif
            ERR("brightness_reg_write: Invalide brightness level.\n");
        } else {
            brightness_level = val;
            INFO("brightness_level : %lld\n", val);
#ifdef TARGET_ARM
            vga_hw_invalidate();
#endif
        }
        return;
    case BRIGHTNESS_OFF:
        INFO("brightness_off : %lld\n", val);
        brightness_off = val;
#ifdef TARGET_ARM
        vga_hw_invalidate();
#endif
        return;
    default:
        ERR("wrong brightness register write - addr : %d\n", (int)addr);
        break;
    }
}

static const MemoryRegionOps brightness_mmio_ops = {
    .read = brightness_reg_read,
    .write = brightness_reg_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int brightness_initfn(PCIDevice *dev)
{
    BrightnessState *s = DO_UPCAST(BrightnessState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_TIZEN);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);

    memory_region_init_io(&s->mmio_addr, &brightness_mmio_ops, s,
                            "maru_brightness_mmio", BRIGHTNESS_REG_SIZE);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio_addr);

    return 0;
}

/* external interface */
int pci_get_brightness(void)
{
    return brightness_level;
}

DeviceState *pci_maru_brightness_init(PCIBus *bus)
{
    return &pci_create_simple(bus, -1, QEMU_DEV_NAME)->qdev;
}

static void brightness_classinit(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->no_hotplug = 1;
    k->init = brightness_initfn;
}

static TypeInfo brightness_info = {
    .name = QEMU_DEV_NAME,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(BrightnessState),
    .class_init = brightness_classinit,
};

static void brightness_register_types(void)
{
    type_register_static(&brightness_info);
}

type_init(brightness_register_types);
