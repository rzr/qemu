gdb --args emulator-x86.exe --vtm default -- -net nic,model=virtio -usb -usbdevice maru-touchscreen -vga tizen -bios bios.bin -L ..\x86\data\pc-bios -kernel ..\x86\data\kernel-img\bzImage -usbdevice keyboard -rtc base=utc -net user -redir tcp:1202:10.0.2.16:22
