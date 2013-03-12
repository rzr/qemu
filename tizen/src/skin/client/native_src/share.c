/**
 *
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or ( at your option ) any later version.
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

#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "org_tizen_emulator_skin_EmulatorShmSkin.h"

#define MAXLEN 512
#define SHMKEY 26099

void *shared_memory = (void *)0;
int shmid;


JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_shmget
  (JNIEnv *env, jobject obj, jint vga_ram_size)
{
    void *temp;
    int keyval;
    shmid = shmget((key_t)SHMKEY, (size_t)MAXLEN, 0666 | IPC_CREAT);
    if (shmid == -1) {
	fprintf(stderr, "share.c: shmget failed\n");
        exit(1);
    }
    
    temp = shmat(shmid, (char*)0x0, 0);
    if (temp == (void *)-1) {
        fprintf(stderr, "share.c: shmat failed\n");
        exit(1);
    }
    keyval = atoi(temp);
    shmdt(temp);

    shmid = shmget((key_t)keyval, (size_t)vga_ram_size, 0666 | IPC_CREAT);
    if (shmid == -1) {
        return 1;
    }

    /* We now make the shared memory accessible to the program. */
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        return 2;
    }

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_shmdt
  (JNIEnv *env, jobject obj)
{
    /* Lastly, the shared memory is detached */
    if (shmdt(shared_memory) == -1) {
        return 1;
    }

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_getPixels
  (JNIEnv *env, jobject obj, jintArray array)
{
    int i = 0;
    int len = (*env)->GetArrayLength(env, array);
    if (len <= 0) {
        return -1;
    }

    int *framebuffer = (int *)shared_memory;

    jint value = 0xFFFFFFFF;
    for(i = 0; i < len; i++) {
        value = framebuffer[i];
        (*env)->SetIntArrayRegion(env, array, i, 1, &value); 
    }

    return len;
}

