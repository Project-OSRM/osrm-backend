@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SET PROJECT_DIR=%CD%
ECHO PROJECT_DIR^: %PROJECT_DIR%
ECHO NUMBER_OF_PROCESSORS^: %NUMBER_OF_PROCESSORS%


:: Check CMake version
SET CMAKE_VERSION=3.16.3
SET PATH=%PROJECT_DIR%\cmake-%CMAKE_VERSION%-win32-x86\bin;%PATH%
ECHO cmake^: && cmake --version
IF %ERRORLEVEL% NEQ 0 ECHO CMAKE not found && GOTO CMAKE_NOT_OK

cmake --version | findstr /C:%CMAKE_VERSION% && GOTO CMAKE_OK

:CMAKE_NOT_OK
ECHO CMAKE NOT OK - downloading new CMake %CMAKE_VERSION%
powershell Invoke-WebRequest https://cmake.org/files/v3.16/cmake-%CMAKE_VERSION%-win32-x86.zip -OutFile $env:PROJECT_DIR\cm.zip
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF NOT EXIST cmake-%CMAKE_VERSION%-win32-x86 7z -y x cm.zip | %windir%\system32\FIND "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:CMAKE_OK
ECHO CMAKE_OK
cmake --version

ECHO activating VS command prompt ...
SET PATH=C:\Program Files (x86)\MSBuild\15.0\Bin;%PATH%
CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

ECHO platform^: %platform%

ECHO cl.exe version
cl
ECHO msbuild version
msbuild /version

:: HARDCODE "x64" as it is uppercase on AppVeyor and download from S3 is case sensitive
SET DEPSPKG=osrm-deps-win-x64-14.2-2019.01.7z

:: local development
ECHO.
ECHO LOCAL_DEV^: %LOCAL_DEV%
IF NOT DEFINED LOCAL_DEV SET LOCAL_DEV=0
IF DEFINED LOCAL_DEV IF %LOCAL_DEV% EQU 1 IF EXIST %DEPSPKG% ECHO skipping deps download && GOTO SKIPDL

IF EXIST %DEPSPKG% DEL %DEPSPKG%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO downloading %DEPSPKG%
powershell Invoke-WebRequest http://project-osrm.wolt.com/windows-build-deps/$env:DEPSPKG -OutFile $env:PROJECT_DIR\$env:DEPSPKG
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:SKIPDL

IF EXIST osrm-deps ECHO deleting osrm-deps... && RD /S /Q osrm-deps
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF EXIST build ECHO deleting build dir... && RD /S /Q build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

7z -y x %DEPSPKG% | %windir%\system32\FIND "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

::tree osrm-deps

MKDIR build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
cd build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET OSRMDEPSDIR=%PROJECT_DIR%/osrm-deps
set PREFIX=%OSRMDEPSDIR%/libs
set BOOST_ROOT=%OSRMDEPSDIR%
set BOOST_LIBRARYDIR=%BOOST_ROOT%/lib
set TBB_INSTALL_DIR=%OSRMDEPSDIR%
REM set TBB_ARCH_PLATFORM=intel64/vc17

ECHO OSRMDEPSDIR       ^: %OSRMDEPSDIR%
ECHO PREFIX            ^: %PREFIX%
ECHO BOOST_ROOT        ^: %BOOST_ROOT%
ECHO BOOST_LIBRARYDIR  ^: %BOOST_LIBRARYDIR%
ECHO TBB_INSTALL_DIR   ^: %TBB_INSTALL_DIR%
REM ECHO TBB_ARCH_PLATFORM ^: %TBB_ARCH_PLATFORM%


ECHO calling cmake ....
cmake .. ^
-G "Visual Studio 16 2019" ^
-DBOOST_ROOT=%BOOST_ROOT% ^
-DBOOST_LIBRARYDIR=%BOOST_LIBRARYDIR% ^
-DBoost_ADDITIONAL_VERSIONS=1.73.0 ^
-DBoost_USE_MULTITHREADED=ON ^
-DBoost_USE_STATIC_LIBS=ON ^
-DEXPAT_INCLUDE_DIR=%OSRMDEPSDIR% ^
-DEXPAT_LIBRARY=%OSRMDEPSDIR%/lib/libexpat.lib ^
-DBZIP2_INCLUDE_DIR=%OSRMDEPSDIR% ^
-DBZIP2_LIBRARIES=%OSRMDEPSDIR%/lib/libbz2.lib ^
-DLUA_INCLUDE_DIR=%OSRMDEPSDIR% ^
-DLUA_LIBRARIES=%OSRMDEPSDIR%/lib/lua5.3.5.lib ^
-DZLIB_INCLUDE_DIR=%OSRMDEPSDIR% ^
-DZLIB_LIBRARY=%OSRMDEPSDIR%/lib/libz.lib ^
-DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
-DCMAKE_INSTALL_PREFIX=%PREFIX%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO building ...
msbuild OSRM.sln ^
/p:Configuration=%Configuration% ^
/p:Platform=x64 ^
/t:rebuild ^
/p:BuildInParallel=true ^
/m:%NUMBER_OF_PROCESSORS% ^
/toolsversion:Current ^
/p:PlatformToolset=v142 ^
/clp:Verbosity=normal ^
/nologo ^
/flp1:logfile=build_errors.txt;errorsonly ^
/flp2:logfile=build_warnings.txt;warningsonly
IF %ERRORLEVEL% EQU 1 GOTO ERROR

CD %PROJECT_DIR%\build
IF %ERRORLEVEL% EQU 1 GOTO ERROR

SET PATH=%PROJECT_DIR%\osrm-deps\lib;%PATH%

ECHO running extractor-tests.exe ...
unit_tests\%Configuration%\extractor-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running contractor-tests.exe ...
unit_tests\%Configuration%\contractor-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running engine-tests.exe ...
unit_tests\%Configuration%\engine-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running util-tests.exe ...
unit_tests\%Configuration%\util-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running server-tests.exe ...
unit_tests\%Configuration%\server-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running partitioner-tests.exe ...
unit_tests\%Configuration%\partitioner-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running customizer-tests.exe ...
unit_tests\%Configuration%\customizer-tests.exe
IF %ERRORLEVEL% EQU 1 GOTO ERROR

ECHO running library-tests.exe ...
SET test_region=monaco
SET test_region_ch=ch\monaco
SET test_region_corech=corech\monaco
SET test_region_mld=mld\monaco
SET test_osm=%test_region%.osm.pbf
IF NOT EXIST %test_osm% powershell Invoke-WebRequest http://project-osrm.wolt.com/testing/monaco.osm.pbf -OutFile %test_osm%
ECHO running %Configuration%\osrm-extract.exe -p ../profiles/car.lua %test_osm%
%Configuration%\osrm-extract.exe
%Configuration%\osrm-extract.exe -p ../profiles/car.lua %test_osm%
MKDIR ch
XCOPY %test_region%.osrm.* ch\
XCOPY %test_region%.osrm ch\
MKDIR corech
XCOPY %test_region%.osrm.* corech\
XCOPY %test_region%.osrm corech\
MKDIR mld
XCOPY %test_region%.osrm.* mld\
XCOPY %test_region%.osrm mld\
%Configuration%\osrm-contract.exe %test_region_ch%.osrm
%Configuration%\osrm-contract.exe --core 0.8 %test_region_corech%.osrm
%Configuration%\osrm-partition.exe %test_region_mld%.osrm
%Configuration%\osrm-customize.exe %test_region_mld%.osrm
XCOPY /Y ch\*.* ..\test\data\ch\
XCOPY /Y corech\*.* ..\test\data\corech\
XCOPY /Y mld\*.* ..\test\data\mld\
unit_tests\%Configuration%\library-tests.exe

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
