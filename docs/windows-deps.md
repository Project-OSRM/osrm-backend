# Building OSRM for Windows

There is experimental support for building OSRM on Windows.

## Dependencies

You will need a modern Windows development stack (e.g. Visual Studio 17). The published binaries are built with
[Windows Server 2025](https://github.com/actions/runner-images/blob/main/images/win/Windows2025-Readme.md) Github hosted runners.

Dependencies are managed via [vcpkg](https://vcpkg.io/) in manifest mode
(see `vcpkg.json` at the repo root). The baseline commit is pinned in
`vcpkg-configuration.json`.

## Prerequisites

1. Install Visual Studio 2022 with the "Desktop development with C++" workload.
2. Clone vcpkg and bootstrap it:

```bat
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
set VCPKG_ROOT=C:\vcpkg
```

## Building

From a `x64 Native Tools Command Prompt for VS 2022` at the repo root:

```bat
cmake --preset ci-windows -DENABLE_NODE_BINDINGS=ON
cmake --build --preset ci-windows
```

The first configure will build every dependency from source, which takes a
while. Subsequent configures reuse vcpkg's binary cache.





