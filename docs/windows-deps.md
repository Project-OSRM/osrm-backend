# Building OSRM for Windows

There is experimental support for building OSRM on Windows.

## Dependencies

You will need a modern Windows development stack (e.g. Visual Studio 17). The published binaries are built with
[Windows Server 2022](https://github.com/actions/runner-images/blob/main/images/win/Windows2022-Readme.md) Github hosted runners.

Dependencies are managed via [Conan](https://conan.io/) and built with [CMake](https://cmake.org/).

## Building

```bat
cmake -DENABLE_CONAN=ON -DENABLE_NODE_BINDINGS=ON -DCMAKE_BUILD_TYPE=%CONFIGURATION% -G "Visual Studio 17 2022" ..

msbuild OSRM.sln ^
/p:Configuration=%CONFIGURATION% ^
/p:Platform=x64 ^
/t:rebuild ^
/p:BuildInParallel=true ^
/m:%NUMBER_OF_PROCESSORS% ^
/toolsversion:Current ^
/clp:Verbosity=normal ^
/nologo
```





