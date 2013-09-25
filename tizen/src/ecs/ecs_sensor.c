/*
 * Emulator Control Server - Sensor Device Handler
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  Daiyoung Kim    <daiyoung777.kim@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
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

#include "qemu-common.h"
//#include "qemu_socket.h"
//#include "qemu-queue.h"
//#include "qemu-option.h"
//#include "qemu-config.h"
//#include "qemu-timer.h"

#include "ecs.h"
#include "hw/maru_virtio_sensor.h"

#define TEMP_BUF_SIZE   255
#define MAX_VAL_LENGTH  40

#define ACCEL_ADJUST    100000
#define ACCEL_MAX       1961330

#define GYRO_ADJUST     17.50

static int parse_val(const char *buff, unsigned char data, char *parsbuf)
{
    int count=0;

    while(1)
    {
        if(count > MAX_VAL_LENGTH)
            return -1;
        if(buff[count] == data)
        {
            count++;
            strncpy(parsbuf, buff, count);
            return count;
        }
        count++;
    }

    return 0;
}

static int get_parse_val (const char* buf, char* tmp)
{
    int index = 0;

    memset(tmp, 0, sizeof(TEMP_BUF_SIZE));

    index = parse_val(buf, 0x0a, tmp);

    return index;
}

int accel_min_max(double value)
{
    int result = (int)(value * ACCEL_ADJUST);

    if (result > ACCEL_MAX)
        result = ACCEL_MAX;

    if (result < -ACCEL_MAX)
        result = -ACCEL_MAX;

    return result;
}

static int _accel_min_max(char* tmp)
{
    return accel_min_max(atof(tmp));
}

void req_set_sensor_accel(int x, int y, int z)
{
    char tmp[TEMP_BUF_SIZE] = { 0, };

    sprintf(tmp, "%d, %d, %d", x, y, z);

    set_sensor_accel(tmp, strlen(tmp));
}

static void _req_set_sensor_accel(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x, y, z;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = _accel_min_max(tmp);

    // y
    len += get_parse_val(data + len, tmp);
    y = _accel_min_max(tmp);

    // z
    len += get_parse_val(data + len, tmp);
    z = _accel_min_max(tmp);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d, %d, %d", x, y, z);

    set_sensor_accel(tmp, strlen(tmp));
}

static void _req_set_sensor_proxi(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // vo
    len += get_parse_val(data + len, tmp);

    set_sensor_proxi(tmp, strlen(tmp));
}

static void _req_set_sensor_light(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = atoi(tmp);

    if (x == 2) {
        // y
        len += get_parse_val(data + len, tmp);

        set_sensor_light(tmp, strlen(tmp));
    }
}

static void _req_set_sensor_gyro(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x, y, z;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = (int)(atoi(tmp) / GYRO_ADJUST);

    // y
    len += get_parse_val(data + len, tmp);
    y = (int)(atoi(tmp) / GYRO_ADJUST);

    // z
    len += get_parse_val(data + len, tmp);
    z = (int)(atoi(tmp) / GYRO_ADJUST);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d %d %d", x, y, z);

    set_sensor_gyro(tmp, strlen(tmp));
}

static void _req_set_sensor_geo(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x, y, z, accuracy, t_north, t_east, t_vertical;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = atoi(tmp);

    // y
    len += get_parse_val(data + len, tmp);
    y = atoi(tmp);

    // z
    len += get_parse_val(data + len, tmp);
    z = atoi(tmp);

    len += get_parse_val(data + len, tmp);
    accuracy = atoi(tmp);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d %d %d %d", x, y, z, accuracy);

    set_sensor_tilt(tmp, strlen(tmp));

    // tesla_north
    len += get_parse_val(data + len, tmp);
    t_north = atoi(tmp);

    // tesla_east
    len += get_parse_val(data + len, tmp);
    t_east = atoi(tmp);

    // tesla_vertical
    len += get_parse_val(data + len, tmp);
    t_vertical = atoi(tmp);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d %d %d", t_north, t_east, t_vertical);

    set_sensor_mag(tmp, strlen(tmp));
}

static void _req_set_sensor_tilt(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x, y, z, accuracy = 3;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = atoi(tmp);

    // y
    len += get_parse_val(data + len, tmp);
    y = atoi(tmp);

    // z
    len += get_parse_val(data + len, tmp);
    z = atoi(tmp);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d %d %d %d", x, y, z, accuracy);

    set_sensor_tilt(tmp, strlen(tmp));
}

static void _req_set_sensor_mag(int len, const char* data)
{
    char tmp[TEMP_BUF_SIZE];
    int x, y, z;

    // get sensor level
    len += get_parse_val(data + len, tmp);

    // x
    len += get_parse_val(data + len, tmp);
    x = atoi(tmp);

    // y
    len += get_parse_val(data + len, tmp);
    y = atoi(tmp);

    // z
    len += get_parse_val(data + len, tmp);
    z = atoi(tmp);

    memset(tmp, 0, TEMP_BUF_SIZE);

    sprintf(tmp, "%d %d %d", x, y, z);

    set_sensor_mag(tmp, strlen(tmp));
}

void set_sensor_data(int length, const char* data)
{
    char tmpbuf[TEMP_BUF_SIZE];
    int len = get_parse_val(data, tmpbuf);

    switch(atoi(tmpbuf)) {
        case level_accel:
            _req_set_sensor_accel(len, data);
            break;
        case level_proxi:
            _req_set_sensor_proxi(len, data);
            break;
        case level_light:
            _req_set_sensor_light(len, data);
            break;
        case level_gyro:
            _req_set_sensor_gyro(len, data);
            break;
        case level_geo:
            _req_set_sensor_geo(len, data);
            break;
        case level_tilt:
            _req_set_sensor_tilt(len, data);
            break;
        case level_magnetic:
            _req_set_sensor_mag(len, data);
            break;
        default:
            break;
    }
}

