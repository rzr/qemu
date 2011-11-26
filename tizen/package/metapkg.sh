PKG_NAME=emulator
PKG_VERSION=$1
PKG_DESC="Tizen Emulator"
PKG_INSTALL=install
PKG_REMOVE=remove
PKG_OS=`uname`
pkginfofile="pkginfo.manifest"

case $PKG_OS in
	Linux*)
	;;
	MINGW*)
	PKG_NAME="tizen-$PKG_NAME"
	PKG_DESC="$PKG_DESC"
	PKG_INSTALL="$PKG_INSTALL.bat"
	PKG_REMOVE="$PKG_REMOVE.bat"
	;;
esac

## Create pkginfo.manifest file 
cat > $pkginfofile << END
Package: $PKG_NAME
Version: 1.1.$PKG_VERSION
Maintainer: Yeong-Kyoon, Lee <yeongkyoon.lee@samsung.com> Dong-Kyun, Yun <dk77.yun@samsung.com>
Description: $PKG_DESC
Install-script: $PKG_INSTALL
Remove-script: $PKG_REMOVE
Category: SDK/Emulator
END
