REM;
REM;  This is batch-file to create FFFTP installer.
REM; 

call installer_config.bat

set EXEPRESS_PATH="%ProgramFiles%\Web Technology\EXEpress 6\EXEpress.exe"
if %PROCESSOR_ARCHITECTURE%==AMD64 set EXEPRESS_PATH="%ProgramFiles(x86)%\Web Technology\EXEpress 6\EXEpress.exe"

%EXEPRESS_PATH% "%cd%\%INI_INST_JPN%"
%EXEPRESS_PATH% "%cd%\%INI_INST_ENG%"
%EXEPRESS_PATH% "%cd%\%AMD64_INI_INST_JPN%"
%EXEPRESS_PATH% "%cd%\%AMD64_INI_INST_ENG%"

