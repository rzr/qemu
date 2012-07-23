#!/bin/sh

UNAME=`uname`
# Build both x86 and ARM emulators by default
CONFIGURE_SCRIPT="./emulator_configure_debug.sh"

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

if [ ! -z "$1" ] ; then
  case "$1" in
  x86|i386|i486|i586|i686)
    CONFIGURE_SCRIPT="./emulator_configure_x86_debug.sh"
  ;;
  arm)
    CONFIGURE_SCRIPT="./emulator_configure_arm_debug.sh"
  ;;
  all)
    CONFIGURE_SCRIPT="./emulator_configure_debug.sh"
  ;;
  *)
    echo "ERROR: unknown target architecture"
    exit 1
  ;;
  esac
fi

$CONFIGURE_SCRIPT && make -j$NUMCPU && make install
