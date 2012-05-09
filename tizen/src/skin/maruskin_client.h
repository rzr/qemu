/*
 * communicate with java skin process
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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

#ifndef MARUSKIN_CLIENT_H_
#define MARUSKIN_CLIENT_H_

#define JAVA_MAX_COMMAND_LENGTH 512

#define JAR_SKINFILE_PATH "emulator-skin.jar"
#define JAVA_EXEFILE_PATH "java"
#define JAVA_EXEOPTION "-jar"
#define JAVA_SIMPLEMODE_OPTION "simple.msg"


int start_skin_client(int argc, char* argv[]);
int start_simple_client(char* msg);

#endif /* MARUSKIN_CLIENT_H_ */
