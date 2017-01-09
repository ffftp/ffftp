@echo off
pushd %~dp0
set var0=VC-WIN64A
set var1=..\dist\amd64
rem Use compat51.bat to support Windows XP or later.
rem Use compat50.bat to support Windows 2000.
call compat50.bat
perl nodebug.pl
nmake /f makefile build_libs
copy /y libeay32.dll %var1%\libeay32.dll
copy /y ssleay32.dll %var1%\ssleay32.dll
popd
exit /b
