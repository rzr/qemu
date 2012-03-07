#!/bin/sh
# OS specific
#--target-list=i386-softmmu,arm-softmmu \
targetos=`uname -s`

cd ..
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --disable-werror \
 --audio-drv-list=pa \
 --enable-mixemu \
 --disable-vnc-tls \
 --enable-maru
# --enable-ffmpeg
# --enable-tcg-x86-opt \
# --enable-v4l2 \
# --enable-debug \
# --enable-profiler \
# --enable-gles2 --gles2dir=/usr
;;
MINGW*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --audio-drv-list=winwave \
 --enable-mixemu \
 --disable-vnc-tls \
 --enable-maru
# --enable-ffmpeg
# --disable-vnc-jpeg \
# --disable-jpeg
;;
esac
