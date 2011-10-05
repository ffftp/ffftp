REM;
REM;  This is batch-file to prepare for creating FFFTP installer.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

REM;  copy Japanese files
mkdir %DIR_JPN%
copy /Y %BIN_JPN% %DIR_JPN%
copy /Y %HTMLHELP% %DIR_JPN%
copy /Y %DIR_JPN_DOC%\*.txt %DIR_JPN%
copy /Y %DIR_DIST%\*.dll %DIR_JPN%
copy /Y %DIR_DIST%\*.manifest %DIR_JPN%

REM;  copy English files
mkdir %DIR_ENG%
copy /Y %BIN_ENG% %DIR_ENG%
copy /Y %DIR_ENG_DOC%\*.txt %DIR_ENG%
copy /Y %DIR_DIST%\*.dll %DIR_ENG%
copy /Y %DIR_DIST%\*.manifest %DIR_ENG%

REM; copy to installer working directory
copy /Y %DIR_JPN%\*.* %DIR_JPN_INST%
copy /Y %DIR_ENG%\*.* %DIR_ENG_INST%

pause
