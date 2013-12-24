#!/bin/sh
UNAME=`uname`

PREFIX="${PWD}/output"

CELT_PATH="common/celt-0.5.1.3/"
SPICE_COMMON_PATH="common/spice-common/"
SPICE_SERVER_PATH="server/spice-0.12.2/"
SPICE_GTK_PATH="client/spice-gtk-0.19/"
VIRT_VIEWER_PATH="client/virt-viewer-0.5.3/"

export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH}

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

cd $CELT_PATH
./configure --prefix=$PREFIX --exec_prefix=$PREFIX && make -j$NUMCPU && make install
cd ../../

cd ${SPICE_COMMON_PATH}
./configure --prefix=$PREFIX --exec_prefix=$PREFIX --enable-smartcard=no && make -j$NUMCPU
cd ../../

cd $SPICE_SERVER_PATH
./configure --prefix=$PREFIX --exec_prefix=$PREFIX && make -j$NUMCPU && make install
cd ../../

cd ${SPICE_GTK_PATH}
./configure --prefix=$PREFIX --exec_prefix=$PREFIX --enable-smartcard=no && make -j$NUMCPU && make install
cd ../../

cd ${VIRT_VIEWER_PATH}
./configure --prefix=$PREFIX --exec_prefix=$PREFIX --libdir=$PREFIX/lib && make -j$NUMCPU && make install
cd ../../

mkdir -p ../../emulator/remote/bin
mkdir -p ../../emulator/remote/lib
mkdir -p ../../emulator/remote/share/virt-viewer/ui

cp ${PREFIX}/bin/remote-viewer ../../emulator/remote/bin/
cp ${PREFIX}/bin/remote-viewer.sh ../../emulator/remote/bin/

cp ${PREFIX}/lib/libcelt051.so.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libcelt051.so.0.0.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-client-glib-2.0.so.8 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-client-glib-2.0.so.8.3.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-client-gtk-3.0.so.4 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-client-gtk-3.0.so.4.0.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-controller.so.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-controller.so.0.0.0 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-server.so.1 ../../emulator/remote/lib/
cp ${PREFIX}/lib/libspice-server.so.1.6.0 ../../emulator/remote/lib/

cp ${PREFIX}/share/virt-viewer/ui/*.xml ../../emulator/remote/share/virt-viewer/ui/
