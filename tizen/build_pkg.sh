#!/bin/sh -xe
# Emulator Release Package

set_emulator_var () {
	echo ==== Init ====
	BUILD_DIR=`pwd`
	PKG_DIR=$BUILD_DIR/EMUL_PKG
	PKG_BIN_DIR=$PKG_DIR/EMUL_BIN
	PKG_EMUL_DIR=$PKG_BIN_DIR/data
	PKG_BRANCH="release"
	PKG_VERSION=$2
	PKG_BUILD_DATE=`date +%Y%m%d`
	PKG_OUTPUT="emulator_1.1.${PKG_VERSION}_linux.zip"
	PKG="-pub"
	if test "$1" = "inhouse"
	then
		PKG="-inh"
	fi

	if test -e $PKG_DIR
	then
		echo "==== Remove PACKAGE Directory ===="
		rm -rf $PKG_DIR
	fi

	mkdir -p EMUL_PKG/EMUL_BIN/data
}

build_emulator () {
	cd $BUILD_DIR
	echo ==== Start building emulator ====
	git checkout $PKG_BRANCH
	./build.sh
	make install
	mkdir -p Emulator/x86/data/kernel-img
	mv Emulator $PKG_EMUL_DIR/
	echo ==== Finish building emulator ====
}

build_emulator_kernel () {
	echo ==== git clone emulator-kernel ====
	cd $PKG_DIR 
	git clone gerrithost:slp/sdk/public/common/emulator/open-source/emulator-kernel
	
	if test -d emulator-kernel
	then
		echo ==== Get emualtor-kernel git successfully!! ====
	else
		echo ==== Failed to get emulator-kernel git ===
		exit 1
	fi

	echo ==== Start building emulator-Kernel ====
	KERNEL_OUTPUT="arch/x86/boot/bzImage"
	KERNEL_DIR="/SLP2.0_SDK/Emulator/windows"
	cd $PKG_DIR/emulator-kernel
	git checkout $PKG_BRANCH 
	./build.sh
	cp $KERNEL_OUTPUT $PKG_EMUL_DIR/Emulator/x86/data/kernel-img/
	echo ==== Upload kernel image for Windows ====
	ncftpput -u sdk -p binary 172.21.111.180 $KERNEL_DIR $KERNEL_OUTPUT
	if [ $? != 0 ] ; then
		echo "Failed to upload the file."
	    exit 1
	fi
	echo "Succeeded to upload the file"
	echo ==== Finish building emulator-Kernel ====
}

create_emulator_metascript () {
	echo ==== Begin Emulator Packaging ====
	cd $BUILD_DIR/package
	./metapkg.sh $1	
	cp install $PKG_BIN_DIR/
	cp remove $PKG_BIN_DIR/
	cp pkginfo.manifest $PKG_BIN_DIR/
	echo ==== End Emualtor Packaging ====
}

make_emulator_release_pkg () {
	echo ==== Start compressing emulator Packaging ====
	cp $BUILD_DIR/skins/icons/vtm.ico $PKG_BIN_DIR/
	cd $PKG_BIN_DIR
	zip -r $PKG_OUTPUT *
	echo ==== Finish compressing emulator Packaging ====
}

upload_release_pkg () {
	echo ==== Remove the previous emulator package file ====
	build-util -del $PKG "emulator_1.1.*.zip"
	echo ==== Upload the current emulator package file ====
	build-util -put $PKG "$PKG_OUTPUT"
}

make_emulator_standalone_pkg () {
	cd $PKG_EMUL_DIR
	SERVER_DIR="/packages/Emulator_SA"
	PKG_OUTPUT="Tizen_Emulator_${PKG_BUILD_DATE}_linux.zip"
	zip -r $OUTPUT Emulator/
}


upload_standalone_pkg () {
	cd $PKG_EMUL_DIR
	SERVER_DIR="/packages/Emulator_SA"
	echo "===== Start upload ====="
	ncftpput -u core -p tmaxcore 172.21.111.180 $SERVER_DIR $PKG_OUTPUT
	if [ $? != 0 ] ; then
		echo "Failed to upload the file."
	    exit 1
	fi
	echo "Succeeded to upload the file"
	echo "===== Finish upload ====="
}

emulator_linux ()
{
	echo ==== Start $TARGETOS Build ====
	case $1 in
		release)
			echo ==== Start Release Build ====
			set_emulator_var $2 $3
			build_emulator
			build_emulator_kernel
			create_emulator_metascript $3
			make_emulator_release_pkg
			upload_release_pkg
			echo ==== Finish Release Build ====
			;;
		standalone)
			echo ==== Start Standalone Build ====
			set_emulator_var
			PKG_BRANCH="master"
			build_emulator
			build_emulator_kernel
			make_emulator_standalone_pkg
			upload_standalone_pkg
			echo ==== Start Standalone Build ====
			;;
		*)
			echo "usage : `basename $0` <release/standalone> <public/inhouse> <build_number>"
			echo "release) $ ./build_pkg.sh release <public/inhouse> 104"
			echo "standalone) $ ./build_pkg.sh standalone"
	esac
	echo ==== End $TARGETOS BUILD
}

emulator_windows()
{
	echo ==== Start $TARGETOS Build ====
	echo ==== Start Emulator Build ====
	WIN_BASE_DIR=/home/core1/emulator
	WIN_QEMU_DIR=$WIN_BASE_DIR/qemu/tizen
	WIN_PKG_DIR=$WIN_QEMU_DIR/EMUL_PKG
	WIN_PKG_NAME="emulator_1.1.${BUILD_NUMBER}_windows.zip"
	cd $WIN_QEMU_DIR
	autoconf
	./configure
	make && make install
	mkdir -p $WIN_QEMU_DIR/Emulator/x86/data/kernel-img
	echo ==== End Emulator Build ====

	echo ==== Get the lastest kernel image ===
	wget http://172.21.111.180/slpsdk-binary/SLP2.0_SDK/Emulator/windows/bzImage -P $WIN_QEMU_DIR/Emulator/x86/data/kernel-img
	wget http://172.21.111.180/slpsdk-binary/SLP2.0_SDK/Emulator/windows/emulator_dll.zip -P $WIN_QEMU_DIR
	
	echo ==== Make windows package directory ===
	cd $WIN_QEMU_DIR
	mkdir -p EMUL_PKG/data
	mv $WIN_QEMU_DIR/Emulator $WIN_PKG_DIR/data
	editbin.exe /subsystem:windows $WIN_PKG_DIR/data/Emulator/bin/emulator-manager.exe
	editbin.exe /subsystem:windows $WIN_PKG_DIR/data/Emulator/bin/emulator-x86.exe

	if test -e emulator_dll.zip
	then
		unzip emulator_dll.zip -d $WIN_PKG_DIR/data/Emulator/bin/
	fi

	echo ==== Copy mate files into windows package ===
	cd $WIN_QEMU_DIR/package
	./metapkg.sh ${BUILD_NUMBER}
	cp pkginfo.manifest $WIN_PKG_DIR
	cp install.bat $WIN_PKG_DIR
	cp remove.bat $WIN_PKG_DIR

	cd $WIN_PKG_DIR
	zip -r $WIN_PKG_NAME *
	echo ==== End $TARGETOS Build ====
}

TARGETOS=`uname`
case $TARGETOS in
	Linux*)
		emulator_linux $1 $2 $3
		;;
	MINGW*)
		emulator_windows
		;;
esac
