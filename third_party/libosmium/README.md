# Libosmium

http://osmcode.org/libosmium

A fast and flexible C++ library for working with OpenStreetMap data.

Libosmium works on Linux, Mac OSX and Windows.

[![Build Status](https://secure.travis-ci.org/osmcode/libosmium.svg)](https://travis-ci.org/osmcode/libosmium)
[![Build status](https://ci.appveyor.com/api/projects/status/github/osmcode/libosmium?svg=true)](https://ci.appveyor.com/project/Mapbox/libosmium)

Please see the [Libosmium manual](http://osmcode.org/libosmium/manual.html)
for more details than this README can provide.


## Prerequisites

Because Libosmium uses many C++11 features you need a modern compiler and
standard C++ library. Osmium needs at least GCC 4.8 or clang (LLVM) 3.4.
(Some parts may work with older versions.)

Different parts of Libosmium (and the applications built on top of it) need
different libraries. You DO NOT NEED to install all of them, just install those
you need for your programs.

For details see the [list of
dependencies](http://osmcode.org/libosmium/manual.html#dependencies) in the
manual.

The following external (header-only) libraries are included in the libosmium
repository:
* [gdalcpp](https://github.com/joto/gdalcpp)
* [protozero](https://github.com/mapbox/protozero)
* [utfcpp](http://utfcpp.sourceforge.net/)


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

For more detals see the
[Building Libosmium](http://osmcode.org/libosmium/manual.html#building-libosmium)
chapter in the manual.


## Testing

See the
[Libosmium Manual](http://osmcode.org/libosmium/manual.html#running-tests)
for instructions.


## Switching from the old Osmium

If you have been using the old version of Osmium at
https://github.com/joto/osmium you might want to read about the [changes
needed](http://osmcode.org/libosmium/manual.html#changes-from-old-versions-of-osmium).


## License

Libosmium is available under the Boost Software License. See LICENSE.txt.


## Authors

Libosmium was mainly written and is maintained by Jochen Topf
(jochen@topf.org). See the git commit log for other authors.

