@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ECHO activating VS command prompt ...
SET PATH=C:\Program Files (x86)\MSBuild\14.0\Bin;%PATH%
CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

ECHO platform^: %platform%
:: HARDCODE "x64" as it is uppercase on AppVeyor and download from S3 is case sensitive
SET DEPSPKG=osrm-deps-win-x64-14.0.7z

:: local development
IF "%computername%"=="WIN-2G2NPH4S5B8" GOTO SKIPDL

IF EXIST %DEPSPKG% DEL %DEPSPKG%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET PROJECT_DIR=%CD%
ECHO PROJECT_DIR^: %PROJECT_DIR%
ECHO downloading %DEPSPKG%
powershell Invoke-WebRequest https://mapbox.s3.amazonaws.com/windows-builds/windows-build-deps/$env:DEPSPKG -OutFile $env:PROJECT_DIR\$env:DEPSPKG
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

7z -y x %DEPSPKG% | %windir%\system32\FIND "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:SKIPDL

IF EXIST build rd /s /q build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
mkdir build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
cd build
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET OSRMDEPSDIR=%PROJECT_DIR%\osrm-deps
set PREFIX=%OSRMDEPSDIR%/libs
set BOOST_ROOT=%OSRMDEPSDIR%/boost
set TBB_INSTALL_DIR=%OSRMDEPSDIR%/tbb
set TBB_ARCH_PLATFORM=intel64/vc14

ECHO calling cmake ....
cmake .. ^
-G "Visual Studio 14 Win64" ^
-DBOOST_ROOT=%BOOST_ROOT% ^
-DBoost_ADDITIONAL_VERSIONS=1.58 ^
-DBoost_USE_MULTITHREADED=ON ^
-DBoost_USE_STATIC_LIBS=ON ^
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
/toolsversion:14.0 ^
/p:PlatformToolset=v140 ^
/clp:Verbosity=normal ^
/nologo ^
/flp1:logfile=build_errors.txt;errorsonly ^
/flp2:logfile=build_warnings.txt;warningsonly
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD %PROJECT_DIR%\build\%Configuration%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET PATH=%PROJECT_DIR%\osrm-deps\libs\bin;%PATH%

ECHO running datastructure-tests.exe ...
datastructure-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO running algorithm-tests.exe ...
algorithm-tests.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF NOT "%APPVEYOR_REPO_BRANCH%"=="develop" GOTO DONE
ECHO ========= CREATING PACKAGES ==========

SET P=%PROJECT_DIR%
SET ZIP= %P%\osrm_%Configuration%.zip
IF EXIST %ZIP% ECHO deleting %ZIP% && DEL /F /Q %ZIP%
IF %ERRORLEVEL% NEQ 0 ECHO deleting %ZIP% FAILED && GOTO ERROR

7z a %ZIP% *.lib *.exe *.pdb %P%/osrm-deps/libs/bin/*.dll -tzip -mx9 | %windir%\system32\FIND "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CD ..\..\profiles
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO disk=c:\temp\stxxl,10000,wincall > .stxxl.txt
7z a %ZIP% * -tzip -mx9 | %windir%\system32\FIND "ing archive"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
