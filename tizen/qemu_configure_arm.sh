#!/bin/sh
# OS specific
#--target-list=i386-softmmu,arm-softmmu \
targetos=`uname -s`

cd ..
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=arm-softmmu \
 --disable-werror \
 --audio-drv-list=alsa \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-opengles \
 --enable-ldst-optimization \
 --enable-maru \
 --disable-pie
# --enable-mixemu \
# --enable-gl
# --enable-ffmpeg
# --enable-v4l2 \
# --enable-debug \
# --enable-profiler
;;
MINGW*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=arm-softmmu \
 --audio-drv-list=winwave \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-hax \
 --enable-ldst-optimization \
 --enable-maru
# --enable-gl
# --enable-ffmpeg
# --disable-vnc-jpeg \
# --disable-jpeg
;;
esac
