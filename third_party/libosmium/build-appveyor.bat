@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

SET CUSTOM_CMAKE=cmake-3.6.2-win64-x64
::show all available env vars
SET
ECHO cmake on AppVeyor
cmake -version

ECHO activating VS cmd prompt && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET lodir=%CD%
SET PATH=%lodir%\%CUSTOM_CMAKE%\bin;%PATH%
SET LODEPSDIR=%lodir%\libosmium-deps
SET PROJ_LIB=%LODEPSDIR%\proj\share
SET GDAL_DATA=%LODEPSDIR%\gdal\data
::gdal.dll
SET PATH=%LODEPSDIR%\gdal\lib;%PATH%
::geos.dll
SET PATH=%LODEPSDIR%\geos\lib;%PATH%
::libtiff.dll
SET PATH=%LODEPSDIR%\libtiff\lib;%PATH%
::jpeg.dll
SET PATH=%LODEPSDIR%\jpeg\lib;%PATH%
::libexpat.dll
SET PATH=%LODEPSDIR%\expat\lib;%PATH%
::zlibwapi.dll
SET PATH=%LODEPSDIR%\zlib\lib;%PATH%
::convert backslashes in bzip2 path to forward slashes
::cmake cannot find it otherwise
SET LIBBZIP2=%LODEPSDIR%\bzip2\lib\libbz2.lib
SET LIBBZIP2=%LIBBZIP2:\=/%

IF NOT EXIST cm.7z ECHO downloading cmake %CUSTOM_CMAKE% ... && powershell Invoke-WebRequest https://mapbox.s3.amazonaws.com/windows-builds/windows-build-deps/%CUSTOM_CMAKE%.7z -OutFile cm.7z
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF NOT EXIST lodeps.7z ECHO downloading binary dependencies... && powershell Invoke-WebRequest https://mapbox.s3.amazonaws.com/windows-builds/windows-build-deps/libosmium-deps-win-14.0-x64.7z -OutFile lodeps.7z
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF NOT EXIST %CUSTOM_CMAKE% ECHO extracting cmake... && 7z x cm.7z | %windir%\system32\find "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF NOT EXIST %LODEPSDIR% ECHO extracting binary dependencies... && 7z x lodeps.7z | %windir%\system32\find "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO %LODEPSDIR%
DIR %LODEPSDIR%
::TREE %LODEPSDIR%

::powershell (Get-ChildItem $env:LODEPSDIR\boost\lib -Filter *boost*.dll)[0].BaseName.split('_')[-1]
FOR /F "tokens=1 usebackq" %%i in (`powershell ^(Get-ChildItem %LODEPSDIR%\boost\lib -Filter *boost*.dll^)[0].BaseName.split^('_'^)[-1]`) DO SET BOOST_VERSION=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO BOOST_VERSION^: %BOOST_VERSION%

ECHO our own cmake
cmake -version

CD %lodir%\..

IF NOT EXIST osm-testdata ECHO cloning osm-testdata && git clone --depth 1 https://github.com/osmcode/osm-testdata.git
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
CD osm-testdata
git fetch
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
git pull
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD %lodir%
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
-LA -G "Visual Studio 14 Win64" ^
-DOsmium_DEBUG=TRUE ^
-DCMAKE_BUILD_TYPE=%config% ^
-DBUILD_HEADERS=OFF ^
-DBOOST_ROOT=%LODEPSDIR%\boost ^
-DZLIB_LIBRARY=%LODEPSDIR%\zlib\lib\zlibwapi.lib ^
-DBZIP2_LIBRARY_RELEASE=%LIBBZIP2% ^
-DCMAKE_PREFIX_PATH=%LODEPSDIR%\zlib;%LODEPSDIR%\expat;%LODEPSDIR%\bzip2;%LODEPSDIR%\geos;%LODEPSDIR%\gdal;%LODEPSDIR%\proj;%LODEPSDIR%\sparsehash;%LODEPSDIR%\wingetopt

ECHO calling^: %CMAKE_CMD%
%CMAKE_CMD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

msbuild libosmium.sln ^
/p:Configuration=%config% ^
/toolsversion:14.0 ^
/p:Platform=x64 ^
/p:PlatformToolset=v140
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ctest --output-on-failure ^
-C %config% ^
-E testdata-overview
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


GOTO DONE

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%
