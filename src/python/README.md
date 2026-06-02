# osrm-bindings

**Python bindings for [osrm-backend](https://github.com/Project-OSRM/osrm-backend) using [nanobind](https://github.com/wjakob/nanobind).**

## PyPI

```
pip install osrm-bindings
```

> [!NOTE]
> On PyPI we only distribute `abi3` wheels for each platform, i.e. one needs at least Python 3.12 to install the wheels. Of course it'll fall back to attempt an installation from source.

Platform | Arch
---|---
Linux | x86_64
MacOS | aarch64
MacOS | arm64
Windows | x86_64

## From Source

osrm-bindings requires **CPython 3.10+** and can be installed from source (e.g. from an sdist on PyPI) by running the following command in the repository root:

```
pip install .
```

The Python bindings are built alongside the OSRM C++ libraries. The version of the bindings matches the version of osrm-backend.

### System dependencies

A source/sdist build compiles the full OSRM C++ library, so the usual C++ toolchain (CMake ≥ 3.18, a C++20 compiler, Git) plus OSRM's native dependencies must be available. Also a development Python installation has to be in the PATH.

**Debian / Ubuntu**

```
sudo apt-get install -y \
    cmake g++ git pkg-config \
    libboost-all-dev libbz2-dev liblua5.4-dev \
    libtbb-dev libxml2-dev libzip-dev
```

**Fedora / RHEL / Rocky / AlmaLinux**

```
sudo dnf install -y \
    cmake gcc-c++ git pkgconf-pkg-config \
    boost-devel bzip2-devel lua-devel \
    tbb-devel libxml2-devel libzip-devel
```

**Alpine**

```
apk add --no-cache \
    cmake clang make git pkgconf \
    boost-dev bzip2-dev lua5.4-dev \
    onetbb-dev libxml2-dev libzip-dev expat-dev
```

**macOS (Homebrew)**

```
brew install cmake lua tbb boost@1.90
brew link boost@1.90
```

**Windows**

Windows uses [Conan](https://conan.io/) for OSRM's C++ dependencies. Install Conan 2.x and pass `ENABLE_CONAN=ON` to CMake:

```
pip install conan==2.27.0
conan profile detect --force
pip install . -C cmake.define.ENABLE_CONAN=ON
```

A full Visual Studio 2022 toolchain (or the Build Tools equivalent with the C++ workload) is required.

## Example

The following example will showcase the process of calculating routes between two coordinates.

First, import the `osrm` library, and instantiate an instance of OSRM:
```python
import osrm

# Instantiate osrm_py instance
osrm_py = osrm.OSRM("./test/data/ch/monaco.osrm")
```

Then, declare `RouteParameters`, and then pass it into the `osrm_py` instance:
```python
# Declare Route Parameters
route_params = osrm.RouteParameters(
    coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)]
)

# Pass it into the osrm_py instance
res = osrm_py.Route(route_params)

# Print out result output
print(res["waypoints"])
print(res["routes"])
```

## Type Stubs

The file `src/python/osrm/osrm_ext.pyi` contains auto-generated type stubs for the C++ extension module. These are used by IDEs for autocompletion and by documentation tools without compiling the extension.

After changing C++ bindings, rebuild the project to regenerate the stubs:

```
pip install -e .
```

Then commit the updated `.pyi` file.
