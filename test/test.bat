@echo off
title m32-pars-adapt test
cd /d %~dp0

rem settings
set old_udt=MachSettings-old.udt
set src_pth_w=%UserProfile%\Macotec\Machines\m32-Strato\sde
set src_pth_s=%UserProfile%\Macotec\Machines\m32-StratoS\sde
set udt_w=%src_pth_w%\userdata\MachSettings.udt
set udt_s=%src_pth_s%\userdata\MachSettings.udt
set udt_db=%src_pth_w%\userdata\configs\machsettings-overlays.txt
set parax_w=%src_pth_w%\param\par2kax.txt
set parax_s=%src_pth_s%\param\par2kax.txt
set parax_db=%src_pth_w%\param\configs\par2kax-overlays.txt
set exe="..\.msvc\x64-Debug\m32-pars-adapt.exe"
::set exe="m32-pars-adapt.exe"
echo %exe%


:select_test
echo.
echo -----------
echo Choose test:
choice /N /C 123456q /M "[1]Update, [2]Adapt-W, [3]Adapt-S, [4]ParAx-W, [5]ParAx-S, [6]Confront, [q]Quit: "
goto menu%errorlevel%

:menu0
goto select_test


:menu1
rem Test udt update
%exe% -v -tgt "%udt_w%" -db %old_udt%
echo ret=%errorlevel%
goto select_test


:menu2
rem Test udt adaptation (W)
::set mach=hp(4.0/4.6)
set mach=ActWR-4.0/4.6;no-buf;combo
::set mach=wr/4.0
%exe% -v -tgt "%udt_w%" -db "%udt_db%" -m "%mach%"
echo ret=%errorlevel%
goto select_test


:menu3
rem Test udt adaptation (S)
set mach=ActiveF-4.6/3.2-(rot)
%exe% -v -tgt "%udt_s%" -db "%udt_db%" -m "%mach%"
echo ret=%errorlevel%
goto select_test


:menu4
rem Test parax adaptation (W)
set mach=ActHP-4.9/4.6;no-buf;combo
%exe% -v -tgt "%parax_w%" -db "%parax_db%" -m "%mach%"
echo ret=%errorlevel%
goto select_test


:menu5
rem Test parax adaptation (S)
set mach=ActiveF-3.7-buf
%exe% -v -tgt "%parax_s%" -db "%parax_db%" -m "%mach%"
echo ret=%errorlevel%
goto select_test


:menu6
rem Confront
set loc_udt=~MachSettings.udt
set new_udt_theirs=~MachSettings-theirs.udt
set new_udt_mine=~MachSettings-mine.udt
rem Copy here the file
copy /Y "%udt_w%" "%loc_udt%"
rem Create my updated udt
%exe% -v -q --tgt "%loc_udt%" --db %old_udt% --out "%new_udt_mine%"
echo ret=%errorlevel%
rem Create the other
rem with MachSettings-tool.js
::cscript "%src_pth_w%\userdata\configs\MachSettings-tool.js" -tgt="%old_udt%" -tpl="%loc_udt%" -out="%new_udt_theirs%" -quiet
rem with previous m32-pars-adapt
m32-pars-adapt -v -q --tgt "%loc_udt%" --db %old_udt% --out "%new_udt_theirs%"
rem Confront them
"%ProgramFiles%\WinMerge\WinMergeU.exe" /e /u "%new_udt_theirs%" "%new_udt_mine%"
del "%loc_udt%"
del "%new_udt_theirs%"
del "%new_udt_mine%"
goto select_test


:menu7
exit
