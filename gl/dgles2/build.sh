#!/bin/sh

if [ -n "$1" ]; then
    TARGET_ARCH=$1
else
    TARGET_ARCH=`uname -m`
fi;

if test "$TARGET_ARCH" != "i386" -a "$TARGET_ARCH" != "i486" -a "$TARGET_ARCH" != "i686" -a "$TARGET_ARCH" != "x86_64" -a "$TARGET_ARCH" != "x86" ; then
    echo "Error! Uknown arch $TARGET_ARCH, supported: [i386|i486|i686|x86|x86_64]"
    exit 1
fi;

TARGET_OS=`uname -s`
case $TARGET_OS in
MINGW*)
TARGET_OS="MINGW32"
;;
esac

if test "$TARGET_ARCH" = "x86_64" ; then
 TARGET_DIR="x86_64"
else
 TARGET_DIR="x86"
fi

CONFIG_LIB_SUBDIR=$TARGET_OS/$TARGET_DIR
export CONFIG_LIB_SUBDIR

case $TARGET_OS in
Linux*)
make distclean && \
./configure --disable-static --disable-wgl --disable-cocoa --enable-osmesa --enable-offscreen --enable-glx --disable-x11 --prefix="`pwd`/.." --arch=$TARGET_ARCH && \
CFLAGS="-D_GNU_SOURCE=1 -U_FORTIFY_SOURCE -fno-stack-protector" make install
make distclean
;;
MINGW*)
make distclean && \
./configure --disable-static --enable-wgl --disable-cocoa --disable-osmesa --enable-offscreen --disable-glx --disable-x11 --prefix="`pwd`/.." --arch=$TARGET_ARCH && \
make install
make distclean
;;
*)
echo "OpenGL acceleration is not currently supported for $TARGET_OS"
;;
esac

