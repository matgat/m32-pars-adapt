@echo off
title m32-pars-adapt test
cd /d %~dp0

rem settings
set old_udt=MachSettings-old.udt
set udt_name=MachSettings.udt
set parax_name=par2kax.txt
set udt_db=configs\machsettings-overlays.txt
set parax_db=configs\par2kax-overlays.txt
set src_pth_w=%UserProfile%\Macotec\Machines\m32-Strato\sde
set src_pth_f=%UserProfile%\Macotec\Machines\m32-Strato\families\ActiveF
set udtdir_w=%src_pth_w%\userdata
set udtdir_f=%src_pth_f%\userdata
set pardir_w=%src_pth_w%\param
set pardir_f=%src_pth_f%\param
set exe="..\.msvc\x64-Debug\m32-pars-adapt.exe"
::set exe="m32-pars-adapt.exe"
echo %exe%


:select_test
echo.
echo -----------
echo Choose test:
choice /N /C 123456q /M "[1]Update, [2]Adapt-W, [3]Adapt-F, [4]ParAx-W, [5]ParAx-F, [6]Confront, [q]Quit: "
goto menu%errorlevel%

:menu0
goto select_test


:menu1
rem Test udt update
%exe% -v -tgt "%udtdir_w%\%udt_name%" -db %old_udt%
echo ---- ret=%errorlevel% ----
goto select_test


:menu2
rem Test udt adaptation (W)
::set mach=hp(4.0/4.6)
set mach=ActWR-4.0/4.6;no-buf;combo
::set mach=wr/4.0
%exe% -v -tgt "%udtdir_w%\%udt_name%" -db "%udtdir_w%\%udt_db%" -m "%mach%"
echo ---- ret=%errorlevel% ----
goto select_test


:menu3
rem Test udt adaptation (F)
set mach=ActiveF-4.6/3.2-(lowe,buf)
%exe% -v -tgt "%udtdir_f%\%udt_name%" -db "%udtdir_f%\%udt_db%" -m "%mach%"
echo ---- ret=%errorlevel% ----
goto select_test


:menu4
rem Test parax adaptation (W)
set mach=ActHP-4.9/4.6;no-buf;combo
%exe% -v -tgt "%pardir_w%\%parax_name%" -db "%pardir_w%\%parax_db%" -m "%mach%"
echo ---- ret=%errorlevel% ----
goto select_test


:menu5
rem Test parax adaptation (F)
set mach=ActiveF-3.7-buf
%exe% -v -tgt "%pardir_f%\%parax_name%" -db "%pardir_f%\%parax_db%" -m "%mach%"
echo ---- ret=%errorlevel% ----
goto select_test


:menu6
rem Confront
set loc_udt=~MachSettings.udt
set new_udt_theirs=~MachSettings-theirs.udt
set new_udt_mine=~MachSettings-mine.udt
rem Copy here the file
copy /Y "%udtdir_w%\%udt_name%" "%loc_udt%"
rem Create my updated udt
%exe% -v -q --tgt "%loc_udt%" --db %old_udt% --out "%new_udt_mine%"
echo ---- ret=%errorlevel% ----
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
