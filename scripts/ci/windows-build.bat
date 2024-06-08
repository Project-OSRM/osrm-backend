@ECHO OFF
SETLOCAL
SET EL=0

ECHO NUMBER_OF_PROCESSORS^: %NUMBER_OF_PROCESSORS%

SET PROJECT_DIR=%CD%
SET CONFIGURATION=Release

mkdir build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
cd build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_CONAN=ON -DENABLE_NODE_BINDINGS=ON ..
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
cmake --build . --config Release
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM cmake -DENABLE_CONAN=ON -DENABLE_NODE_BINDINGS=ON -DCMAKE_BUILD_TYPE=%CONFIGURATION% -G "Visual Studio 17 2022" ..
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM msbuild OSRM.sln ^
@REM /p:Configuration=%CONFIGURATION% ^
@REM /p:Platform=x64 ^
@REM /t:rebuild ^
@REM /p:BuildInParallel=true ^
@REM /m:%NUMBER_OF_PROCESSORS% ^
@REM /toolsversion:Current ^
@REM /clp:Verbosity=quiet ^
@REM /nologo
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD %PROJECT_DIR%\build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running extractor-tests.exe ...
unit_tests\%CONFIGURATION%\extractor-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running contractor-tests.exe ...
unit_tests\%CONFIGURATION%\contractor-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running engine-tests.exe ...
unit_tests\%CONFIGURATION%\engine-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running util-tests.exe ...
unit_tests\%CONFIGURATION%\util-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running server-tests.exe ...
unit_tests\%CONFIGURATION%\server-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running partitioner-tests.exe ...
unit_tests\%CONFIGURATION%\partitioner-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO running customizer-tests.exe ...
unit_tests\%CONFIGURATION%\customizer-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET test_region=monaco
SET test_region_ch=ch\monaco
SET test_region_mld=mld\monaco
SET test_osm=%test_region%.osm.pbf
COPY %PROJECT_DIR%\test\data\%test_region%.osm.pbf %test_osm%
%CONFIGURATION%\osrm-extract.exe -p %PROJECT_DIR%\profiles\car.lua %test_osm%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR ch
XCOPY %test_region%.osrm.* ch\
XCOPY %test_region%.osrm ch\
MKDIR mld
XCOPY %test_region%.osrm.* mld\
XCOPY %test_region%.osrm mld\
%CONFIGURATION%\osrm-contract.exe %test_region_ch%.osrm
%CONFIGURATION%\osrm-partition.exe %test_region_mld%.osrm
%CONFIGURATION%\osrm-customize.exe %test_region_mld%.osrm
XCOPY /Y ch\*.* ..\test\data\ch\
XCOPY /Y mld\*.* ..\test\data\mld\
unit_tests\%CONFIGURATION%\library-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
