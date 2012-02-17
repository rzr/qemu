set vtm_shortcut_name=Emulator Manager
set execute_path=Emulator\bin
set vtm_execute_file=emulator-manager.exe
set program_path=%INSTALLED_PATH%\%execute_path%
set etc_path=%program_path%\etc
IF EXIST %etc_path% (
	ECHO delete %etc_path%	
	del /F /Q %etc_path%
)
ECHO Start menu path=%start_menu_programs_path%
echo delete from registry
reg delete "hklm\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"  /f /v %program_path%\%vtm_execute_file%
wscript.exe %REMOVE_SHORTCUT% /shortcut:%vtm_shortcut_name%

