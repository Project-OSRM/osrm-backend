@ECHO OFF
SETLOCAL

SET PATH=c:\python27;%PATH%

ECHO activating VS command prompt
IF /I "%PLATFORM"=="x64" (
  CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
) ELSE (
  CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
)

IF NOT EXIST deps\gyp git clone --quiet --depth 1 https://chromium.googlesource.com/external/gyp.git deps/gyp

CALL deps\gyp\gyp.bat variant.gyp --depth=. ^
-f msvs ^
-G msvs_version=2015 ^
--generator-output=build

SET MSBUILD_PLATFORM=%platform%
IF /I "%MSBUILD_PLATFORM%" == "x86" SET MSBUILD_PLATFORM=Win32


msbuild ^
build\variant.sln ^
/nologo ^
/toolsversion:14.0 ^
/p:PlatformToolset=v140 ^
/p:Configuration=%configuration% ^
/p:Platform=%MSBUILD_PLATFORM%

build\"%configuration%"\tests.exe
