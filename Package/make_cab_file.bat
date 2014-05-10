REM;
REM;  This is batch-file to prepare for creating FFFTP installer.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

call make_cab_file_routine.bat "%cd%\%DIR_INST_JPN%\.." ffftp
call make_cab_file_routine.bat "%cd%\%DIR_INST_ENG%\.." ffftp
call make_cab_file_routine.bat "%cd%\%AMD64_DIR_INST_JPN%\.." ffftp
call make_cab_file_routine.bat "%cd%\%AMD64_DIR_INST_ENG%\.." ffftp

