@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

::show all available env vars
SET
ECHO cmake on AppVeyor
cmake -version

ECHO activating VS cmd prompt && CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF EXIST build ECHO deleting build dir... && RD /Q /S build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD build
ECHO config^: %config%

SET CMAKE_CMD=cmake .. ^
-LA -G "Visual Studio 15 2017 Win64"

ECHO calling^: %CMAKE_CMD%
%CMAKE_CMD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET avlogger=
IF /I "%APPVEYOR%"=="True" SET avlogger=/logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

msbuild vtzero.sln ^
/p:Configuration=%config% ^
%avlogger%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ctest --output-on-failure ^
-C %config% ^
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%
