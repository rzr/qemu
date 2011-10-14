@echo off

IF NOT EXIST vtm.exe (
:: delims is a TAB followed by a space
FOR /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKCU\Software\JavaSoft\Prefs" /v slpsdk-installpath') DO SET sdk_path=%%B
)

IF NOT EXIST vtm.exe (
cd %sdk_path%
cd Emulator
)

vtm.exe 
