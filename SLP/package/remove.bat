set vtm_shortcut_name=Virtual Target Manager
:: delims is a TAB followed by a space
FOR /F "tokens=2* delims=	 " %%A IN ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Programs') DO SET start_menu_programs_path=%%B
ECHO Start menu path=%start_menu_programs_path%

del "%start_menu_programs_path%\Samsung Linux Platform\%vtm_shortcut_name%.lnk"
