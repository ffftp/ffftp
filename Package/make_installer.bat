@echo off

REM;
REM;  This is batch-file to create installer files.
REM; 

call make_installer_pre.bat
call make_cab_file.bat
call make_exe_file.bat
echo Please create ZIP files manually and then resume this.
pause
call make_installer_post.bat
echo Done.
pause

