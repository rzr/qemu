#!/bin/sh
UNAME=`uname`

PREFIX="${PWD}/output/"

CELT_PATH="common/celt-0.5.1.3/"
SPICE_COMMON_PATH="common/spice-common/"
SPICE_SERVER_PATH="server/spice-0.12.2/"
SPICE_GTK_PATH="client/spice-gtk-0.19/"
VIRT_VIEWER_PATH="client/virt-viewer-0.5.3/"

case "$UNAME" in
Linux)
    NUMCPU=`grep -c 'cpu cores' /proc/cpuinfo`
    ;;
MINGW*)
    NUMCPU=`echo $NUMBER_OF_PROCESSORS`
    ;;
Darwin)
    NUMCPU=`sysctl hw.ncpu | awk '{print $2}'`
    ;;
esac

echo "Number of CPUs $NUMCPU"

if [ "x$NUMCPU" != "x" ] ; then
    NUMCPU=$(( NUMCPU + 1 ))
else
    NUMCPU=1
fi

dist_clean()
{
    if test -e "Makefile"
    then
        make distclean
    fi
}

cd $CELT_PATH
dist_clean
cd ../../

cd $SPICE_COMMON_PATH
dist_clean
cd ../../

cd $SPICE_SERVER_PATH
dist_clean
cd ../../

cd ${SPICE_GTK_PATH}
dist_clean
cd ../../

cd ${VIRT_VIEWER_PATH}
dist_clean
cd ../../

rm -rf ../../emulator/remote

rm -rf output/include/
rm -rf output/lib/
rm -rf output/share/
find output/bin/* ! -name "remote-viewer.sh" -exec rm {} \;
