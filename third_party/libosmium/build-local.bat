@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

ECHO.
ECHO build-local ["config=Dev"]
ECHO default config^: RelWithDebInfo
ECHO.

SET platform=x64
SET config=RelWithDebInfo

:: OVERRIDE PARAMETERS >>>>>>>>
:NEXT-ARG

IF '%1'=='' GOTO ARGS-DONE
ECHO setting %1
SET %1
SHIFT
GOTO NEXT-ARG

:ARGS-DONE
::<<<<< OVERRIDE PARAMETERS

WHERE 7z
IF %ERRORLEVEL% NEQ 0 ECHO 7zip not on PATH && GOTO ERROR

CALL build-appveyor.bat
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%
