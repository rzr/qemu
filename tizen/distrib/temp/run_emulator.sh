#!/bin/bash
./qemu-system-i386 --skin-args test.hb.ignore=true --qemu-args -drive file=emulimg.x86,if=virtio -boot c -append "console=ttyS0 video=uvesafb:ywrap,480x800-32@60 proxy= dns1=168.126.63.1 sdb_port=26100 dpi=2070 root=/dev/vda rw ip=10.0.2.16::10.0.2.2:255.255.255.0::eth0:none 5" -serial file:emulator.klog -m 512 -M maru-x86-machine -net nic,model=virtio -soundhw all -usb -usbdevice maru-touchscreen -vga maru -bios bios.bin -L . -kernel bzImage -usbdevice keyboard -net user -rtc base=utc -enable-kvm -redir tcp:1202:10.0.2.16:22 
