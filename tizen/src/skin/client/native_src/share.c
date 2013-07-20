/**
 * Transmit the framebuffer from shared memory by JNI
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
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

#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "org_tizen_emulator_skin_EmulatorShmSkin.h"

#define MAXLEN 512

void *shared_memory = (void *)0;
int shmid;


JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_shmget
    (JNIEnv *env, jobject obj, jint shmkey, jint size)
{
    fprintf(stdout, "share.c: shared memory key=%d, size=%d bytes\n",
        shmkey, size);
    fflush(stdout);

    shmid = shmget((key_t)shmkey, (size_t)size, 0666 | IPC_CREAT);
    if (shmid == -1) {
        fprintf(stderr, "share.c: shmget failed\n");
        fflush(stderr);

        if (errno == EINVAL) {
            /* get identifiedr of unavailable segment */
            shmid = shmget((key_t)shmkey, (size_t)1, 0666 | IPC_CREAT);
            if (shmid == -1) {
                perror("share.c: ");
                return 1;
            }

            /* a segment with given key existed, but size is greater than
            the size of that segment */
            if (shmctl(shmid, IPC_RMID, 0) == -1) {
                perror("share.c: ");
                return 1;
            }

            /* try to shmget one more time */
            shmid = shmget((key_t)shmkey, (size_t)size, 0666 | IPC_CREAT);
            if (shmid == -1) {
                perror("share.c: ");
                return 1;
            } else {
                fprintf(stdout, "share.c: new segment was to be created!\n");
                fflush(stdout);
            }
        }
    }

    /* We now make the shared memory accessible to the program */
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        fprintf(stderr, "share.c: shmat failed\n");
        fflush(stderr);
        return 2;
    }

    fprintf(stdout, "share.c: memory attached at 0x%X\n",
        (int)shared_memory);
    fflush(stdout);

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_shmdt
    (JNIEnv *env, jobject obj)
{
    struct shmid_ds shm_info;

    if (shmctl(shmid, IPC_STAT, &shm_info) == -1) {
        fprintf(stderr, "share.c: shmctl failed\n");
        fflush(stderr);
        perror("share.c: ");

        shm_info.shm_nattch = -1;
    }

    fprintf(stdout, "share.c: clean up the shared memory\n");
    fflush(stdout);

    /* Lastly, the shared memory is detached */
    if (shmdt(shared_memory) == -1) {
        fprintf(stderr, "share.c: shmdt failed\n");
        fflush(stderr);
        perror("share.c: ");
        return 1;
    }
    shared_memory = NULL;

    if (shm_info.shm_nattch == 1) {
        /* remove */
        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            fprintf(stdout, "share.c: segment was already removed\n");
            fflush(stdout);
            perror("share.c: ");
            return 2;
        }
    } else if (shm_info.shm_nattch != -1) {
        fprintf(stdout, "share.c: number of current attaches = %d\n",
            (int)shm_info.shm_nattch);
        fflush(stdout);
    }

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorShmSkin_getPixels
    (JNIEnv *env, jobject obj, jintArray array)
{
    int len = (*env)->GetArrayLength(env, array);
    if (len <= 0) {
        fprintf(stderr, "share.c: get length failed\n");
        fflush(stderr);
        return -1;
    }

    if (shared_memory != NULL) {
        (*env)->SetIntArrayRegion(env, array, 0, len, shared_memory);
    }

    return len;
}

