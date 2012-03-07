/*
 * communicate with java skin process
 *
 * Copyright (C) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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


#include <pthread.h>
#include "maruskin_client.h"


#define JAR_SKINFILE_PATH ~/tizen/Emulator/bin/EmulatorSkin.jar
#define JAVA_EXEFILE_PATH java
#define JAVA_EXEOPTION -jar

static void* run_skin_client(void* args)
{
    pid_t pid;
    char cmd[256];

    sprintf(cmd, "%s %s %s", JAVA_EXEFILE_PATH, JAVA_EXEOPTION, JAR_SKINFILE_PATH);
    system(cmd);
}

bool start_skin_client(void)
{
    pthread_t thread_id = -1;

    if (0 != pthread_create(&thread_id, NULL, run_skin_client, NULL)) {
        fprintf(stderr, "fail to create skin_client pthread.\n");
        return false;
    }

    return true;
}

