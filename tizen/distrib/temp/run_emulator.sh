#!/bin/bash
./qemu-system-i386 --skin-args a --qemu-args -M maru-x86-machine -net nic,model=virtio -soundhw all -usb -usbdevice wacom-tablet -vga maru -bios bios.bin -L ../../../pc-bios -kernel bzImage -usbdevice keyboard -net user -rtc base=utc -enable-kvm emulimg.x86
