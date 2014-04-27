REM;
REM;  This is batch-file to create FFFTP installer.
REM; 

call installer_config.bat

set EXEPRESS_PATH="%ProgramFiles%\Web Technology\EXEpress CX 5\EXEpress.exe"
if %PROCESSOR_ARCHITECTURE%==AMD64 set EXEPRESS_PATH="%ProgramFiles(x86)%\Web Technology\EXEpress CX 5\EXEpress.exe"

%EXEPRESS_PATH% %cd%\%INI_JPN_INST%
%EXEPRESS_PATH% %cd%\%INI_ENG_INST%
%EXEPRESS_PATH% %cd%\%AMD64_INI_JPN_INST%
%EXEPRESS_PATH% %cd%\%AMD64_INI_ENG_INST%

