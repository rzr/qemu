@ECHO OFF
set emul_shortcut_name=SLP Emulator
set vtm_shortcut_name=Virtual Target Manager
set execute_path=Emulator
set emul_execute_file=emulator.vbs
set vtm_execute_file=vtm.vbs
set icon_path=Emulator\skins\icons
set emul_icon_file=emulator.ico
set vtm_icon_file=vtm.ico
:: Get installed path of SLP-SDK. 
FOR /F "tokens=2* delims=	 " %%A IN ('reg query "HKEY_CURRENT_USER\Software\JavaSoft\Prefs" /v slpsdk-installpath') DO SET installed_path=%%B
ECHO Installed Path=%installed_path%
FOR /F "tokens=2* delims=	 " %%A IN ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Programs') DO SET start_menu_programs_path=%%B
ECHO Start menu path=%start_menu_programs_path%

set program_path=%installed_path%\%execute_path%
set desktop_menu_icon_path=%installed_path%\%icon_path%

echo Program path : %program_path%
echo Desktop menu icon path : %desktop_menu_icon_path%
echo Setting shortcut...
wscript.exe %CD%/res/desktop_directory/makeshortcut.vbs /shortcut:"%start_menu_programs_path%\Samsung Linux Platform\%emul_shortcut_name%" /target:"%program_path%\%emul_execute_file%" /icon:"%desktop_menu_icon_path%\%emul_icon_file%"
wscript.exe %CD%/res/desktop_directory/makeshortcut.vbs /shortcut:"%start_menu_programs_path%\Samsung Linux Platform\%vtm_shortcut_name%" /target:"%program_path%\%vtm_execute_file%" /icon:"%desktop_menu_icon_path%\%vtm_icon_file%"
echo COMPLETE
