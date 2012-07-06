#!/bin/sh
# OS specific
targetos=`uname -s`

cd ..
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list="i386-softmmu arm-softmmu" \
 --disable-werror \
 --audio-drv-list=alsa \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-maru \
 --enable-opengles \
 --enable-gl \
 --disable-pie
;;
MINGW*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list="i386-softmmu arm-softmmu" \
 --audio-drv-list=winwave \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-hax \
 --enable-maru \
 --enable-opengles \
 --enable-gl
;;
esac
