#!/bin/bash
javah -classpath ../bin/:../lib/swt/gtk-linux/swt.jar -jni org.tizen.emulator.skin.EmulatorSkin
rm ./org_tizen_emulator_skin_EmulatorSkin_PollFBThread.h
rm ./org_tizen_emulator_skin_EmulatorSkin_SkinReopenPolicy.h
gcc -c Share.c -o Share.o -I /usr/lib/jvm/jdk1.7.0_04/include -I /usr/lib/jvm/jdk1.7.0_04/include/linux
gcc -shared Share.o -o libshared.so -fPIC
