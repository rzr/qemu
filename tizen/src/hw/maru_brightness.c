/*
 * Maru brightness device for VGA
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "hw/i386/pc.h"
#ifdef TARGET_ARM
#include "ui/console.h"
#endif
#include "hw/pci/pci.h"
#include "maru_device_ids.h"
#include "maru_brightness.h"
#include "skin/maruskin_server.h"
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
pixman_color_t level_color;
pixman_image_t *brightness_image;

/* level : 1 ~ 100, interval : 1 or 2 */
/* skip 100 level, set to default alpha */
uint8_t brightness_tbl[] = {155, /* level 0 : for dimming */
/* level 01 ~ 10 */         149, 147, 146, 144, 143, 141, 140, 138, 137, 135,
/* level 11 ~ 20 */         134, 132, 131, 129, 128, 126, 125, 123, 122, 120,
/* level 21 ~ 30 */         119, 117, 116, 114, 113, 111, 110, 108, 107, 105,
/* level 31 ~ 40 */         104, 102, 101,  99,  98,  96,  95,  93,  92,  90,
/* level 41 ~ 50 */          89,  87,  86,  84,  83,  81,  80,  78,  77,  75,
/* level 51 ~ 60 */          74,  72,  71,  69,  68,  66,  65,  63,  62,  60,
/* level 61 ~ 70 */          59,  57,  56,  54,  53,  51,  50,  48,  47,  45,
/* level 71 ~ 80 */          44,  42,  41,  39,  38,  36,  35,  33,  32,  30,
/* level 81 ~ 90 */          29,  27,  26,  24,  23,  21,  20,  18,  17,  15,
/* level 91 ~ 99 */          14,  12,  11,   9,   8,   6,   5,   3,   2};

QEMUBH *bh;

static uint64_t brightness_reg_read(void *opaque,
                                    hwaddr addr,
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

static void maru_pixman_image_set_alpha(uint8_t value)
{
    if (brightness_image) {
        pixman_image_unref(brightness_image);
    }
    level_color.alpha = value << 8;
    brightness_image = pixman_image_create_solid_fill(&level_color);
}

static void brightness_reg_write(void *opaque,
                                 hwaddr addr,
                                 uint64_t val,
                                 unsigned size)
{
    switch (addr & 0xFF) {
    case BRIGHTNESS_LEVEL:
        INFO("brightness_level : %lld\n", val);
        if (brightness_level == val) {
            return;
        }
#if BRIGHTNESS_MIN > 0
        if (val < BRIGHTNESS_MIN || val > BRIGHTNESS_MAX) {
#else
        if (val > BRIGHTNESS_MAX) {
#endif
            ERR("brightness_reg_write: Invalide brightness level.\n");
        } else {
            brightness_level = val;
            maru_pixman_image_set_alpha(brightness_tbl[brightness_level]);
#ifdef TARGET_ARM
            graphic_hw_invalidate(NULL);
#endif
        }
        return;
    case BRIGHTNESS_OFF:
        INFO("brightness_off : %lld\n", val);
        if (brightness_off == val) {
            return;
        }
        brightness_off = val;
        if (brightness_off) {
            maru_pixman_image_set_alpha(0xFF); /* set black */
        } else {
            maru_pixman_image_set_alpha(brightness_tbl[brightness_level]);
        }

#ifdef TARGET_ARM
        graphic_hw_invalidate(NULL);
#endif

        /* notify to skin process */
        qemu_bh_schedule(bh);

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

static void brightness_exitfn(PCIDevice *dev)
{
    if (bh) {
        qemu_bh_delete(bh);
    }
    if (brightness_image) {
        pixman_image_unref(brightness_image);
    }
}

static void maru_brightness_bh(void *opaque)
{
    if (brightness_off == 0) {
        notify_brightness(true);
    } else {
        notify_brightness(false);
    }
}

static int brightness_initfn(PCIDevice *dev)
{
    BrightnessState *s = DO_UPCAST(BrightnessState, dev, dev);
    uint8_t *pci_conf = s->dev.config;

    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_TIZEN);
    pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_OTHER);

    memory_region_init_io(&s->mmio_addr, OBJECT(s), &brightness_mmio_ops, s,
                            "maru_brightness_mmio", BRIGHTNESS_REG_SIZE);
    pci_register_bar(&s->dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio_addr);

    bh = qemu_bh_new(maru_brightness_bh, s);

    level_color.alpha = 0x0000;
    level_color.red = 0x0000;
    level_color.green = 0x0000;
    level_color.blue = 0x0000;
    brightness_image = pixman_image_create_solid_fill(&level_color);

    return 0;
}

/* external interface */
DeviceState *pci_maru_brightness_init(PCIBus *bus)
{
    return &pci_create_simple(bus, -1, QEMU_DEV_NAME)->qdev;
}

static void brightness_classinit(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->no_hotplug = 1;
    k->init = brightness_initfn;
    k->exit = brightness_exitfn;
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
