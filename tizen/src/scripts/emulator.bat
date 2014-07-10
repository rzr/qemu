@echo off

rem find EMULATOR_BIN_PATH
SET EMULATOR_BIN_PATH=%~dp0

rem run emulator
%EMULATOR_BIN_PATH%\emulator-x86 %*
