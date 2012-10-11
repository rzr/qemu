#!/bin/sh
# OS specific
targetos=`uname -s`
targetarch=`echo | gcc -E -dM - | grep __x86_64`
bindir="i386"
ffmpegarc="x86"
if test "$targetarch" != ""
then
	bindir="x86_64"
	ffmpegarc="x86_64"
fi

echo "##### checking for os... targetos $targetos"

case $targetos in
Linux*)
cd distrib/libav
echo ""
echo "##### FFMPEG configure for emulator"
./configure \
 --prefix=./$bindir --arch=${ffmpegarc} --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1
cd ../.. 

cd ..

echo ""
echo "##### QEMU configure for emulator"
exec ./configure \
 --disable-werror \
 --audio-drv-list=alsa \
 --audio-card-list=ac97 \
 --enable-maru \
 --disable-vnc \
 --disable-pie $1
 # --enable-ldst-optimization \
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
 --audio-drv-list=winwave \
 --audio-card-list=ac97 \
 --enable-hax \
 --enable-maru \
 --disable-vnc $1
 # --enable-ldst-optimization \
;;
Darwin*)
cd distrib/libav
echo ""
echo "##### FFMPEG configure for emulator"
./configure \
--prefix=./$bindir --arch=x86 --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1 --cc=cc
cd ../..

cd ..
echo ""
echo "##### QEMU configure for emulator"
./configure \
 --audio-drv-list=coreaudio \
 --enable-mixemu \
 --audio-card-list=ac97 \
 --enable-maru \
 --enable-hax \
 --disable-vnc \
 --disable-sdl $1
;;
esac
