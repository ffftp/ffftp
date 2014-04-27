REM;
REM;  This is batch-file to copy FFFTP installer and archive.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

REM;  copy zip archive
copy /Y %JPN_ZIP% ffftp-%FFFTP_VERSION%.zip
copy /Y %ENG_ZIP% ffftp-%FFFTP_VERSION%-eng.zip

REM; copy installer
copy /Y %JPN_INST% ffftp-%FFFTP_VERSION%.exe
copy /Y %ENG_INST% ffftp-%FFFTP_VERSION%-eng.exe

REM;  copy zip archive
copy /Y %AMD64_JPN_ZIP% ffftp-%FFFTP_VERSION%-64.zip
copy /Y %AMD64_ENG_ZIP% ffftp-%FFFTP_VERSION%-64-eng.zip

REM; copy installer
copy /Y %AMD64_JPN_INST% ffftp-%FFFTP_VERSION%-64.exe
copy /Y %AMD64_ENG_INST% ffftp-%FFFTP_VERSION%-64-eng.exe

