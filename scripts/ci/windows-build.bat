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
cmake -DENABLE_CONAN=ON -DCMAKE_BUILD_TYPE=%CONFIGURATION% -DBUILD_TOOLS=ON -G "Visual Studio 17 2022" ..
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

msbuild OSRM.sln ^
/p:Configuration=%CONFIGURATION% ^
/p:Platform=x64 ^
/t:rebuild ^
/p:BuildInParallel=true ^
/m:%NUMBER_OF_PROCESSORS% ^
/toolsversion:Current ^
/clp:Verbosity=normal ^
/nologo
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

dir C:\Users\runneradmin\.conan\
dir C:\Users\runneradmin\.conan\data\tbb\2020.3\_\_\package\e9a552ebe8f994398de9ceee972f0ad207df0658\lib\
SET PATH=C:\Users\runneradmin\.conan\data\tbb\2020.3\_\_\package\e9a552ebe8f994398de9ceee972f0ad207df0658\lib\;%PATH%

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
SET test_region_corech=corech\monaco
SET test_region_mld=mld\monaco
SET test_osm=%test_region%.osm.pbf
COPY %PROJECT_DIR%\test\data\%test_region%.osm.pbf %test_osm% 
%CONFIGURATION%\osrm-extract.exe -p %PROJECT_DIR%\profiles\car.lua %test_osm%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR ch
XCOPY %test_region%.osrm.* ch\
XCOPY %test_region%.osrm ch\
MKDIR corech
XCOPY %test_region%.osrm.* corech\
XCOPY %test_region%.osrm corech\
MKDIR mld
XCOPY %test_region%.osrm.* mld\
XCOPY %test_region%.osrm mld\
%CONFIGURATION%\osrm-contract.exe %test_region_ch%.osrm
%CONFIGURATION%\osrm-contract.exe --core 0.8 %test_region_corech%.osrm
%CONFIGURATION%\osrm-partition.exe %test_region_mld%.osrm
%CONFIGURATION%\osrm-customize.exe %test_region_mld%.osrm
XCOPY /Y ch\*.* ..\test\data\ch\
XCOPY /Y corech\*.* ..\test\data\corech\
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
