@ECHO OFF
SETLOCAL EnableDelayedExpansion

SET DATA_DIR=%CD%

SET test_region=monaco
SET test_region_ch=ch\monaco
SET test_region_mld=mld\monaco
SET test_osm=%test_region%.osm.pbf

SET CMD=python -m osrm extract -p %DATA_DIR%\..\..\profiles\car.lua %DATA_DIR%\monaco.osm.pbf
%CMD%
IF !ERRORLEVEL! NEQ 0 (SET EL=!ERRORLEVEL! & GOTO ERROR)

MKDIR ch
XCOPY %test_region%.osrm.* ch\
XCOPY %test_region%.osrm ch\
MKDIR mld
XCOPY %test_region%.osrm.* mld\
XCOPY %test_region%.osrm mld\

SET CMD=python -m osrm contract %test_region_ch%.osrm
%CMD%
IF !ERRORLEVEL! NEQ 0 (SET EL=!ERRORLEVEL! & GOTO ERROR)

SET CMD=python -m osrm partition %test_region_mld%.osrm
%CMD%
IF !ERRORLEVEL! NEQ 0 (SET EL=!ERRORLEVEL! & GOTO ERROR)

SET CMD=python -m osrm customize %test_region_mld%.osrm
%CMD%
IF !ERRORLEVEL! NEQ 0 (SET EL=!ERRORLEVEL! & GOTO ERROR)

GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO Failed command: %CMD%
ECHO Exit code: %EL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
