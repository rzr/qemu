#!/bin/sh
TARGET_OS=`uname -s`
BUILD_OS=`uname -p`
BIN_DIR="i386"

if test "$BUILD_OS" = "x86_64"
then
	BIN_DIR="x86_64"
fi

case $TARGET_OS in
Linux*)
./configure --prefix=./$BIN_DIR --arch=x86 --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1
;;
MINGW*)
./configure --prefix=./$BIN_DIR --arch=x86 --enable-static --enable-w32threads --enable-optimizations --enable-memalign-hack --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1
;;
Darwin*)
./configure --prefix=./$BIN_DIR --arch=x86 --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1 --cc=cc
;;
esac
