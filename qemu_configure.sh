#!/bin/sh
# OS specific
#--target-list=i386-softmmu,arm-softmmu \
targetos=`uname -s`
case $targetos in
Linux*)
echo "checking for os... targetos $targetos"
exec ./configure \
 --target-list=i386-softmmu \
 --enable-profiler \
 --disable-werror \
 --audio-drv-list=alsa \
 --audio-card-list=es1370 \
 --enable-mixemu \
 --disable-vnc-tls \
# --enable-gles2 --gles2dir=/usr
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
 --target-list=i386-softmmu,arm-softmmu \
 --audio-drv-list=winwave \
 --audio-card-list=es1370 \
 --enable-mixemu \
 --disable-vnc-tls \
 --disable-vnc-jpeg \
 --disable-jpeg
;;
esac
