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

#include "../maru_common.h"

#define JAVA_MAX_COMMAND_LENGTH 1024

#define JAR_SKINFILE "emulator-skin.jar"
#define JAVA_LIBRARY_PATH "-Djava.library.path"

#ifndef CONFIG_DARWIN
#define JAVA_EXEOPTION "-jar"
#else
#define JAVA_EXEOPTION "-XstartOnFirstThread -jar" // Must start the Java window on the first thread on Mac
#endif
#define JAVA_SIMPLEMODE_OPTION "simple.msg"

#ifdef CONFIG_WIN32
#define  MY_KEY_WOW64_64KEY 0x0100
int is_wow64(void);
int get_java_path(char**);
static char* JAVA_EXEFILE_PATH = 0;
#else
#define JAVA_EXEFILE_PATH "java"
#endif

int start_skin_client(int argc, char* argv[]);
int start_simple_client(char* msg);

#endif /* MARUSKIN_CLIENT_H_ */
