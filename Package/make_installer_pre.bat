REM;
REM;  This is batch-file to prepare for creating FFFTP installer.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

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

REM; copy to installer working directory
copy /Y %DIR_ZIP_JPN%\*.* %DIR_INST_JPN%
copy /Y %DIR_ZIP_ENG%\*.* %DIR_INST_ENG%
copy /Y %AMD64_DIR_ZIP_JPN%\*.* %AMD64_DIR_INST_JPN%
copy /Y %AMD64_DIR_ZIP_ENG%\*.* %AMD64_DIR_INST_ENG%

REM; copy a portable indicator file
copy /Y %PORTABLE_FILE% %DIR_ZIP_JPN%
copy /Y %PORTABLE_FILE% %DIR_ZIP_ENG%
copy /Y %PORTABLE_FILE% %AMD64_DIR_ZIP_JPN%
copy /Y %PORTABLE_FILE% %AMD64_DIR_ZIP_ENG%

