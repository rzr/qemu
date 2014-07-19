#!/bin/sh

if [ -z "$TIZEN_SDK_DEV_PATH" ] ; then
	TIZEN_SDK_DEV_PATH=${HOME}/tizen-sdk-dev
fi

CONFIGURE_APPEND=""
EMUL_TARGET_LIST=""
YAGL_EN=""
YAGL_STATS_EN=""
VIGS_EN=""

usage() {
    echo "usage: build.sh [options] [target]"
    echo ""
    echo "target"
    echo "    emulator target, one of: [x86|i386|i486|i586|i686|arm|all]. Defaults to \"all\""
    echo ""
    echo "options:"
    echo "-d, --debug"
    echo "    build debug configuration"
    echo "-yagl|--yagl-device"
    echo "    enable YaGL passthrough device"
    echo "-ys|--yagl-stats"
    echo "    enable YaGL stats"
    echo "-vigs|--vigs-device"
    echo "    enable VIGS device"
    echo "-e|--extra"
    echo "    extra options for QEMU configure"
    echo "-u|-h|--help|--usage"
    echo "    display this help message and exit"
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

vigs_enable() {
  case "$1" in
  0|no|disable)
    VIGS_EN="no"
  ;;
  1|yes|enable)
    VIGS_EN="yes"
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
    if [ -z "$YAGL_EN" ] ; then
      yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] ; then
      vigs_enable yes
    fi
  ;;
  arm)
    EMUL_TARGET_LIST="arm-softmmu"
    if [ -z "$YAGL_EN" ] && [ "$targetos" != "Darwin" ] ; then
      yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] && [ "$targetos" != "Darwin" ] ; then
      vigs_enable yes
    fi
  ;;
  all)
#    EMUL_TARGET_LIST="i386-softmmu,arm-softmmu"
    EMUL_TARGET_LIST="i386-softmmu"
    if [ -z "$YAGL_EN" ] ; then
        yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] ; then
      vigs_enable yes
    fi
  ;;
  esac
}


# OS specific
targetos=`uname -s`
echo "##### checking for os... targetos $targetos"

echo "##### TIZEN_SDK_DEV_PATH: ${TIZEN_SDK_DEV_PATH}"

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
    -e|--extra)
        shift
        CONFIGURE_APPEND="$CONFIGURE_APPEND $1"
    ;;
    -yagl|--yagl-device)
        yagl_enable 1
    ;;
    -ys|--yagl-stats)
        yagl_stats_enable 1
    ;;
    -vigs|--vigs-device)
        vigs_enable 1
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

if test "$VIGS_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-vigs"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-vigs"
fi

# append common flags
CONFIGURE_APPEND="--enable-maru --enable-libav --enable-curl --enable-png --disable-gtk $CONFIGURE_APPEND"

if [ -z ${PKG_CONFIG_PATH} ] ; then	# avoid pkg-config bug on Windows
export PKG_CONFIG_PATH=${TIZEN_SDK_DEV_PATH}/distrib/lib/pkgconfig
else
export PKG_CONFIG_PATH=${TIZEN_SDK_DEV_PATH}/distrib/lib/pkgconfig:${PKG_CONFIG_PATH}
fi

case $targetos in
Linux*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
exec ./configure \
 --enable-werror \
 --audio-drv-list=alsa \
 --disable-vnc \
 --disable-pie \
 --enable-virtfs \
 --disable-xen \
 $CONFIGURE_APPEND \
;;
MINGW*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
exec ./configure \
 --enable-werror \
 --extra-cflags=-Wno-error=format \
 --extra-cflags=-Wno-error=format-extra-args \
 --extra-cflags=-Wno-error=redundant-decls \
 --extra-ldflags=-Wl,--large-address-aware \
 --cc=gcc \
 --disable-coroutine-pool \
 --audio-drv-list=winwave \
 --enable-hax \
 --disable-vnc \
 $CONFIGURE_APPEND \
;;
Darwin*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
./configure \
 --enable-werror \
 --extra-cflags=-mmacosx-version-min=10.4 \
 --audio-drv-list=coreaudio \
 --enable-shm \
 --enable-hax \
 --disable-vnc \
 --disable-cocoa \
 --disable-sdl \
 $CONFIGURE_APPEND \
;;
esac
