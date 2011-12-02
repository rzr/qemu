@ECHO OFF
set vtm_shortcut_name=Emulator Manager
set execute_path=Emulator
set vtm_execute_file=emulator-manager.exe
set icon_path=Emulator\skins\icons
set vtm_icon_file=vtm.ico
FOR /F "tokens=2* delims=	 " %%A IN ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Programs') DO SET start_menu_programs_path=%%B
ECHO Start menu path=%start_menu_programs_path%

set program_path=%INSTALLED_PATH%\%execute_path%
set desktop_menu_icon_path=%INSTALLED_PATH%\%icon_path%

echo Program path : %program_path%
echo Desktop menu icon path : %desktop_menu_icon_path%
echo Setting shortcut...
wscript.exe %MAKESHORTCUT_PATH% /shortcut:"%start_menu_programs_path%\%CATEGORY%\%vtm_shortcut_name%" /target:"%program_path%\%vtm_execute_file%" /icon:"%desktop_menu_icon_path%\%vtm_icon_file%"
echo COMPLETE
