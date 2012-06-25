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
 --audio-drv-list=alsa \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-maru \
 --enable-gl \
 --enable-efence
# --enable-ffmpeg
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
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-hax \
 --enable-maru \
 --enable-gl $1
# --enable-ffmpeg
# --disable-vnc-jpeg \
# --disable-jpeg
;;
esac
