#!/bin/sh -xe

./qemu_configure.sh
make && make install
