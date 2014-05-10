@echo off

REM;
REM;  This is batch-file to create installer files.
REM; 

call make_installer_pre.bat
call make_cab_file.bat
call make_exe_file.bat
call make_zip_file.bat
call make_installer_post.bat
echo Done.
pause

