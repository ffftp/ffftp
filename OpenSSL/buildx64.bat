@echo off
pushd %~dp0
set var0=VC-WIN64A
set var1=..\dist\amd64
set var2=ms\do_win64a.bat
perl Configure %var0% no-asm --prefix=.
md %var1%
call %var2%
perl nodebug.pl
nmake /f ms\ntdll.mak
copy /y out32dll\libeay32.dll %var1%\libeay32.dll
copy /y out32dll\ssleay32.dll %var1%\ssleay32.dll
popd
exit /b
