# Building OSRM for Windows

## Dependencies

Get a decent Windows with decent Visual Studio (14 at least for C++11 support). The published binaries are build with
VS2019 and Windows SDK8.1.

In case you are using [prepacked Windows VM with VS2019](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/), you
have to install [Windows SDK 8.1](https://go.microsoft.com/fwlink/p/?LinkId=323507)

Prepare directories for dependencies, build and target file location.Target directory ($target starting from that moment) should have /include and /lib subdirectories.

### Bzip2

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/bzip2-1.0.8.tar.gz
    * https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz

2. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
3. Issue `nmake /f makefile.msc`
4. Copy bzlib.h to $target\include and libbz2.lib to $target\lib

### ZLib

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/zlib-1.2.11.tar.gz
    * https://www.zlib.net/zlib-1.2.11.tar.gz

2. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
3. Switch to `contrib\vstudio\vc14`
4. If needed, open `zlibvc.sln` with Visual Studio and retarget to your version of compiler and SDK.
5. Issue `msbuild zlibvc.sln /p:BuildInParallel=true /p:Configuration=Release /p:Platform=x64 /m:<Number of cpu cores>`
6. Copy x64\ZlibStatRelease\zlibstat.lib to $target\lib\libz.lib, copy zlib.h and zconf.h to $target\include

### ICU

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://wolt-project.wolt.com/deps/icu4c-66_1-src.zip
    * https://github.com/unicode-org/icu/releases/download/release-66-1/icu4c-66_1-src.zip
    * https://wolt-project.wolt.com/deps/icu4c-66_1-data.zip
    * https://github.com/unicode-org/icu/releases/download/release-66-1/icu4c-66_1-data.zip
2. Do retarget if neededby openinig .\source\allinone\allinone.sln and editing projects
3. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
4. Run build:
    msbuild .\source\allinone\allinone.sln /nologo /p:BuildInParallel=true /p:Configuration=Release /p:Platform=x64 /m:<Number of cpu cores>
5. Copy lib64\*.lib to $target\lib, copy include contents to $target\include
6. Copy bin64\*dll to any dir withing your $PATH. At the same time copy them to $target\lib

### Boost

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/boost_1_73_0.zip
    * https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.zip

2. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
3. Build b2:
    bootstrap.bat --with-toolset=msvc-14.2
4. Build boost:
       b2 -a -d release state --build-type=minimal toolset=msvc-14.2 -q runtime-link=shared link=static address-model=64 --with-iostreams --with-test --with-thread --with-filesystem --with-date_time --with-system --with-program_options --with-regex --disable-filesystem2 -sHAVE_ICU=1 include=<target>\include library-path=<target>\lib -sZLIB_SOURCE=<builddir>/zlib -zBZIP2_BINARY=libbz2 -sBZIP2_INCLUDE=<target>\include -sBZIP2_LIBPATH=<target>\lib -sICU_ICUUC_NAME=icuuc -sICU_ICUDT_NAME=icudt -sICU_ICUIN_NAME=icuin -sBUILD=boost_unit_test_framework -j<number of cpu cores>
5. Copy `boost` subdirectory to <target>\include and contents of `stage` to <target>\lib

### Expat

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/libexpat-2_2_9.zip
    * https://github.com/libexpat/libexpat/archive/R_2_2_9.zip
2. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
3. Configure build my calling cmake:
    mkdir expat\build
    cd expat\build
    cmake -G"Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -DEXPAT_MSVC_STATIC_CRT=ON -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TESTS=OFF -DEXPAT_SHARED_LIBS=OFF ..
4. Build expat: `msbuild expat.sln /nologo /p:Configuration=Release /p:Platform=x64`
5. Copy `Release\libexpat.*` to <target>/lib. Copy `expat/lib/expat.h` and `expat/lib/expat_external.h` to <target>/include

### LUA

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/lua-5.3.5.tar.gz
    * https://www.lua.org/ftp/lua-5.3.5.tar.gz
2. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
3. Lua doesn't have native MSVC support, so you have to compile it by hand:
    cd src
    cl /MD /O2 /c /DLUA_COMPAT_5_2 *.c
    ren lua.obj lua.o
    ren luac.obj luac.o
    link /LIB /OUT:lua5.3.5.dll *.obj
4. Copy `lua5.3.5.lib` to <target>/lib. Copy `lua.h`,`lauxlib,h`,`lua.hpp`,`lualib.h`,`luaconf.h` to <target>/include

### TBB

1. Download either from Wolt OSRM mirror or original distribution and unpack.
    * https://project-osrm.wolt.com/deps/oneTBB-v2020.2.zip
    * https://github.com/oneapi-src/oneTBB/archive/v2020.2.zip
2. Retarget by opening build\vs2013\makefile.sln
3. Start 'x64 Native Tools Command Prompt for VS2019' and change directory to unpacked source tree.
4. Switch to build\vs2013 and build: `msbuild makefle.sln /nologo /p:Configuration=Release /p:Platform=x64`
5. Copy x64/Release/*.{dll,lib} files to <target>/lib and copy contents of include directory to <target>/include
