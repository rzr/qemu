/*
 * Maru Virtual USB Touchscreen emulation.
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


#include "maru_touchscreen.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, touchscreen);


#define MAX_TOUCH_EVENT_CNT 128

//lock for between communication thread and main thread
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

static QTAILQ_HEAD(, TouchEventEntry) events_queue =
    QTAILQ_HEAD_INITIALIZER(events_queue);

static unsigned int event_cnt = 0;
static unsigned int _processed_buf_cnt = 0;
static TouchEventEntry _event_buf[MAX_TOUCH_EVENT_CNT];

/**
 * @brief : qemu touch(host mouse) event handler
 * @param opaque : state of device
 * @param x : X-axis value
 * @param y : Y-axis value
 * @param z : event id for multiple touch
 */
static void usb_touchscreen_event(void *opaque, int x, int y, int z, int buttons_state)
{
    TouchEventEntry *te;
    USBTouchscreenState *s = opaque;

    pthread_mutex_lock(&event_mutex);
    if (event_cnt >= MAX_TOUCH_EVENT_CNT) {
        pthread_mutex_unlock(&event_mutex);
        INFO("full touch event queue, lose event\n", event_cnt);
        return;
    }

    //using prepared memory
    te = &(_event_buf[_processed_buf_cnt % MAX_TOUCH_EVENT_CNT]);
    _processed_buf_cnt++;

    /* mouse event is copied into the queue */
    te->index = ++event_cnt;
    te->queue_packet.x = x;
    te->queue_packet.y = y;
    te->queue_packet.z = z;
    te->queue_packet.state = buttons_state;

    QTAILQ_INSERT_TAIL(&events_queue, te, node);
    s->changed = 1;
    pthread_mutex_unlock(&event_mutex);

    TRACE("touch event (%d) : x=%d, y=%d, z=%d, state=%d\n", te->index, x, y, z, buttons_state);
}

/**
 * @brief : fill the usb packet
 * @param s : state of device
 * @param buf : usb packet
 * @param len : size of packet
 */
static int usb_touchscreen_poll(USBTouchscreenState *s, uint8_t *buf, int len)
{
    USBEmulTouchscreenPacket *packet = (USBEmulTouchscreenPacket *)buf;

    if (s->mouse_grabbed == 0) {
        s->eh_entry = qemu_add_mouse_event_handler(usb_touchscreen_event, s, 1, "QEMU Virtual touchscreen");
        qemu_activate_mouse_event_handler(s->eh_entry);
        s->mouse_grabbed = 1;
    }

    if (len < EMUL_TOUCHSCREEN_PACKET_LEN) {
        return 0;
    }

    packet->x = s->dx & 0xffff;
    packet->y = s->dy & 0xffff;
    packet->z = s->dz & 0xffff;

    if (s->buttons_state == 0) {
        packet->state = 0;
    } else {
        packet->state = 1;
    }

    return EMUL_TOUCHSCREEN_PACKET_LEN;
}

static void usb_touchscreen_handle_reset(USBDevice *dev)
{
    USBTouchscreenState *s = (USBTouchscreenState *) dev;

    pthread_mutex_lock(&event_mutex);

    s->dx = 0;
    s->dy = 0;
    s->dz = 0;
    s->buttons_state = 0;

    event_cnt = 0;
    _processed_buf_cnt = 0;

    pthread_mutex_unlock(&event_mutex);
}

static int usb_touchscreen_handle_control(USBDevice *dev, USBPacket *p,
    int request, int value, int index, int length, uint8_t *data)
{
    return usb_desc_handle_control(dev, p, request, value, index, length, data);
}

/**
 * @brief : call by uhci frame timer
 * @param dev : state of device
 * @param p : usb packet
 */
static int usb_touchscreen_handle_data(USBDevice *dev, USBPacket *p)
{
    USBTouchscreenState *s = (USBTouchscreenState *) dev;
    uint8_t buf[p->iov.size];
    int ret = 0;

    switch (p->pid) {
    case USB_TOKEN_IN:
        if (p->devep == 1) {
            pthread_mutex_lock(&event_mutex);

            if (s->changed == 0) {
                pthread_mutex_unlock(&event_mutex);
                return USB_RET_NAK;
            }

            if (event_cnt != 0) {
                if (!QTAILQ_EMPTY(&events_queue)) {
                    TouchEventEntry *te = QTAILQ_FIRST(&events_queue);

                    s->dx = te->queue_packet.x;
                    s->dy = te->queue_packet.y;
                    s->dz = te->queue_packet.z;
                    s->buttons_state = te->queue_packet.state;

                    QTAILQ_REMOVE(&events_queue, te, node);
                    event_cnt--;
                    TRACE("processed touch event (%d) : x=%d, y=%d, z=%d, state=%d\n",
                        te->index, s->dx, s->dy, s->dz, s->buttons_state);

                    if (QTAILQ_EMPTY(&events_queue)) {
                        s->changed = 0;
                        TRACE("processed all touch events (%d)\n", event_cnt);
                    }
                }
            } else {
                s->changed = 0;
            }

            pthread_mutex_unlock(&event_mutex);

            memset(buf, 0, sizeof(buf) * (p->iov.size - 1));
            ret = usb_touchscreen_poll(s, buf, p->iov.size); //write event to packet
            usb_packet_copy(p, buf, ret);
            break;
        }
        /* Fall through */
    case USB_TOKEN_OUT:
    default:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static void usb_touchscreen_handle_destroy(USBDevice *dev)
{
    USBTouchscreenState *s = (USBTouchscreenState *) dev;

    if (s->mouse_grabbed == 1) {
        qemu_remove_mouse_event_handler(s->eh_entry);
        s->mouse_grabbed = 0;
    }
}

/**
 * @brief : initialize a touchscreen device
 * @param opaque : state of device
 */
static int usb_touchscreen_initfn(USBDevice *dev)
{
    USBTouchscreenState *s = DO_UPCAST(USBTouchscreenState, dev, dev);
    usb_desc_init(dev);

    pthread_mutex_lock(&event_mutex);
    s->changed = 1;
    pthread_mutex_unlock(&event_mutex);

    return 0;
}

/**
 * @brief : remove mouse handlers before loading
 * @param opaque : state of device
 */
static int touchscreen_pre_load(void *opaque)
{
    USBTouchscreenState *s = (USBTouchscreenState *)opaque;

    if (s->eh_entry) {
        qemu_remove_mouse_event_handler(s->eh_entry);
    }

    return 0;
}

static int touchscreen_post_load(void *opaque, int version_id)
{
    USBTouchscreenState *s = (USBTouchscreenState *)opaque;

    pthread_mutex_lock(&event_mutex);
    s->changed = 1;
    pthread_mutex_unlock(&event_mutex);

    if (s->mouse_grabbed == 1) {
        s->eh_entry = qemu_add_mouse_event_handler(usb_touchscreen_event, s, 1, "QEMU Virtual touchscreen");
        qemu_activate_mouse_event_handler(s->eh_entry);
    }

    return 0;
}

static VMStateDescription vmsd_usbdevice = {
    .name = "maru-touchscreen-usbdevice",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField []) {
        VMSTATE_UINT8(addr, USBDevice),
        VMSTATE_INT32(state, USBDevice),
        VMSTATE_END_OF_LIST()
    }
};

static VMStateDescription vmsd = {
    .name = "maru-touchscreen",
    .version_id = 2,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .pre_load = touchscreen_pre_load,
    .post_load = touchscreen_post_load,
    .fields = (VMStateField []) {
        VMSTATE_STRUCT(dev, USBTouchscreenState, 1, vmsd_usbdevice, USBDevice),
        VMSTATE_INT32(dx, USBTouchscreenState),
        VMSTATE_INT32(dy, USBTouchscreenState),
        VMSTATE_INT32(dz, USBTouchscreenState),
        VMSTATE_INT32(buttons_state, USBTouchscreenState),
        VMSTATE_INT8(mouse_grabbed, USBTouchscreenState),
        VMSTATE_INT8(changed, USBTouchscreenState),
        VMSTATE_END_OF_LIST()
    }
};

static struct USBDeviceInfo touchscreen_info = {
    .product_desc   = "QEMU Virtual Touchscreen",
    .qdev.name      = "usb-maru-touchscreen",
    .qdev.desc      = "QEMU Virtual Touchscreen",
    .usbdevice_name = "maru-touchscreen",
    .usb_desc       = &desc_touchscreen,
    .qdev.size      = sizeof(USBTouchscreenState),
    .qdev.vmsd      = &vmsd,
    .init           = usb_touchscreen_initfn,
    .handle_packet  = usb_generic_handle_packet,
    .handle_reset   = usb_touchscreen_handle_reset,
    .handle_control = usb_touchscreen_handle_control,
    .handle_data    = usb_touchscreen_handle_data,
    .handle_destroy = usb_touchscreen_handle_destroy,
};

/**
 * @brief : register a touchscreen device
 */
static void usb_touchscreen_register_devices(void)
{
    usb_qdev_register(&touchscreen_info);
}

device_init(usb_touchscreen_register_devices)
