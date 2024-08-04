# Libosmium

https://osmcode.org/libosmium

A fast and flexible C++ library for working with OpenStreetMap data.

Libosmium works on Linux, macOS and Windows.

[![Github Build Status](https://github.com/osmcode/libosmium/workflows/CI/badge.svg?branch=master)](https://github.com/osmcode/libosmium/actions)
[![Packaging status](https://repology.org/badge/tiny-repos/libosmium.svg)](https://repology.org/metapackage/libosmium)

Please see the [Libosmium manual](https://osmcode.org/libosmium/manual.html)
for more details than this README can provide.


## Prerequisites

You need a C++11 compiler and standard C++ library. Osmium needs at least GCC
4.8 or clang (LLVM) 3.4. (Some parts may work with older versions.)

Different parts of Libosmium (and the applications built on top of it) need
different libraries. You DO NOT NEED to install all of them, just install those
you need for your programs.

For details see the [list of
dependencies](https://osmcode.org/libosmium/manual.html#dependencies) in the
manual.

The following external (header-only) libraries are included in the libosmium
repository:
* [gdalcpp](https://github.com/joto/gdalcpp)

Note that [protozero](https://github.com/mapbox/protozero) was included in
earlier versions of libosmium, but isn't any more.


## Directories

* benchmarks: Some benchmarks checking different parts of Libosmium.

* cmake: CMake configuration scripts.

* doc: Config for API reference documentation.

* examples: Osmium example applications.

* include: C/C++ include files. All of Libosmium is in those header files
  which are needed for building Osmium applications.

* test: Tests (see below).


## Building

Osmium is a header-only library, so there is nothing to build for the
library itself.

But there are some tests and examples that can be build. Libosmium uses
cmake:

    mkdir build
    cd build
    cmake ..
    make

This will build the examples and tests. Call `ctest` to run the tests.

For more details see the
[Building Libosmium](https://osmcode.org/libosmium/manual.html#building-libosmium)
chapter in the manual.


## Testing

To download the `osm-testdata` submodule call:

```
git submodule update --init
```

This will enable additional tests.

See the
[Libosmium Manual](https://osmcode.org/libosmium/manual.html#running-tests)
for instructions.


## License

Libosmium is available under the Boost Software License. See LICENSE.


## Authors

Libosmium was mainly written and is maintained by Jochen Topf
(jochen@topf.org). See the git commit log for other authors.

