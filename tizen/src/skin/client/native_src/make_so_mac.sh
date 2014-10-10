#!/bin/bash

#Example

javah -classpath ../emulator-skin.jar:../lib/swt.jar -jni org.tizen.emulator.skin.EmulatorShmSkin
rm ./org_tizen_emulator_skin_EmulatorShmSkin_PollFBThread.h
#TODO: jdk path
cc -c share.c -o share.o -I/System/Library/Frameworks/JavaVM.framework/Headers
cc -dynamiclib share.o -o libshared.dylib
