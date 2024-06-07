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
cmake -DENABLE_CONAN=ON -DENABLE_NODE_BINDINGS=ON -DCMAKE_BUILD_TYPE=%CONFIGURATION% -G "Visual Studio 17 2022" ..
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

msbuild OSRM.sln ^
/p:Configuration=%CONFIGURATION% ^
/p:Platform=x64 ^
/t:rebuild ^
/p:nowarn="4244;4267;4365;4456;4514;4625;4626;4710;4711;4820;5026;5027" ^
/p:WarningLevel=0 ^
/clp:NoSummary;NoItemAndPropertyList;ErrorsOnly ^
/p:RunCodeAnalysis=false ^
/toolsversion:Current ^
/clp:Verbosity=quiet ^
/nologo
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM /p:BuildInParallel=true ^
@REM /m:2 ^

@REM CD %PROJECT_DIR%\build
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running extractor-tests.exe ...
@REM unit_tests\%CONFIGURATION%\extractor-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running contractor-tests.exe ...
@REM unit_tests\%CONFIGURATION%\contractor-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running engine-tests.exe ...
@REM unit_tests\%CONFIGURATION%\engine-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running util-tests.exe ...
@REM unit_tests\%CONFIGURATION%\util-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running server-tests.exe ...
@REM unit_tests\%CONFIGURATION%\server-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running partitioner-tests.exe ...
@REM unit_tests\%CONFIGURATION%\partitioner-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM ECHO running customizer-tests.exe ...
@REM unit_tests\%CONFIGURATION%\customizer-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM SET test_region=monaco
@REM SET test_region_ch=ch\monaco
@REM SET test_region_mld=mld\monaco
@REM SET test_osm=%test_region%.osm.pbf
@REM COPY %PROJECT_DIR%\test\data\%test_region%.osm.pbf %test_osm%
@REM %CONFIGURATION%\osrm-extract.exe -p %PROJECT_DIR%\profiles\car.lua %test_osm%
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM MKDIR ch
@REM XCOPY %test_region%.osrm.* ch\
@REM XCOPY %test_region%.osrm ch\
@REM MKDIR mld
@REM XCOPY %test_region%.osrm.* mld\
@REM XCOPY %test_region%.osrm mld\
@REM %CONFIGURATION%\osrm-contract.exe %test_region_ch%.osrm
@REM %CONFIGURATION%\osrm-partition.exe %test_region_mld%.osrm
@REM %CONFIGURATION%\osrm-customize.exe %test_region_mld%.osrm
@REM XCOPY /Y ch\*.* ..\test\data\ch\
@REM XCOPY /Y mld\*.* ..\test\data\mld\
@REM unit_tests\%CONFIGURATION%\library-tests.exe
@REM IF %ERRORLEVEL% NEQ 0 GOTO ERROR

@REM :ERROR
@REM ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
@REM ECHO ERRORLEVEL^: %ERRORLEVEL%
@REM SET EL=%ERRORLEVEL%

@REM :DONE
@REM ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@REM EXIT /b %EL%
