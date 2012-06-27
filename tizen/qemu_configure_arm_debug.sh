#!/bin/sh
# OS specific
targetos=`uname -s`

cd ..
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=arm-softmmu \
 --disable-werror \
 --audio-drv-list=alsa \
 --enable-opengles \
 --disable-gl \
 --enable-maru \
 --enable-efence \
 --enable-debug \
 --disable-pie
;;
MINGW*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=arm-softmmu \
 --disable-werror \
 --audio-drv-list=winwave \
 --enable-opengles \
 --disable-gl \
 --enable-maru \
 --enable-efence \
 --enable-debug \
 --disable-pie
;;
esac
