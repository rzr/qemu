#!/bin/sh
# OS specific
targetos=`uname -s`
targetarch=`echo | gcc -E -dM - | grep __x86_64`
bindir="i386"

if test "$targetarch" = "x86_64"
then
	bindir="x86_64"
fi

echo "##### checking for os... targetos $targetos"

case $targetos in
Linux*)
cd distrib/libav
echo ""
echo "##### FFMPEG configure for emulator"
./configure \
 --prefix=./$bindir --arch=x86 --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1
cd ../.. 

cd ..

echo ""
echo "##### QEMU configure for emulator"
exec ./configure \
 --target-list="i386-softmmu arm-softmmu" \
 --disable-werror \
 --audio-drv-list=alsa \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-maru \
 --disable-vnc-jpeg \
 --disable-vnc-png \
 --enable-opengles \
 --enable-gl \
 --disable-pie
;;
MINGW*)
cd distrib/libav
echo ""
echo "##### FFMPEG configure for emulator"
./configure \
 --prefix=./$bindir --arch=x86 --enable-static --enable-w32threads --enable-optimizations --enable-memalign-hack --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1
cd ../..

cd ..
echo ""
echo "##### QEMU configure for emulator"
exec ./configure \
 --target-list="i386-softmmu arm-softmmu" \
 --audio-drv-list=winwave \
 --enable-mixemu \
 --disable-vnc-tls \
 --audio-card-list=ac97 \
 --enable-ldst-optimization \
 --enable-hax \
 --enable-maru \
 --disable-vnc-jpeg \
 --disable-vnc-png \
 --enable-opengles \
 --enable-gl $1
;;
esac
