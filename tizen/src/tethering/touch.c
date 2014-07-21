/*
 * emulator controller client
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  JiHye Kim <jihye1128.kim@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "emulator_common.h"
#include "emul_state.h"
#include "common.h"
#include "touch.h"
#include "genmsg/tethering.pb-c.h"
#include "ecs/ecs_tethering.h"
#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

typedef struct touch_state {
    bool is_touch_event;
    bool is_touch_supported;

    // int touch_max_point;
    // display_state *display;
} touch_event;

// static bool is_touch_event;
static int touch_device_status;

#ifndef DISPLAY_FEATURE
#include "encode_fb.h"

enum {
    ENCODE_WEBP = 0,
    ENCODE_PNG,
};

// static void set_touch_event_status(bool status);
static bool send_display_image_data(void);
#endif


#if 0
touch_state *init_touch_state(void)
{
    input_device_list *device = NULL;
    touch_state *touch = NULL;
    int ret = 0;

    device = g_malloc0(sizeof(device));
    if (!device) {
        return NULL;
    }

    touch = g_malloc0(sizeof(touch_state));
    if (!touch) {
        g_free(device);
        return NULL;
    }

    device->type = TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG;
    device->opaque = touch;

    ret = add_input_device(device);
    if (ret < 0) {
        g_free(touch);
        g_free(device);
    }

    return touch;
}
#endif

static bool build_touch_msg(Tethering__TouchMsg *touch)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG;
    msg.touchmsg = touch;

    LOG_TRACE("touch message size: %d\n", tethering__tethering_msg__get_packed_size(&msg));
    ret = send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_touch_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;

    Tethering__TouchMsg mt = TETHERING__TOUCH_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    mt.type = TETHERING__TOUCH_MSG__TYPE__START_ANS;
    mt.startans = &start_ans;

    ret = build_touch_msg(&mt);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_touch_max_count(void)
{
    bool ret = false;

    Tethering__TouchMsg mt = TETHERING__TOUCH_MSG__INIT;
    Tethering__TouchMaxCount touch_cnt =
        TETHERING__TOUCH_MAX_COUNT__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    touch_cnt.max = get_emul_max_touch_point();

    mt.type = TETHERING__TOUCH_MSG__TYPE__MAX_COUNT;
    mt.maxcount = &touch_cnt;

    LOG_TRACE("send touch max count: %d\n", touch_cnt.max);
    ret = build_touch_msg(&mt);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static void set_touch_data(Tethering__TouchData *data)
{
    float x = 0.0, y = 0.0;
    int32_t index = 0, state = 0;

    switch(data->state) {
    case TETHERING__TOUCH_STATE__PRESSED:
        LOG_TRACE("touch pressed\n");
        index = data->index;
        x = data->xpoint;
        y = data->ypoint;
        state = PRESSED;
        break;
    case TETHERING__TOUCH_STATE__RELEASED:
        LOG_TRACE("touch released\n");
        index = data->index;
        x = data->xpoint;
        y = data->ypoint;
        state = RELEASED;
        break;
    default:
        LOG_TRACE("invalid touch data\n");
        break;
    }

    LOG_TRACE("set touch_data. index: %d, x: %lf, y: %lf\n", index, x, y);
    send_tethering_touch_data(x, y, index, state);
}

static bool send_set_touch_resolution(void)
{
    bool ret = false;

    Tethering__TouchMsg mt = TETHERING__TOUCH_MSG__INIT;
    Tethering__Resolution resolution = TETHERING__RESOLUTION__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    resolution.width = get_emul_resolution_width();
    resolution.height = get_emul_resolution_height();

    mt.type = TETHERING__TOUCH_MSG__TYPE__RESOLUTION;
    mt.resolution = &resolution;

    LOG_TRACE("send touch resolution: %dx%d\n",
        resolution.width, resolution.height);
    ret = build_touch_msg(&mt);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

#if 0
static void set_touch_event_status(bool status)
{
    is_touch_event = status;

    LOG_TRACE("set touch_event status: %d\n", status);
}

static void set_hwkey_data(Tethering__HWKeyMsg *msg)
{
    int32_t keycode = 0;

    switch (msg->type) {
    case TETHERING__HWKEY_TYPE__MENU:
        // keycode = HARD_KEY_ ;
        break;

    case TETHERING__HWKEY_TYPE__HOME:
        keycode = HARD_KEY_HOME;
        break;

    case TETHERING__HWKEY_TYPE__BACK:
        // keycode = ;
        break;

    case TETHERING__HWKEY_TYPE__POWER:
        keycode = HARD_KEY_POWER;
        break;

    case TETHERING__HWKEY_TYPE__VOLUMEUP:
        keycode = HARD_KEY_VOL_UP;
        break;

    case TETHERING__HWKEY_TYPE__VOLUMEDOWN:
        keycode = HARD_KEY_VOL_DOWN;
        break;

    default:
        LOG_WARNING("undefined type: %d\n", msg->type);
    }

    LOG_TRACE("convert hwkey msg to keycode: %d\n", keycode);
    send_tethering_hwkey_data(keycode);
}
#endif

// bool msgproc_tethering_touch_msg(Tethering__TouchMsg *msg)
bool msgproc_tethering_touch_msg(void *message)
{
    bool ret = true;
    Tethering__TouchMsg *msg = (Tethering__TouchMsg *)message;

    // touch_state *state = NULL;

    switch(msg->type) {
    case TETHERING__TOUCH_MSG__TYPE__START_REQ:
        LOG_TRACE("TOUCH_MSG_TYPE_START\n");
        // state = init_touch_state();

        // it means that app starts to send touch values.
        // set_touch_event_status(true);

        send_set_touch_max_count();
        send_set_touch_resolution();

        ret = send_touch_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);
        break;
    case TETHERING__TOUCH_MSG__TYPE__TERMINATE:
        LOG_TRACE("TOUCH_MSG_TYPE_TERMINATE\n");

        // it means that app stops to send touch values.
        // set_touch_event_status(false);
        break;

    case TETHERING__TOUCH_MSG__TYPE__TOUCH_DATA:
        set_touch_data(msg->touchdata);
        break;

    case TETHERING__TOUCH_MSG__TYPE__DISPLAY_MSG:
        send_display_image_data();
        break;

#if 0
    case TETHERING__TOUCH_MSG__TYPE__HWKEY_MSG:
        set_hwkey_data(msg->hwkey);
        break;
#endif
    default:
        LOG_TRACE("invalid touch_msg\n");
        ret = false;
        break;
    }

    return ret;
}

int get_tethering_touch_status(void)
{
    return touch_device_status;
}

void set_tethering_touch_status(int status)
{
    touch_device_status = status;
    send_tethering_touch_status_ecp();
}

static bool send_display_image_data(void)
{
    bool ret = false;
    struct encode_mem *image = NULL;

    Tethering__TouchMsg touch = TETHERING__TOUCH_MSG__INIT;
    Tethering__DisplayMsg display = TETHERING__DISPLAY_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    image = (struct encode_mem *)encode_framebuffer(ENCODE_WEBP);
    if (!image) {
        LOG_SEVERE("failed to encode framebuffer\n");
        return false;
    }

    LOG_TRACE("image data size %d\n", image->length);
    display.has_imagedata = true;
    display.imagedata.len = image->length;
    display.imagedata.data = image->buffer;

#ifdef IMAGE_DUMP
    {
        FILE *fp = NULL;

        fp = fopen("test2.png", "wb");
        if (fp != NULL) {
            fwrite(image->buffer, 1, image->length, fp);
            fclose(fp);
        }
    }
#endif

    touch.type = TETHERING__TOUCH_MSG__TYPE__DISPLAY_MSG;
    touch.display = &display;

    // ret = build_display_msg(&display);
    ret = build_touch_msg(&touch);
    LOG_TRACE("send display message: %d\n", ret);

    g_free(image->buffer);
    g_free(image);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);
    return ret;
}
