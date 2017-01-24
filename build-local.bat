@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SET PLATFORM=x64
SET CONFIGURATION=Release
::SET LOCAL_DEV=1

FOR /F "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD') do SET APPVEYOR_REPO_BRANCH=%%i
ECHO APPVEYOR_REPO_BRANCH^: %APPVEYOR_REPO_BRANCH%

SET PATH=C:\mb\windows-builds-64\tmp-bin\cmake-3.7.0-rc2-win32-x86\bin;%PATH%
SET PATH=C:\Program Files\7-Zip;%PATH%

powershell Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted -Force
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
CALL appveyor-build.bat
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE


:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
