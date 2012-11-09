#!/bin/sh

CONFIGURE_APPEND=""
EMUL_TARGET_LIST=""
VIRTIOGL_EN=""
OPENGLES_EN=""
YAGL_EN=""
YAGL_STATS_EN=""

usage() {
    echo "usage: build.sh [options] [target]"
    echo ""
    echo "target"
    echo "    emulator target, one of: [x86|i386|i486|i586|i686|arm|all]. Defaults to \"all\""
    echo ""
    echo "options:"
    echo "-d, --debug"
    echo "    build debug configuration"
    echo "-dgl, --debug-gles"
    echo "    build with openGLES passthrough device debug messages enable"
    echo "-vgl|--virtio-gl"
    echo "    enable virtio GL support"
    echo "-gles|--opengles"
    echo "    enable openGLES passthrough device"
    echo "-yagl|--yagl-device"
    echo "    enable YaGL passthrough device"
    echo "-ys|--yagl-stats"
    echo "    enable YaGL stats"
    echo "-e|--extra"
    echo "    extra options for QEMU configure"
    echo "-u|-h|--help|--usage"
    echo "    display this help message and exit"
}

virtgl_enable() {
  case "$1" in
  0|no|disable)
    VIRTIOGL_EN="no"
  ;;
  1|yes|enable)
    VIRTIOGL_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

opengles_enable() {
  case "$1" in
  0|no|disable)
    OPENGLES_EN="no"
  ;;
  1|yes|enable)
    OPENGLES_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

yagl_enable() {
  case "$1" in
  0|no|disable)
    YAGL_EN="no"
  ;;
  1|yes|enable)
    YAGL_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

yagl_stats_enable() {
  case "$1" in
  0|no|disable)
    YAGL_STATS_EN="no"
  ;;
  1|yes|enable)
    YAGL_STATS_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

set_target() {
  if [ ! -z "$EMUL_TARGET_LIST" ] ; then
      usage
      exit 1
  fi

  case "$1" in
  x86|i386|i486|i586|i686)
    EMUL_TARGET_LIST="i386-softmmu"
    if [ -z "$VIRTIOGL_EN" ] ; then
      virtgl_enable yes
    fi
  ;;
  arm)
    EMUL_TARGET_LIST="arm-softmmu"
    if [ -z "$YAGL_EN" ] && [ -z "$OPENGLES_EN" ] ; then
      if test "$targetos" = "Linux" ; then
        yagl_enable yes
      fi
    fi
  ;;
  all)
    EMUL_TARGET_LIST="i386-softmmu,arm-softmmu"
    if [ -z "$VIRTIOGL_EN" ] ; then
      virtgl_enable yes
    fi
    if [ -z "$YAGL_EN" ] && [ -z "$OPENGLES_EN" ] ; then
      if test "$targetos" = "Linux" ; then    
        yagl_enable yes
      fi
    fi
  ;;
  esac
}


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

echo "$*"

while [ "$#" -gt "0" ]
do
    case $1 in
    x86|i386|i486|i586|i686|arm|all)
        set_target $1
    ;;
    -d|--debug)
        CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-debug"
    ;;
    -dgl|--debug-gles)
        CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-debug-gles"
    ;;
    -e|--extra)
        shift
        CONFIGURE_APPEND="$CONFIGURE_APPEND $1"
    ;;
    -vgl|--virtio-gl)
        virtgl_enable 1
    ;;
    -gles|--opengles)
        opengles_enable 1
    ;;
    -yagl|--yagl-device)
        yagl_enable 1
    ;;
    -ys|--yagl-stats)
        yagl_stats_enable 1
    ;;
    -u|-h|--help|--usage)
        usage
        exit 0
    ;;
    *)
        echo "Syntax Error"
        usage
        exit 1
    ;;
    esac
    shift
done

if [ -z "$EMUL_TARGET_LIST" ] ; then
  set_target all
fi

CONFIGURE_APPEND="--target-list=$EMUL_TARGET_LIST $CONFIGURE_APPEND"

if test "$VIRTIOGL_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-gl"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-gl"
fi

if test "$OPENGLES_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-opengles"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-opengles"
fi

if test "$YAGL_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-yagl"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-yagl"
fi

if test "$YAGL_STATS_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-yagl-stats"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-yagl-stats"
fi

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
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
exec ./configure \
 $CONFIGURE_APPEND \
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
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
exec ./configure \
 $CONFIGURE_APPEND \
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
--prefix=./$bindir --extra-cflags=-mmacosx-version-min=10.4 --arch=x86 --enable-static --enable-pic --enable-optimizations --disable-doc --disable-gpl --disable-yasm --disable-postproc --disable-swscale --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --disable-decoders --disable-encoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters --enable-encoder=h263 --enable-encoder=h263p --enable-encoder=mpeg4 --enable-encoder=msmpeg4v2 --enable-encoder=msmpeg4v3 --enable-decoder=aac --enable-decoder=h263 --enable-decoder=h264 --enable-decoder=mp3 --enable-decoder=mp3adu --enable-decoder=mpeg4 --enable-decoder=mpegvideo --enable-decoder=msmpeg4v1 --enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=wmav1 --enable-decoder=wmav2 --enable-decoder=wmv3 --enable-decoder=vc1 --cc=cc
cd ../..

cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
./configure \
 $CONFIGURE_APPEND \
 --extra-cflags=-mmacosx-version-min=10.4 \
 --audio-drv-list=coreaudio \
 --enable-mixemu \
 --audio-card-list=ac97 \
 --enable-maru \
 --enable-hax \
 --disable-vnc \
 --disable-cocoa \
 --enable-gl \
 --disable-sdl $1
;;
esac
