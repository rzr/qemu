#!/bin/sh
# Build both x86 and ARM emulators by default

UNAME=`uname`
CONFIGURE_SCRIPT="./emulator_configure.sh"

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


$CONFIGURE_SCRIPT $* && make -j$NUMCPU && make install

