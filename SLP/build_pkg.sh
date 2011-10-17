#!/bin/sh
# Emulator Release Package

set_emulator_var () {
	echo ==== Init ====
	BUILD_DIR=`pwd`
	PACKAGE_DIR=$BUILD_DIR/EMUL_PKG
	PACKAGE_SRC_DIR=$PACKAGE_DIR/EMUL_SRC
	PACKAGE_BIN_DIR=$PACKAGE_DIR/EMUL_BIN
	PACKAGE_EMUL_DIR=$PACKAGE_BIN_DIR/data

	PKG_BRANCH="release"

	if test -e $PACKAGE_DIR
	then
		echo "==== Remove PACKAGE Directory ===="
		rm -rf $PACKAGE_DIR
	fi

	mkdir EMUL_PKG
	cd $PACKAGE_DIR
	mkdir EMUL_SRC
	mkdir -p EMUL_BIN/data

	cd $PACKAGE_SRC_DIR
}

build_emulator_pkg () { 
	echo ==== git clone ====
	git clone slp-git:slp/sdk/public/common/emulator/open-source/qemu
	git clone slp-git:slp/sdk/public/common/emulator/open-source/emulator-kernel
	
	if test -d qemu 
	then
		if test -d emulator-kernel
		then
			echo ==== Get qemu and emualtor-kernel git successfully!! ====
		else
			echo ==== Failed to get emulator-kernel git ===
			exit 1
		fi
	else
		echo ==== Qemu directory does not exist!! ====
		exit 1
	fi
	
	echo ==== Start building emulator ====
	cd $PACKAGE_SRC_DIR/qemu/SLP
	git checkout $PKG_BRANCH
	./build.sh
	make install
	mkdir -p binary/data/kernel-img
	mv binary $PACKAGE_EMUL_DIR/Emulator
	echo ==== Finish building emulator ====

	echo ==== Start building emulator-Kernel ====
	cd $PACKAGE_SRC_DIR/emulator-kernel
	git checkout $PKG_BRANCH 
	make i386_emul_defconfig
	make -j4
	cp arch/x86/boot/bzImage $PACKAGE_EMUL_DIR/Emulator/data/kernel-img/
	echo ==== Finish building emulator-Kernel ====
}

downloading_emulator_script () {
	echo ==== Begin Emulator Packaging ====
	cd $BUILD_DIR
	cd $PACKAGE_BIN_DIR
	cp $PACKAGE_EMUL_DIR/Emulator/emulator.png ./
	wget http://172.21.111.180/slpsdk-binary/SLP2.0_SDK/Emulator/qemu/install
	wget http://172.21.111.180/slpsdk-binary/SLP2.0_SDK/Emulator/qemu/pkginfo.manifest
	wget http://172.21.111.180/slpsdk-binary/SLP2.0_SDK/Emulator/qemu/remove

	chmod 755 install remove
	chmod 644 pkginfo.manifest

	PKG_VERSION=`grep 'Version' pkginfo.manifest | cut -f3- -d"."`
	sed -i s/$PKG_VERSION/$1/g pkginfo.manifest

	echo ==== End Emualtor Packaging ====
}

package_emulator_for_standalone () {
	cd $PACKAGE_EMUL_DIR
	mv Emulator/ $BUILD_DIR
}

case $1 in
	release)
	echo ==== Start Release Build ====
	set_emulator_var
	build_emulator_pkg
	downloading_emulator_script $2
	echo ==== Finish Release Build ====
	;;
	standalone)
	echo ==== Start Standalone Build ====
	set_emulator_var
	PKG_BRANCH="master"
	build_emulator_pkg
	package_emulator_for_standalone
	echo ==== Start Standalone Build ====
	;;
	*)
	echo "usage : `basename $0` <package>"
	echo " <package> : release/standalone"
esac
