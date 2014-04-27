REM;
REM;  Please do not run this directly.
REM; 

set DIR_CAB=%~1
set CAB_NAME=%~2
set SED_FILE=%DIR_CAB%\%CAB_NAME%.sed
echo [Version] > %SED_FILE%
echo Class=IEXPRESS >> %SED_FILE%
echo SEDVersion=3 >> %SED_FILE%
echo [Options] >> %SED_FILE%
echo PackagePurpose=CreateCAB >> %SED_FILE%
echo ShowInstallProgramWindow=0 >> %SED_FILE%
echo HideExtractAnimation=0 >> %SED_FILE%
echo UseLongFileName=1 >> %SED_FILE%
echo InsideCompressed=0 >> %SED_FILE%
echo CAB_FixedSize=0 >> %SED_FILE%
echo CAB_ResvCodeSigning=6144 >> %SED_FILE%
echo RebootMode=I >> %SED_FILE%
echo InstallPrompt=%%InstallPrompt%% >> %SED_FILE%
echo DisplayLicense=%%DisplayLicense%% >> %SED_FILE%
echo FinishMessage=%%FinishMessage%% >> %SED_FILE%
echo TargetName=%%TargetName%% >> %SED_FILE%
echo FriendlyName=%%FriendlyName%% >> %SED_FILE%
echo AppLaunched=%%AppLaunched%% >> %SED_FILE%
echo PostInstallCmd=%%PostInstallCmd%% >> %SED_FILE%
echo AdminQuietInstCmd=%%AdminQuietInstCmd%% >> %SED_FILE%
echo UserQuietInstCmd=%%UserQuietInstCmd%% >> %SED_FILE%
echo SourceFiles=SourceFiles >> %SED_FILE%
echo [Strings] >> %SED_FILE%
echo InstallPrompt= >> %SED_FILE%
echo DisplayLicense= >> %SED_FILE%
echo FinishMessage= >> %SED_FILE%
echo TargetName=%CAB_NAME%.cab >> %SED_FILE%
echo FriendlyName=IExpress Wizard >> %SED_FILE%
echo AppLaunched= >> %SED_FILE%
echo PostInstallCmd= >> %SED_FILE%
echo AdminQuietInstCmd= >> %SED_FILE%
echo UserQuietInstCmd= >> %SED_FILE%
set PREV_CD=%cd%
cd %DIR_CAB%\%CAB_NAME%
for %%i in (*) do echo FILE%%i=%%i >> %SED_FILE%
cd %PREV_CD%
echo [SourceFiles] >> %SED_FILE%
echo SourceFiles0=%CAB_NAME%\ >> %SED_FILE%
echo [SourceFiles0] >> %SED_FILE%
set PREV_CD=%cd%
cd %DIR_CAB%\%CAB_NAME%
for %%i in (*) do echo %%FILE%%i%%= >> %SED_FILE%
cd %PREV_CD%
set PREV_CD=%cd%
cd %DIR_CAB%
iexpress /N %CAB_NAME%.sed
cd %PREV_CD%

