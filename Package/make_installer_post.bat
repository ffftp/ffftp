REM;
REM;  This is batch-file to copy FFFTP installer and archive.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

REM;  copy zip archive
copy /Y %ZIP_JPN% ffftp-%FFFTP_VERSION%.zip
copy /Y %ZIP_ENG% ffftp-%FFFTP_VERSION%-eng.zip

REM; copy installer
copy /Y %INST_JPN% ffftp-%FFFTP_VERSION%.exe
copy /Y %INST_ENG% ffftp-%FFFTP_VERSION%-eng.exe

REM;  copy zip archive
copy /Y %AMD64_ZIP_JPN% ffftp-%FFFTP_VERSION%-64.zip
copy /Y %AMD64_ZIP_ENG% ffftp-%FFFTP_VERSION%-64-eng.zip

REM; copy installer
copy /Y %AMD64_INST_JPN% ffftp-%FFFTP_VERSION%-64.exe
copy /Y %AMD64_INST_ENG% ffftp-%FFFTP_VERSION%-64-eng.exe

