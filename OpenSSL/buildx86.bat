@echo off
pushd %~dp0
set var0=VC-WIN32
set var1=..\dist
perl Configure %var0% no-asm enable-ssl3 enable-ssl3-method enable-weak-ssl-ciphers
md %var1%
perl nodebug.pl
nmake /f makefile
copy /y libeay32.dll %var1%\libeay32.dll
copy /y ssleay32.dll %var1%\ssleay32.dll
popd
exit /b
