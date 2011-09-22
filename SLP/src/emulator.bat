@echo off

:: delims is a TAB followed by a space
FOR /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKCU\Software\JavaSoft\Prefs" /v target-directory') DO SET sdk_path=%%B
::ECHO %sdk_path%

cd %sdk_path%
cd Emulator

emulator-x86.exe --disk emulimg-default.x86 --skin skins\emul_240x400\default.dbi --ssh-port 1202 -- -net nic,model=rtl8139 -net user -soundhw all -usb -usbdevice wacom-tablet -usbdevice keyboard -vga slp -bios bios.bin -L data\pc-bios -kernel data\kernel-img\bzImage -localtime -redir tcp:3578:10.0.2.16:3578 -redir udp:3579:10.0.2.16:3579 -redir udp:3580:10.0.2.16:3580 -redir tcp:1202:10.0.2.16:22 -redir tcp:9990:10.0.2.16:9990 -redir tcp:9991:10.0.2.16:9991 -redir tcp:9992:10.0.2.16:9992 -redir tcp:9993:10.0.2.16:9993 -redir tcp:9994:10.0.2.16:9994 -redir tcp:9995:10.0.2.16:9995 -redir tcp:9996:10.0.2.16:9996 -redir tcp:9997:10.0.2.16:9997 -redir tcp:9998:10.0.2.16:9998 -redir tcp:9999:10.0.2.16:9999 -monitor tcp:127.0.0.1:9000,server,nowait
