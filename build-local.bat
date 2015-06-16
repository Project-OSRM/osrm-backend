@ECHO OFF

SET PLATFORM=x64
SET CONFIGURATION=Release

WHERE msbuild
IF %ERRORLEVEL% EQU 0 GOTO RUNBUILD

SET PATH=C:\mb\windows-builds-64\tmp-bin\cmake-3.1.0-win32-x86\bin;%PATH%
SET PATH=C:\Program Files\7-Zip;%PATH%
ECHO activating VS command prompt ...
SET PATH=C:\Program Files (x86)\MSBuild\14.0\Bin;%PATH%
CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

:RUNBUILD

powershell Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted -Force
CALL appveyor-build.bat
EXIT /b %ERRORLEVEL%
