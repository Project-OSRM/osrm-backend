@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~ %~f0 ~~~~~~

echo "Adding MSYS2 to path..."
SET "PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%"
echo %PATH%

echo "Installing MSYS2 packages..."
bash -lc "pacman -Sy --needed --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-geos mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-cppcheck mingw-w64-x86_64-doxygen mingw-w64-x86_64-gdb mingw-w64-x86_64-gdal mingw-w64-x86_64-ruby mingw-w64-x86_64-libspatialite mingw-w64-x86_64-spatialite-tools mingw-w64-x86_64-clang-tools-extra"
bash -lc "pacman -Sy --needed --noconfirm mingw-w64-x86_64-postgresql mingw-w64-x86_64-netcdf mingw-w64-x86_64-crypto++"
call C:\msys64\mingw64\bin\gem.cmd install json

REM Workaround for problem with spatialite (see https://github.com/osmcode/libosmium/issues/262)
copy /y C:\msys64\mingw64\bin\libreadline8.dll C:\msys64\mingw64\bin\libreadline7.dll

echo "Setting PROJ_LIB/DATA variable for correct PROJ.4 working"
set PROJ_LIB=c:\msys64\mingw64\share\proj
set PROJ_DATA=c:\msys64\mingw64\share\proj

set CXXFLAGS=-Wno-stringop-overflow

echo "Generating makefiles"
mkdir build
cd build
cmake .. -G "MSYS Makefiles" -DBUILD_DATA_TESTS=ON -DBUILD_HEADERS=OFF
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

echo "Building"
make
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

echo "Testing"
ctest --output-on-failure
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:ERROR
ECHO ~~~~~~ ERROR %~f0 ~~~~~~
SET EL=%ERRORLEVEL%

:DONE
IF %EL% NEQ 0 ECHO. && ECHO !!! ERRORLEVEL^: %EL% !!! && ECHO.
ECHO ~~~~~~ DONE %~f0 ~~~~~~

EXIT /b %EL%
