#!/bin/sh

# find EMUALTOR_INSTALLED_PATH
SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

if [ ! -z $TIZEN_SDK_PATH ]; then
	EMULATOR_INSTALLED_PATH=$TIZEN_SDK_PATH/tools/emulator
elif [ -e "$SCRIPT_DIR/../../../sdk.info" ]; then
	EMULATOR_INSTALLED_PATH=$SCRIPT_DIR/..
elif [ -e "$HOME/.installmanager/tizensdkpath" ]; then
	. $HOME/.installmanager/tizensdkpath
	EMULATOR_INSTALLED_PATH=$TIZEN_SDK_INSTALLED_PATH/tools/emulator
fi

EMULATOR_INSTALLED_PATH=$(readlink -f $EMULATOR_INSTALLED_PATH)

# prepare running
EMULATOR_BIN_PATH=$EMULATOR_INSTALLED_PATH/bin
LIBRARY_PATH=$EMULATOR_BIN_PATH:$EMULATOR_INSTALLED_PATH/remote/lib:

# check "--with-gdb"
WITH_GDB="no"
if [ "$1" = "--with-gdb" ]; then
	shift
	WITH_GDB="yes"
fi

# use default conf file if no options provided
if [ -z $1 ]; then
	if [ -e $HOME/.emulator_default.conf ]; then
		set -- "--conf" "$HOME/.emulator_default.conf"
		echo "No options provided. Use default conf file [$HOME/.emulator_default.conf]."
	fi
fi

# run emulator
if [ "$WITH_GDB" = "yes" ]; then
	LD_LIBRARY_PATH=$LIBRARY_PATH gdb --args $EMULATOR_BIN_PATH/emulator-x86 $@
else
	LD_LIBRARY_PATH=$LIBRARY_PATH $EMULATOR_BIN_PATH/emulator-x86 $@
fi
