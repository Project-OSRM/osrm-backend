@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

::show all available env vars
SET
ECHO cmake on AppVeyor
cmake -version

ECHO activating VS cmd prompt && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET protobuf_sdk=protozero-dep-protobuf-2.6.1.7z
IF EXIST %protobuf_sdk% (ECHO protobuf already downloaded) ELSE (ECHO downloading protobuf ... && powershell Invoke-WebRequest https://mapbox.s3.amazonaws.com/windows-builds/windows-build-deps/$env:protobuf_sdk -OutFile $pwd\$env:protobuf_sdk)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF EXIST deps\protobuf (ECHO protobuf already extracted) ELSE (CALL 7z x -y %protobuf_sdk% | %windir%\system32\FIND "ing archive")
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
SET PATH=%~dp0deps\protobuf;%PATH%

IF EXIST build ECHO deleting build dir... && RD /Q /S build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD build
ECHO config^: %config%

::This will produce lots of LNK4099 warnings which can be ignored.
::Unfortunately they can't be disabled, see
::http://stackoverflow.com/questions/661606/visual-c-how-to-disable-specific-linker-warnings
SET CMAKE_CMD=cmake .. ^
-LA -G "Visual Studio 14 Win64"

ECHO calling^: %CMAKE_CMD%
%CMAKE_CMD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET avlogger=
IF /I "%APPVEYOR%"=="True" SET avlogger=/logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

msbuild protozero.sln ^
/p:Configuration=%config% ^
/toolsversion:14.0 ^
/p:Platform=x64 ^
/p:PlatformToolset=v140 %avlogger%
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
