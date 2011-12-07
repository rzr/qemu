#!/bin/sh
# OS specific
#--target-list=i386-softmmu,arm-softmmu \
targetos=`uname -s`
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --disable-werror \
 --audio-drv-list=pa \
 --audio-card-list=es1370 \
 --enable-mixemu \
 --disable-vnc-tls \
 --extra-ldflags="-lv4l2 -lv4lconvert -Xlinker -rpath -Xlinker ./lib"
# --enable-ffmpeg \
# --enable-gles2 --gles2dir=/usr \
# --enable-profiler \
;;
CYGWIN*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --audio-drv-list=winwave \
 --audio-card-list=es1370 \
 --enable-mixemu \
 --disable-vnc-tls 
;;
MINGW*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --audio-drv-list=winwave \
 --audio-card-list=es1370 \
 --enable-mixemu \
 --disable-vnc-tls
# --enable-ffmpeg
# --disable-vnc-jpeg \
# --disable-jpeg
;;
esac
