/*
 * Virtio Sensor Device
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi  	<jinhyung2.choi@samsung.com>
 *  Daiyoung Kim 	<daiyoung777.kim@samsung.com>
 *  YeongKyoon Lee 	<yeongkyoon.lee@samsung.com>
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

#ifndef MARU_VIRTIO_SENSOR_H_
#define MARU_VIRTIO_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw/virtio/virtio.h"

enum request_cmd {
	request_get = 0,
	request_set,
	request_answer
};

enum sensor_types {
	sensor_type_accel = 0,
	sensor_type_geo,
	sensor_type_gyro,
	sensor_type_light,
	sensor_type_proxi,
	sensor_type_mag,
	sensor_type_tilt,
	sensor_type_max
};

#define MESSAGE_TYPE_SENSOR	"sensor"

#define GROUP_STATUS		15

#define ACTION_ACCEL		110
#define ACTION_GYRO			111
#define ACTION_MAG			112
#define ACTION_LIGHT		113
#define ACTION_PROXI		114


typedef struct VirtIOSensor {
    VirtIODevice    vdev;
    VirtQueue       *rvq;
    VirtQueue       *svq;
    DeviceState     *qdev;

	QEMUBH			*bh;
} VirtIOSensor;



#define TYPE_VIRTIO_SENSOR "virtio-sensor-device"
#define VIRTIO_SENSOR(obj) \
        OBJECT_CHECK(VirtIOSensor, (obj), TYPE_VIRTIO_SENSOR)



//VirtIODevice *virtio_sensor_init(DeviceState *dev);

//void virtio_sensor_exit(VirtIODevice *vdev);

void req_sensor_data(enum sensor_types type, enum request_cmd req, char* data, int len);

#define get_sensor_accel()	\
	req_sensor_data(sensor_type_accel, request_get, NULL, 0);

#define get_sensor_gyro()	\
	req_sensor_data(sensor_type_gyro, request_get, NULL, 0);

#define get_sensor_mag()	\
	req_sensor_data(sensor_type_mag, request_get, NULL, 0);

#define get_sensor_light()	\
	req_sensor_data(sensor_type_light, request_get, NULL, 0);

#define get_sensor_proxi()	\
	req_sensor_data(sensor_type_proxi, request_get, NULL, 0);

#define set_sensor_accel(data, len)	\
	req_sensor_data(sensor_type_accel, request_set, data, len);

#define set_sensor_proxi(data, len)	\
	req_sensor_data(sensor_type_proxi, request_set, data, len);

#define set_sensor_light(data, len)	\
	req_sensor_data(sensor_type_light, request_set, data, len);

#define set_sensor_gyro(data, len)	\
	req_sensor_data(sensor_type_gyro, request_set, data, len);

#define set_sensor_tilt(data, len)	\
	req_sensor_data(sensor_type_tilt, request_set, data, len);

#define set_sensor_mag(data, len)	\
	req_sensor_data(sensor_type_mag, request_set, data, len);

#ifdef __cplusplus
}
#endif

#endif
