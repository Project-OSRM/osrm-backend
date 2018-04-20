@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

SET

ECHO activating VS cmd prompt && CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD ..

nuget install expat.v140 -Version 2.2.5
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET PATH=C:/projects/expat.v140.2.2.5/build/native/bin/x64/%config%;%PATH%

nuget install zlib-vc140-static-64 -Version 1.2.11
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

nuget install bzip2.v140 -Version 1.0.6.9
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET PATH=C:/projects/bzip2.v140.1.0.6.9/build/native/bin/x64/%config%;%PATH%

CD libosmium

ECHO config^: %config%

SET libpostfix=""
IF "%config%"=="Debug" SET libpostfix="d"

IF EXIST build ECHO deleting build dir... && RD /Q /S build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD build

::This will produce lots of LNK4099 warnings which can be ignored.
::Unfortunately they can't be disabled, see
::https://stackoverflow.com/questions/661606/visual-c-how-to-disable-specific-linker-warnings
SET CMAKE_CMD=cmake .. -LA -G "Visual Studio 14 Win64" ^
-DOsmium_DEBUG=TRUE ^
-DCMAKE_BUILD_TYPE=%config% ^
-DBUILD_HEADERS=OFF ^
-DBOOST_ROOT=C:/Libraries/boost_1_63_0 ^
-DZLIB_INCLUDE_DIR=C:/projects/zlib-vc140-static-64.1.2.11/lib/native/include ^
-DZLIB_LIBRARY=C:/projects/zlib-vc140-static-64.1.2.11/lib/native/libs/x64/static/%config%/zlibstatic.lib ^
-DEXPAT_INCLUDE_DIR=C:/projects/expat.v140.2.2.5/build/native/include ^
-DEXPAT_LIBRARY=C:/projects/expat.v140.2.2.5/build/native/lib/x64/%config%/libexpat%libpostfix%.lib ^
-DBZIP2_INCLUDE_DIR=C:/projects/bzip2.v140.1.0.6.9/build/native/include ^
-DBZIP2_LIBRARIES=C:/projects/bzip2.v140.1.0.6.9/build/native/lib/x64/%config%/libbz2%libpostfix%.lib

ECHO calling^: %CMAKE_CMD%
%CMAKE_CMD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET avlogger=
IF /I "%APPVEYOR%"=="True" SET avlogger=/logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

msbuild libosmium.sln ^
/p:Configuration=%config% ^
/toolsversion:14.0 ^
/p:Platform=x64 ^
/p:PlatformToolset=v140 %avlogger%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ctest --output-on-failure -C %config% -E testdata-overview
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%
