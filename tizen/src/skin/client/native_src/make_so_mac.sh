#!/bin/bash
javah -classpath ../emulator-skin.jar:../lib/swt.jar -jni org.tizen.emulator.skin.EmulatorSkin
rm ./org_tizen_emulator_skin_EmulatorSkin_PollFBThread.h
rm ./org_tizen_emulator_skin_EmulatorSkin_SkinReopenPolicy.h
#TODO: jdk path
gcc -c Share.c -o Share.o -I/System/Library/Frameworks/JavaVM.framework/Headers
gcc -dynamiclib Share.o -o libshared.dylib
