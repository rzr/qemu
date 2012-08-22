/*
 * Maru USB Touchscreen Device
 * Based on hw/usb-wacom.c:
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  GiWoong Kim <giwoong.kim@samsung.com>
 *  HyunJun Son <hj79.son@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 

#ifndef MARU_TOUCH_H_
#define MARU_TOUCH_H_

#include <pthread.h>
#include "hw.h"
#include "console.h"
#include "usb.h"
#include "usb-desc.h"


typedef struct USBTouchscreenState {
    USBDevice dev;
    QEMUPutMouseEntry *eh_entry;

    int32_t dx, dy, dz, buttons_state;
    int8_t mouse_grabbed;
    int8_t changed;
} USBTouchscreenState;

/* This structure must match the kernel definitions */
typedef struct USBEmulTouchscreenPacket {
    uint16_t x, y, z;
    uint8_t state;
} USBEmulTouchscreenPacket;


#define EMUL_TOUCHSCREEN_PACKET_LEN 7
#define TOUCHSCREEN_RESOLUTION_X 5040
#define TOUCHSCREEN_RESOLUTION_Y 3780

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIALNUMBER,
};

static const USBDescStrings desc_strings = {
    [STR_MANUFACTURER]     = "QEMU " QEMU_VERSION,
    [STR_PRODUCT]          = "Maru USB Touchscreen",
    [STR_SERIALNUMBER]     = "1",
};

static const USBDescIface desc_iface_touchscreen = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 1,
    .bInterfaceClass               = USB_CLASS_HID,
    .bInterfaceSubClass            = 0x01, /* boot */
    .bInterfaceProtocol            = 0x02,
    .ndesc                         = 1,
    .descs = (USBDescOther[]) {
        {
            /* HID descriptor */
            .data = (uint8_t[]) {
                0x09,          /*  u8  bLength */
                0x21,          /*  u8  bDescriptorType */
                0x01, 0x10,    /*  u16 HID_class */
                0x00,          /*  u8  country_code */
                0x01,          /*  u8  num_descriptors */
                0x22,          /*  u8  type: Report */
                0x6e, 0,       /*  u16 len */
            },
        },
    },
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_INT,
            .wMaxPacketSize        = 8,
            .bInterval             = 0x0a,
        },
    },
};

static const USBDescDevice desc_device_touchscreen = {
    .bcdUSB                        = 0x0110,
    .bMaxPacketSize0               = EMUL_TOUCHSCREEN_PACKET_LEN + 1,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .bmAttributes          = 0x80,
            .bMaxPower             = 40,
            .nif = 1,
            .ifs = &desc_iface_touchscreen,
        },
    },
};

static const USBDesc desc_touchscreen = {
    .id = {
        .idVendor          = 0x056a,
        .idProduct         = 0x0000,
        .bcdDevice         = 0x0010,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_touchscreen,
    .str = desc_strings,
};

typedef struct TouchEventEntry {
    USBEmulTouchscreenPacket queue_packet;
    int index;

    /* used internally by qemu for handling mice */
    QTAILQ_ENTRY(TouchEventEntry) node;
} TouchEventEntry;

#endif /* MARU_TOUCH_H_ */
