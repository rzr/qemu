/*
 * Emulator Control Server - Device Tethering Handler
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  KiTae Kim       <kt920.kim@samsung.com>
 *  JiHey Kim       <jihye1128.kim@samsung.com>
 *  DaiYoung Kim    <daiyoung777.kim@samsung.com>
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

// ecs <-> ecp messages
#define ECS_TETHERING_MSG_CATEGORY                      "tethering"

#define ECS_TETHERING_MSG_GROUP_ECP                     1
// #define TETHERING_MSG_GROUP_USB
// #define TETHERING_MSG_GROUP_WIFI

#define ECS_TETHERING_MSG_ACTION_CONNECT                1
#define ECS_TETHERING_MSG_ACTION_DISCONNECT             2
#define ECS_TETHERING_MSG_ACTION_CONNECTION_STATUS      3
#define ECS_TETHERING_MSG_ACTION_SENSOR_STATUS          4
#define ECS_TETHERING_MSG_ACTION_TOUCH_STATUS           5

void send_tethering_sensor_status_ecp(void);
void send_tethering_touch_status_ecp(void);
void send_tethering_connection_status_ecp(void);
void send_tethering_sensor_data(const char *data, int len);
void send_tethering_touch_data(int x, int y, int index, int status);