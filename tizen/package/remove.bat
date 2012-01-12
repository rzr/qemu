set vtm_shortcut_name=Emulator Manager
set execute_path=Emulator\bin
set vtm_execute_file=emulator-manager.exe
set program_path=%INSTALLED_PATH%\%execute_path%
set etc_path=%program_path%\etc
IF EXIST %etc_path% (
	ECHO delete %etc_path%	
	del /F /Q %etc_path%
)
:: delims is a TAB followed by a space
FOR /F "tokens=2* delims=	 " %%A IN ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Programs') DO SET start_menu_programs_path=%%B
ECHO Start menu path=%start_menu_programs_path%
echo delete from registry
reg delete "hklm\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"  /f /v %program_path%\%vtm_execute_file%
del "%start_menu_programs_path%\%CATEGORY%\%vtm_shortcut_name%.lnk"
