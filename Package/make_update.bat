REM;
REM;  This is batch-file to prepare for creating FFFTP update.
REM;  Please run in "ffftp\Package" directory.
REM; 

call installer_config.bat

set PRIVATE_KEY=private.pem
set /p PRIVATE_KEY_PW=Password:
set PREFIX_JPN=update.jpn.file.
set PREFIX_ENG=update.eng.file.
set PREFIX_AMD64_JPN=update.amd64.jpn.file.
set PREFIX_AMD64_ENG=update.amd64.eng.file.
set DESC_JPN="OpenSSLÇçXêVÇµÇ‹ÇµÇΩÅB"
set DESC_ENG="Updated OpenSSL."
set DESC_AMD64_JPN=%DESC_JPN%
set DESC_AMD64_ENG=%DESC_ENG%

set DIR_UPDATE=update
set DIR_ZIP_JPN=%DIR_UPDATE%\jpn\file
set DIR_ZIP_ENG=%DIR_UPDATE%\eng\file
set AMD64_DIR_ZIP_JPN=%DIR_UPDATE%\amd64\jpn\file
set AMD64_DIR_ZIP_ENG=%DIR_UPDATE%\amd64\eng\file

REM;  copy Japanese files
mkdir %DIR_ZIP_JPN%
copy /Y %BIN_JPN% %DIR_ZIP_JPN%
copy /Y %HTMLHELP% %DIR_ZIP_JPN%
copy /Y %DIR_JPN_DOC%\*.txt %DIR_ZIP_JPN%
copy /Y %DIR_DIST%\*.dll %DIR_ZIP_JPN%
copy /Y %DIR_DIST%\*.manifest %DIR_ZIP_JPN%
copy /Y %DIR_DIST%\*.pem %DIR_ZIP_JPN%

REM;  copy English files
mkdir %DIR_ZIP_ENG%
copy /Y %BIN_ENG% %DIR_ZIP_ENG%
copy /Y %DIR_ENG_DOC%\*.txt %DIR_ZIP_ENG%
copy /Y %DIR_DIST%\*.dll %DIR_ZIP_ENG%
copy /Y %DIR_DIST%\*.manifest %DIR_ZIP_ENG%
copy /Y %DIR_DIST%\*.pem %DIR_ZIP_ENG%

REM;  copy Japanese files
mkdir %AMD64_DIR_ZIP_JPN%
copy /Y %BIN_AMD64_JPN% %AMD64_DIR_ZIP_JPN%
copy /Y %HTMLHELP% %AMD64_DIR_ZIP_JPN%
copy /Y %DIR_JPN_DOC%\*.txt %AMD64_DIR_ZIP_JPN%
copy /Y %DIR_DIST%\amd64\*.dll %AMD64_DIR_ZIP_JPN%
copy /Y %DIR_DIST%\amd64\*.manifest %AMD64_DIR_ZIP_JPN%
copy /Y %DIR_DIST%\*.pem %AMD64_DIR_ZIP_JPN%

REM;  copy English files
mkdir %AMD64_DIR_ZIP_ENG%
copy /Y %BIN_AMD64_ENG% %AMD64_DIR_ZIP_ENG%
copy /Y %DIR_ENG_DOC%\*.txt %AMD64_DIR_ZIP_ENG%
copy /Y %DIR_DIST%\amd64\*.dll %AMD64_DIR_ZIP_ENG%
copy /Y %DIR_DIST%\amd64\*.manifest %AMD64_DIR_ZIP_ENG%
copy /Y %DIR_DIST%\*.pem %AMD64_DIR_ZIP_ENG%

"%cd%\%DIR_ZIP_JPN%\FFFTP.exe" --build-software-update "%cd%\%PRIVATE_KEY%" "%PRIVATE_KEY_PW%" "/dl/ffftp/%PREFIX_JPN%" "%cd%\%DIR_UPDATE%\update.jpn.hash" "%cd%\%DIR_UPDATE%\update.jpn.list" %DESC_JPN%
"%cd%\%DIR_ZIP_ENG%\FFFTP.exe" --build-software-update "%cd%\%PRIVATE_KEY%" "%PRIVATE_KEY_PW%" "/dl/ffftp/%PREFIX_ENG%" "%cd%\%DIR_UPDATE%\update.eng.hash" "%cd%\%DIR_UPDATE%\update.eng.list" %DESC_ENG%
"%cd%\%AMD64_DIR_ZIP_JPN%\FFFTP.exe" --build-software-update "%cd%\%PRIVATE_KEY%" "%PRIVATE_KEY_PW%" "/dl/ffftp/%PREFIX_AMD64_JPN%" "%cd%\%DIR_UPDATE%\update.amd64.jpn.hash" "%cd%\%DIR_UPDATE%\update.amd64.jpn.list" %DESC_AMD64_JPN%
"%cd%\%AMD64_DIR_ZIP_ENG%\FFFTP.exe" --build-software-update "%cd%\%PRIVATE_KEY%" "%PRIVATE_KEY_PW%" "/dl/ffftp/%PREFIX_AMD64_ENG%" "%cd%\%DIR_UPDATE%\update.amd64.eng.hash" "%cd%\%DIR_UPDATE%\update.amd64.eng.list" %DESC_AMD64_ENG%

pushd %DIR_ZIP_JPN%
for %%i in (*) do ren %%i %PREFIX_JPN%%%i
popd
pushd %DIR_ZIP_ENG%
for %%i in (*) do ren %%i %PREFIX_ENG%%%i
popd
pushd %AMD64_DIR_ZIP_JPN%
for %%i in (*) do ren %%i %PREFIX_AMD64_JPN%%%i
popd
pushd %AMD64_DIR_ZIP_ENG%
for %%i in (*) do ren %%i %PREFIX_AMD64_ENG%%%i
popd

pushd %DIR_UPDATE%
for /d %%i in (*) do call :sub0 "%%~i"
popd
exit /b
:sub0
pushd "%~1"
for /d %%i in (*) do call :sub0 "%%~i"
for %%i in (*) do move "%%~i" "..\%%~i"
popd
rd "%~1"
exit /b

