#!/bin/sh
# Build both x86 and ARM emulators by default

UNAME=`uname`
CONFIGURE_SCRIPT="./emulator_configure.sh"
CONFIGURE_APPEND=""
EMUL_TARGET_LIST=""
VIRTIOGL_EN=""
OPENGLES_EN=""
YAGL_EN=""

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
    if [ -z "$OPENGLES_EN" ] ; then
      yagl_enable yes
    fi
  ;;
  all)
    EMUL_TARGET_LIST="i386-softmmu,arm-softmmu"
    if [ -z "$VIRTIOGL_EN" ] ; then
      virtgl_enable yes
    fi
    if [ -z "$OPENGLES_EN" ] ; then
      yagl_enable yes
    fi
  ;;
  esac
}

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
        shift
        virtgl_enable $1
    ;;
    -gles|--opengles)
        shift
        opengles_enable $1
    ;;
    -yagl|--yagl-device)
        shift
        yagl_enable $1
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

case "$UNAME" in
Linux)
    NUMCPU=`grep -c 'cpu cores' /proc/cpuinfo`
    ;;
MINGW*)
    NUMCPU=`echo $NUMBER_OF_PROCESSORS`
    ;;
esac

echo "Number of CPUs $NUMCPU"

if [ "x$NUMCPU" != "x" ] ; then
    NUMCPU=$(( NUMCPU + 1 ))
else
    NUMCPU=1
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

echo $CONFIGURE_SCRIPT $CONFIGURE_APPEND
$CONFIGURE_SCRIPT "$CONFIGURE_APPEND" && make -j$NUMCPU && make install

