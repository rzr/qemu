#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "org_tizen_emulator_skin_EmulatorSkin.h"


void *shared_memory = (void *)0;
int shmid;


JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorSkin_shmget
  (JNIEnv *env, jobject obj, jint vga_ram_size)
{
    int mykey = getuid();

    shmid = shmget((key_t)mykey, (size_t)vga_ram_size, 0666 | IPC_CREAT);
    if (shmid == -1) {
        return 1;
    }

    /* We now make the shared memory accessible to the program. */
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        return 2;
    }

    //printf("Memory attached at %X\n", (int)shared_memory);

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorSkin_shmdt
  (JNIEnv *env, jobject obj)
{
    /* Lastly, the shared memory is detached */
    if (shmdt(shared_memory) == -1) {
        return 1;
    }

    return 0;
}

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_EmulatorSkin_getPixels
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

