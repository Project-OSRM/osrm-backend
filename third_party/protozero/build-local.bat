@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

SET VERBOSITY_MSBUILD=diagnostic
IF NOT "%1"=="" SET VERBOSITY_MSBUILD=%1
SET platform=x64
SET configuration=Release
CALL build-appveyor.bat %VERBOSITY_MSBUILD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET platform=x86
SET configuration=Debug
CALL build-appveyor.bat %VERBOSITY_MSBUILD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO =========== ERROR %~f0 ===========
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
