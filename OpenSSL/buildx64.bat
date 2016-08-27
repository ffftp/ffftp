@echo off
pushd %~dp0
set var0=VC-WIN64A
set var1=..\dist\amd64
perl Configure %var0% no-asm --prefix="%cd%"
md %var1%
perl nodebug.pl
nmake /f makefile
copy /y libeay32.dll %var1%\libeay32.dll
copy /y ssleay32.dll %var1%\ssleay32.dll
popd
exit /b
