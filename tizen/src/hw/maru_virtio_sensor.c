/*
 * Virtio Sensor Device
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung Choi <jinhyung2.choi@samsung.com>
 *  Daiyoung Kim <daiyoung777.kim@samsung.com>
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

#include <pthread.h>

#include "hw/pci/pci.h"

#include "maru_device_ids.h"
#include "maru_virtio_sensor.h"
#include "debug_ch.h"
#include "ecs/ecs.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-sensor);

#define SENSOR_DEVICE_NAME  "sensor"
#define _MAX_BUF            1024
#define __MAX_BUF_SENSOR    32

static QemuMutex accel_mutex;
static QemuMutex geo_mutex;
static QemuMutex gyro_mutex;
static QemuMutex light_mutex;
static QemuMutex proxi_mutex;

static char accel_xyz [__MAX_BUF_SENSOR] = {'0',',','9','8','0','6','6','5',',','0'};
static int accel_enable = 0;
static int accel_delay = 200000000;

static char geo_raw [__MAX_BUF_SENSOR] = {'0',' ','-','9','0',' ','0',' ','3'};
static char geo_tesla [__MAX_BUF_SENSOR] = {'1',' ','0',' ','-','1','0'};
static int geo_enable = 0;
static int geo_delay = 200000000;

static int gyro_x_raw = 0;
static int gyro_y_raw = 0;
static int gyro_z_raw = 0;
static int gyro_enable = 0;
static int gyro_delay = 200000000;

static int light_adc = 65535;
static int light_level = 10;
static int light_enable = 0;
static int light_delay = 200000000;

static int proxi_vo = 8;
static int proxi_enable = 0;
static int proxi_delay = 200000000;

VirtIOSENSOR* vsensor;
static int sensor_capability = 0;

typedef struct msg_info {
    char buf[_MAX_BUF];

    uint16_t type;
    uint16_t req;
} msg_info;

static type_action get_action(enum sensor_types type)
{
    type_action action = 0;

    switch (type) {
    case sensor_type_accel:
        action = ACTION_ACCEL;
        break;
    case sensor_type_gyro:
        action = ACTION_GYRO;
        break;
    case sensor_type_mag:
        action = ACTION_MAG;
        break;
    case sensor_type_light:
        action = ACTION_LIGHT;
        break;
    case sensor_type_proxi:
        action = ACTION_PROXI;
        break;
    default:
        break;
    }

    return action;
}

static void send_sensor_to_ecs(const char* data, enum sensor_types type)
{
    type_length length = 0;
    type_group group = GROUP_STATUS;
    type_action action = 0;
    int buf_len = strlen(data);
    int message_len =  buf_len + 14;

    char* ecs_message = (char*) malloc(message_len + 1);
    if (!ecs_message)
        return;

    memset(ecs_message, 0, message_len + 1);

    length = (unsigned short) buf_len;
    action = get_action(type);

    memcpy(ecs_message, MESSAGE_TYPE_SENSOR, 6);
    memcpy(ecs_message + 10, &length, sizeof(unsigned short));
    memcpy(ecs_message + 12, &group, sizeof(unsigned char));
    memcpy(ecs_message + 13, &action, sizeof(unsigned char));
    memcpy(ecs_message + 14, data, buf_len);

    TRACE("ntf_to_injector- len: %d, group: %d, action: %d, data: %s\n", length, group, action, data);

    send_device_ntf(ecs_message, message_len);

    if (ecs_message)
        free(ecs_message);
}

static void __set_sensor_data (enum sensor_types type, char* data, int len)
{
    if (len < 0 || len > __MAX_BUF_SENSOR) {
        ERR("sensor data size is wrong.\n");
        return;
    }

    if (data == NULL) {
        ERR("sensor data is NULL.\n");
        return;
    }

    TRACE("set_sensor_data with type '%d' with data '%s'", type, data);

    switch (type) {
        case sensor_type_accel:
            qemu_mutex_lock(&accel_mutex);
            strcpy(accel_xyz, data);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_accel_enable:
            qemu_mutex_lock(&accel_mutex);
            sscanf(data, "%d", &accel_enable);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_accel_delay:
            qemu_mutex_lock(&accel_mutex);
            sscanf(data, "%d", &accel_delay);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_gyro_enable:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d", &gyro_enable);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_delay:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d", &gyro_delay);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_x:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d", &gyro_x_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_y:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d", &gyro_y_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_z:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d", &gyro_z_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro:
            qemu_mutex_lock(&gyro_mutex);
            sscanf(data, "%d %d %d", &gyro_x_raw, &gyro_y_raw, &gyro_z_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_light_adc:
            qemu_mutex_lock(&light_mutex);
            sscanf(data, "%d", &light_adc);
            light_level = (light_adc / 6554) % 10 + 1;
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_level:
            qemu_mutex_lock(&light_mutex);
            sscanf(data, "%d", &light_level);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_enable:
            qemu_mutex_lock(&light_mutex);
            sscanf(data, "%d", &light_enable);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_delay:
            qemu_mutex_lock(&light_mutex);
            sscanf(data, "%d", &light_delay);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_proxi:
            qemu_mutex_lock(&proxi_mutex);
            sscanf(data, "%d", &proxi_vo);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        case sensor_type_proxi_enable:
            qemu_mutex_lock(&proxi_mutex);
            sscanf(data, "%d", &proxi_enable);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        case sensor_type_proxi_delay:
            qemu_mutex_lock(&proxi_mutex);
            sscanf(data, "%d", &proxi_delay);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        case sensor_type_mag:
            qemu_mutex_lock(&geo_mutex);
            strcpy(geo_tesla, data);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_tilt:
            qemu_mutex_lock(&geo_mutex);
            strcpy(geo_raw, data);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_geo_enable:
            qemu_mutex_lock(&geo_mutex);
            sscanf(data, "%d", &geo_enable);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_geo_delay:
            qemu_mutex_lock(&geo_mutex);
            sscanf(data, "%d", &geo_delay);
            qemu_mutex_unlock(&geo_mutex);
            break;
        default:
            return;
    }
}

static void __get_sensor_data(enum sensor_types type, char* msg_info)
{
    if (msg_info == NULL) {
        return;
    }

    switch (type) {
        case sensor_type_list:
            sprintf(msg_info, "%d", sensor_capability);
            break;
        case sensor_type_accel:
            qemu_mutex_lock(&accel_mutex);
            strcpy(msg_info, accel_xyz);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_accel_enable:
            qemu_mutex_lock(&accel_mutex);
            sprintf(msg_info, "%d", accel_enable);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_accel_delay:
            qemu_mutex_lock(&accel_mutex);
            sprintf(msg_info, "%d", accel_delay);
            qemu_mutex_unlock(&accel_mutex);
            break;
        case sensor_type_mag:
            qemu_mutex_lock(&geo_mutex);
            strcpy(msg_info, geo_tesla);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_tilt:
            qemu_mutex_lock(&geo_mutex);
            strcpy(msg_info, geo_raw);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_geo_enable:
            qemu_mutex_lock(&geo_mutex);
            sprintf(msg_info, "%d", geo_enable);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_geo_delay:
            qemu_mutex_lock(&geo_mutex);
            sprintf(msg_info, "%d", geo_delay);
            qemu_mutex_unlock(&geo_mutex);
            break;
        case sensor_type_gyro:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d,%d,%d", gyro_x_raw, gyro_y_raw, gyro_z_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_enable:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d", gyro_enable);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_delay:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d", gyro_delay);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_x:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d", gyro_x_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_y:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d", gyro_y_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_gyro_z:
            qemu_mutex_lock(&gyro_mutex);
            sprintf(msg_info, "%d", gyro_z_raw);
            qemu_mutex_unlock(&gyro_mutex);
            break;
        case sensor_type_light:
        case sensor_type_light_adc:
            qemu_mutex_lock(&light_mutex);
            sprintf(msg_info, "%d", light_adc);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_level:
            qemu_mutex_lock(&light_mutex);
            sprintf(msg_info, "%d", light_level);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_enable:
            qemu_mutex_lock(&light_mutex);
            sprintf(msg_info, "%d", light_enable);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_light_delay:
            qemu_mutex_lock(&light_mutex);
            sprintf(msg_info, "%d", light_delay);
            qemu_mutex_unlock(&light_mutex);
            break;
        case sensor_type_proxi:
            qemu_mutex_lock(&proxi_mutex);
            sprintf(msg_info, "%d", proxi_vo);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        case sensor_type_proxi_enable:
            qemu_mutex_lock(&proxi_mutex);
            sprintf(msg_info, "%d", proxi_enable);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        case sensor_type_proxi_delay:
            qemu_mutex_lock(&proxi_mutex);
            sprintf(msg_info, "%d", proxi_delay);
            qemu_mutex_unlock(&proxi_mutex);
            break;
        default:
            return;
    }
}

void req_sensor_data (enum sensor_types type, enum request_cmd req, char* data, int len)
{
    char msg_info [__MAX_BUF_SENSOR];
    memset(msg_info, 0, __MAX_BUF_SENSOR);

    if (type >= sensor_type_max || (req != request_get && req != request_set)) {
        ERR("unavailable sensor type request.\n");
        return;
    }

    if (req == request_set) {
        __set_sensor_data (type, data, len);
    } else if (req == request_get) {
        __get_sensor_data(type, msg_info);
        send_sensor_to_ecs(msg_info, type);
    }
}

static void answer_sensor_data_request(int type, char* data, VirtQueueElement *elem)
{
    msg_info* msginfo = (msg_info*) malloc(sizeof(msg_info));
    if (!msginfo) {
        ERR("msginfo is NULL!\n");
        return;
    }

    msginfo->req = request_answer;
    msginfo->type = type;
    __get_sensor_data(type, msginfo->buf);

    TRACE("sending message: %s, type: %d, req: %d\n", msginfo->buf, msginfo->type, msginfo->req);

    memset(elem->in_sg[0].iov_base, 0, elem->in_sg[0].iov_len);
    memcpy(elem->in_sg[0].iov_base, msginfo, sizeof(struct msg_info));

    if (msginfo)
        free(msginfo);
}

static void handle_msg(struct msg_info *msg, VirtQueueElement *elem)
{
    unsigned int len = 0;

    if (msg == NULL) {
        ERR("msg info structure is NULL.\n");
        return;
    }

    if (msg->req == request_set) {
        __set_sensor_data (msg->type, msg->buf, strlen(msg->buf));
    } else if (msg->req == request_get) {
        answer_sensor_data_request(msg->type, msg->buf, elem);
        len = sizeof(msg_info);
    }

    virtqueue_push(vsensor->vq, elem, len);
    virtio_notify(&vsensor->vdev, vsensor->vq);
}

static void virtio_sensor_vq(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOSENSOR *vsensor = (VirtIOSENSOR*)vdev;
    struct msg_info msg;
    VirtQueueElement elem;
    int index = 0;

    if (vsensor->vq == NULL) {
        ERR("virt queue is not ready.\n");
        return;
    }

    if (!virtio_queue_ready(vsensor->vq)) {
        ERR("virtqueue is not ready.");
        return;
    }

    if (virtio_queue_empty(vsensor->vq)) {
        ERR("<< virtqueue is empty.\n");
        return;
    }

    while ((index = virtqueue_pop(vq, &elem))) {
        memset(&msg, 0x00, sizeof(msg));
        memcpy(&msg, elem.out_sg[0].iov_base, elem.out_sg[0].iov_len);

        TRACE("handling msg from driver: %s, len: %d, type: %d, req: %d, index: %d\n", msg.buf, strlen(msg.buf), msg.type, msg.req, index);

        handle_msg(&msg, &elem);
    }
}

static int set_capability(char* sensor)
{
    if (!strncmp(sensor, SENSOR_NAME_ACCEL, 5)) {
        return sensor_cap_accel;
    } else if (!strncmp(sensor, SENSOR_NAME_GEO, 3)) {
        return sensor_cap_geo;
    } else if (!strncmp(sensor, SENSOR_NAME_GYRO, 4)) {
        return sensor_cap_gyro;
    } else if (!strncmp(sensor, SENSOR_NAME_LIGHT, 5)) {
        return sensor_cap_light;
    } else if (!strncmp(sensor, SENSOR_NAME_PROXI, 5)) {
        return sensor_cap_proxi;
    } else if (!strncmp(sensor, SENSOR_NAME_HAPTIC, 6)) {
        return sensor_cap_haptic;
    } else {
        ERR("unknown sensor request: %s", sensor);
    }

    return 0;
}

static void parse_sensor_capability(char* lists)
{
    char token[] = SENSOR_CAP_TOKEN;
    char* data = NULL;

    if (lists == NULL)
        return;

    data = strtok(lists, token);
    if (data != NULL) {
        sensor_capability |= set_capability(data);
        while ((data = strtok(NULL, token)) != NULL) {
            sensor_capability |= set_capability(data);
        }
    }

    INFO("sensor device capabilty enabled with %02x\n", sensor_capability);
}

static void virtio_sensor_realize(DeviceState *dev, Error **errp)
{
    INFO("initialize virtio-sensor device\n");

    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    vsensor = VIRTIO_SENSOR(vdev);

    virtio_init(vdev, SENSOR_DEVICE_NAME, VIRTIO_ID_SENSOR, 0);

    if (vsensor == NULL) {
        ERR("failed to initialize sensor device\n");
        error_set(errp, QERR_DEVICE_INIT_FAILED, SENSOR_DEVICE_NAME);
        return;
    }

    qemu_mutex_init(&accel_mutex);
    qemu_mutex_init(&gyro_mutex);
    qemu_mutex_init(&geo_mutex);
    qemu_mutex_init(&light_mutex);
    qemu_mutex_init(&proxi_mutex);

    vsensor->vq = virtio_add_queue(&vsensor->vdev, 64, virtio_sensor_vq);

    INFO("initialized sensor type: %s\n", vsensor->sensors);

    if (vsensor->sensors) {
        parse_sensor_capability(vsensor->sensors);
    }
}

static void virtio_sensor_unrealize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    INFO("destroy sensor device\n");

    qemu_mutex_destroy(&accel_mutex);
    qemu_mutex_destroy(&gyro_mutex);
    qemu_mutex_destroy(&geo_mutex);
    qemu_mutex_destroy(&light_mutex);
    qemu_mutex_destroy(&proxi_mutex);

    virtio_cleanup(vdev);
}


static void virtio_sensor_reset(VirtIODevice *vdev)
{
    TRACE("virtio_sensor_reset.\n");
}

static uint32_t virtio_sensor_get_features(VirtIODevice *vdev,
                                            uint32_t request_feature)
{
    TRACE("virtio_sensor_get_features.\n");
    return 0;
}

static Property virtio_sensor_properties[] = {
    DEFINE_PROP_STRING(ATTRIBUTE_NAME_SENSORS, VirtIOSENSOR, sensors),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_sensor_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = virtio_sensor_properties;
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->unrealize= virtio_sensor_unrealize;
    vdc->realize = virtio_sensor_realize;
    vdc->get_features = virtio_sensor_get_features;
    vdc->reset = virtio_sensor_reset;
}

static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_SENSOR,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOSENSOR),
    .class_init = virtio_sensor_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

