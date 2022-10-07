@echo off
title m32-pars-adapt test
cd /d %~dp0

set exe="..\msvc\x64-Debug\m32-pars-adapt.exe"
::set exe="m32-pars-adapt.exe"
set src_pth=%UserProfile%\Macotec\Machines\m32-Strato\sde\userdata
set udt_name=MachSettings.udt
set udt_overlay=%src_pth%\configs\machsettings-overlays.txt

set old_udt=MachSettings-old.udt
set loc_udt=~MachSettings.udt
set new_udt1=~MachSettings-theirs.udt
set new_udt2=~MachSettings-mine.udt

:select_test
echo %exe%
echo Choose test:
choice /c asuc /m "Adapt-W, Adapt-S, Update, Confront"
goto menu%errorlevel%


:menu1
rem Test udt adaptation (W)
::set mach=hp(4.0/4.6)
set mach=ActWR-4.0/4.6;no-buf;combo
::set mach=wr/4.0
%exe% --verbose --tgt "%src_pth%\%udt_name%" --db "%udt_overlay%" --machine "%mach%"
goto menu0


:menu2
rem Test udt adaptation (S)
set mach=ActiveF-4.6/3.2-(rot)
%exe% --verbose --tgt "%UserProfile%\Macotec\Machines\m32-StratoS\sde\userdata\%udt_name%" --db "%udt_overlay%" --machine "%mach%"
goto menu0


:menu3
rem Test udt update
%exe% --verbose --tgt "%src_pth%\%udt_name%" --db %old_udt%
goto menu0



:menu4
rem Confront with MachSettings-tool.js
rem Copy here the file
copy /Y "%src_pth%\%udt_name%" "%loc_udt%"
rem Create updated udts
cscript "%src_pth%\configs\MachSettings-tool.js" -tgt="%old_udt%" -tpl="%loc_udt%" -out="%new_udt1%" -quiet
%exe% --verbose --quiet --tgt "%loc_udt%" --db %old_udt% --out "%new_udt2%"
rem Confront them
"%ProgramFiles%\WinMerge\WinMergeU.exe" /e /u "%new_udt1%" "%new_udt2%"
del "%loc_udt%"
del "%new_udt1%"
del "%new_udt2%"
goto menu0


:menu0
echo.
if %errorlevel% neq 0 pause
exit
