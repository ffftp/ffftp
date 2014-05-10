REM;
REM;  This is batch-file to prepare for creating ZIP files.
REM;  Please run in "ffftp\Package" directory.
REM;  To change settings, please edit "instaler_config.bat".
REM; 

call installer_config.bat

make_zip_file_routine.vbs "%cd%\%DIR_ZIP_JPN%" "%cd%\%ZIP_JPN%"
make_zip_file_routine.vbs "%cd%\%DIR_ZIP_ENG%" "%cd%\%ZIP_ENG%"
make_zip_file_routine.vbs "%cd%\%AMD64_DIR_ZIP_JPN%" "%cd%\%AMD64_ZIP_JPN%"
make_zip_file_routine.vbs "%cd%\%AMD64_DIR_ZIP_ENG%" "%cd%\%AMD64_ZIP_ENG%"

