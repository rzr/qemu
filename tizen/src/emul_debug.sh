#!/bin/sh
gdb --args  ./emulator-x86 --vtm default --disk x86/VMs/default/emulimg-default.x86 \
 -- -net nic,model=virtio -soundhw all -usb -usbdevice wacom-tablet -vga tizen -bios bios.bin -L x86/data/pc-bios -kernel x86/data/kernel-img/bzImage -usbdevice keyboard -rtc base=utc -net user --enable-kvm -redir tcp:1202:10.0.2.16:22
