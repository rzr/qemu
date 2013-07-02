#!/bin/bash

#Example

javah -classpath ../bin/:../lib/swt.jar -jni org.tizen.emulator.skin.EmulatorShmSkin
rm ./org_tizen_emulator_skin_EmulatorShmSkin_PollFBThread.h
#TODO: jdk path
gcc -c share.c -o share.o -I /usr/lib/jvm/jdk1.7.0_04/include -I /usr/lib/jvm/jdk1.7.0_04/include/linux
gcc -shared share.o -o libshared.so -fPIC
