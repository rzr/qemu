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

#if 0
#ifndef __WIN32
#include <sys/ioctl.h>
#else
#define EISCONN WSAEISCONN
#endif

#include "qemu/main-loop.h"
#include "qemu/sockets.h"
#include "ui/console.h"
#endif

#include "emulator_common.h"
#include "emul_state.h"

#include "sensor.h"
#include "common.h"
#include "ecs/ecs_tethering.h"
#include "genmsg/tethering.pb-c.h"

#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(app_tethering);

typedef struct sensor_state {
    bool is_sensor_event;
    bool is_sensor_supported;

} sensor_state;

enum sensor_level {
    level_accel = 1,
    level_proxi = 2,
    level_light = 3,
    level_gyro = 4,
    level_geo = 5,
    level_tilt = 12,
    level_magnetic = 13
};

#define ACCEL_ADJUST    100000
#define GYRO_ADJUST     17.50

static int sensor_device_status;

// create a sensor message.
static bool build_sensor_msg(Tethering__SensorMsg *sensor)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__SENSOR_MSG;
    msg.sensormsg = sensor;

    ret = send_msg_to_controller(&msg);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_sensor_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;
    Tethering__SensorMsg event = TETHERING__SENSOR_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    event.type = TETHERING__SENSOR_MSG__TYPE__START_ANS;
    event.startans = &start_ans;

    LOG_TRACE("send sensor_start_ans message\n");
    ret = build_sensor_msg(&event);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_sensor_status_msg(Tethering__SensorType sensor_type,
                                    Tethering__State status)
{
    bool ret = false;

    Tethering__SensorMsg sensor = TETHERING__SENSOR_MSG__INIT;
    Tethering__SetSensorStatus sensor_status =
                            TETHERING__SET_SENSOR_STATUS__INIT;

    LOG_TRACE("enter: %s\n", __func__);

    sensor_status.type = sensor_type;
    sensor_status.state = status;

    sensor.type = TETHERING__SENSOR_MSG__TYPE__SENSOR_STATUS;
    sensor.setstatus = &sensor_status;

    LOG_TRACE("send sensor_set_event_status message\n");
    ret = build_sensor_msg(&sensor);

    LOG_TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static void set_sensor_data(Tethering__SensorData *data)
{
    /*
     * data format for sensor device
     * each value is classified by carriage return character
     * sensor_type/param numbers/parameters
     * ex) acceleration sensor: "level_accel\n3\nx\ny\nz\n"
     */

    switch(data->sensor) {
    case TETHERING__SENSOR_TYPE__ACCEL:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%lf\n%lf\n%lf\n",
                level_accel, 3, (atof(data->x) * ACCEL_ADJUST),
                (atof(data->y) * ACCEL_ADJUST), (atof(data->z) * ACCEL_ADJUST));
        send_tethering_sensor_data(tmp, strlen(tmp));

        LOG_TRACE("sensor_accel x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__MAGNETIC:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s\n%s\n%s\n",
                level_magnetic, 3, data->x, data->y, data->z);
        send_tethering_sensor_data(tmp, strlen(tmp));

        LOG_TRACE("sensor_mag x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__GYROSCOPE:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%lf\n%lf\n%lf\n",
                level_gyro, 3, (atof(data->x) / GYRO_ADJUST),
                (atof(data->y) / GYRO_ADJUST), (atof(data->z) / GYRO_ADJUST));
        send_tethering_sensor_data(tmp, strlen(tmp));

        LOG_TRACE("sensor_gyro x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__PROXIMITY:
    {
        char tmp[255] = {0};
        double x = (double)(atoi(data->x));

        sprintf(tmp, "%d\n%d\n%.1f\n", level_proxi, 1, x);
        send_tethering_sensor_data(tmp, strlen(tmp));

        LOG_TRACE("sensor_proxi x: %.1f, %s\n", x, tmp);
    }
        break;
    case TETHERING__SENSOR_TYPE__LIGHT:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s%s\n", level_light, 2, data->x, data->y);
        send_tethering_sensor_data(tmp, strlen(tmp));

        LOG_TRACE("sensor_light x: %s\n", data->x);
    }
        break;
    default:
        LOG_TRACE("invalid sensor data\n");
        break;
    }
}

bool msgproc_tethering_sensor_msg(void *message)
{
    bool ret = true;
    Tethering__SensorMsg *msg = (Tethering__SensorMsg *)message;

    switch(msg->type) {
    case TETHERING__SENSOR_MSG__TYPE__START_REQ:
        LOG_TRACE("SENSOR_MSG_TYPE_START_REQ\n");

        // set sensor type.
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__ACCEL,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__MAGNETIC,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__GYROSCOPE,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__PROXIMITY,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__LIGHT,
                                TETHERING__STATE__ENABLED);

        LOG_TRACE("SENSOR_MSG_TYPE_START_ANS\n");
        send_sensor_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);

        break;
    case TETHERING__SENSOR_MSG__TYPE__TERMINATE:
        LOG_TRACE("SENSOR_MSG_TYPE_TERMINATE\n");
        break;

    case TETHERING__SENSOR_MSG__TYPE__SENSOR_DATA:
        LOG_TRACE("SENSOR_MSG_TYPE_SENSOR_DATA\n");
        set_sensor_data(msg->data);
        break;
    default:
        LOG_TRACE("invalid sensor_msg type");
        ret = false;
        break;
    }

    return ret;
}

int get_tethering_sensor_status(void)
{
    return sensor_device_status;
}

void set_tethering_sensor_status(int status)
{
    sensor_device_status = status;
    send_tethering_sensor_status_ecp();
}
