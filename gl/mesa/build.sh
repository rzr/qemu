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
if test "$TARGET_ARCH" = "x86_64" ; then
 TARGET_DIR="x86_64"
 TARGETLIB_SIZE="--enable-64-bit"
else
 TARGET_DIR="x86"
 TARGETLIB_SIZE="--enable-32-bit"
fi

case $TARGET_OS in
Linux*)
make distclean && \
./configure CPPFLAGS="-D_GNU_SOURCE=1" CFLAGS="-U_FORTIFY_SOURCE -fno-stack-protector" CXXFLAGS="-U_FORTIFY_SOURCE -fno-stack-protector" $TARGETLIB_SIZE --enable-shared --disable-glu --enable-opengl --enable-osmesa --disable-gles1 --disable-gles2 --disable-gallium-egl --disable-gallium-gbm --disable-gallium-g3dvl --disable-gallium-llvm --disable-egl --disable-openvg --without-gallium-drivers  --disable-dri --disable-glx --disable-gbm --disable-xvmc --disable-vdpau --disable-va --prefix="`pwd`/../lib/$TARGET_OS/$TARGET_DIR" --includedir="`pwd`/../include" --libdir="\${prefix}" && \
make && \
make install && \
make distclean
;;
*)
echo "OpenGL acceleration is not currently supported for $TARGET_OS"
;;
esac

