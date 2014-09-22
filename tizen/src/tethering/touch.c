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
#include "encode_fb.h"
#include "genmsg/tethering.pb-c.h"
#include "ecs/ecs_tethering.h"
#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

static int touch_device_status;
static bool send_display_image_data(void);

#define HARD_KEY_MENU 169
#define HARD_KEY_BACK 158

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

static void set_hwkey_data(Tethering__HWKeyMsg *msg)
{
    int32_t keycode = 0;

    switch (msg->type) {
    case TETHERING__HWKEY_TYPE__MENU:
        keycode = HARD_KEY_MENU;
        break;

    case TETHERING__HWKEY_TYPE__HOME:
        keycode = HARD_KEY_HOME;
        break;

    case TETHERING__HWKEY_TYPE__BACK:
        keycode = HARD_KEY_BACK;
        break;

    case TETHERING__HWKEY_TYPE__POWER:
        keycode = HARD_KEY_POWER;
        break;

    case TETHERING__HWKEY_TYPE__VOLUME_UP:
        keycode = HARD_KEY_VOL_UP;
        break;

    case TETHERING__HWKEY_TYPE__VOLUME_DOWN:
        keycode = HARD_KEY_VOL_DOWN;
        break;

    default:
        LOG_WARNING("undefined type: %d\n", msg->type);
    }

    LOG_TRACE("convert hwkey msg to keycode: %d\n", keycode);
    send_tethering_hwkey_data(keycode);
}

bool msgproc_tethering_touch_msg(void *message)
{
    bool ret = true;
    Tethering__TouchMsg *msg = (Tethering__TouchMsg *)message;

    switch(msg->type) {
    case TETHERING__TOUCH_MSG__TYPE__START_REQ:
        LOG_TRACE("TOUCH_MSG_TYPE_START\n");
        send_set_touch_max_count();
        send_set_touch_resolution();
        ret = send_touch_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);
        break;
    case TETHERING__TOUCH_MSG__TYPE__TERMINATE:
        LOG_TRACE("TOUCH_MSG_TYPE_TERMINATE\n");
        break;

    case TETHERING__TOUCH_MSG__TYPE__TOUCH_DATA:
        LOG_TRACE("TOUCH_MSG_TYPE_TOUCH_DATA\n");
        set_touch_data(msg->touchdata);
        break;

    case TETHERING__TOUCH_MSG__TYPE__DISPLAY_MSG:
        LOG_TRACE("TOUCH_MSG_TYPE_DISPLAY_MSG\n");

        if (is_display_dirty()) {
            LOG_TRACE("display dirty status!! send the image\n");

            send_display_image_data();
            set_display_dirty(false);
        }
        break;

    case TETHERING__TOUCH_MSG__TYPE__HWKEY_MSG:
        LOG_TRACE("TOUCH_MSG_TYPE_HWKEY_MSG\n");
        set_hwkey_data(msg->hwkey);
        break;

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

static void dump_display_image_data(struct encode_mem *image)
{
#ifdef IMAGE_DUMP
    FILE *fp = NULL;

    fp = fopen("display_image_dump.png", "wb");
    if (fp != NULL) {
        fwrite(image->buffer, 1, image->length, fp);
        fclose(fp);
    }
#endif
}

static bool send_display_image_data(void)
{
    bool ret = false;
    struct encode_mem *image = NULL;

    Tethering__TouchMsg touch = TETHERING__TOUCH_MSG__INIT;
    Tethering__DisplayMsg display = TETHERING__DISPLAY_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    image = (struct encode_mem *)encode_framebuffer(ENCODE_PNG);
    if (!image) {
        LOG_SEVERE("failed to encode framebuffer\n");
        return false;
    }

    dump_display_image_data(image);

    LOG_TRACE("image data size %d\n", image->length);
    display.has_imagedata = true;
    display.imagedata.len = image->length;
    display.imagedata.data = image->buffer;

    touch.type = TETHERING__TOUCH_MSG__TYPE__DISPLAY_MSG;
    touch.display = &display;

    ret = build_touch_msg(&touch);
    LOG_TRACE("send display message: %d\n", ret);

    g_free(image->buffer);
    g_free(image);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);
    return ret;
}
