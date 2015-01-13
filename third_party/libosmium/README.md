# Osmium Library

http://osmcode.org/libosmium

A fast and flexible C++ library for working with OpenStreetMap data.

NOTE: This is a beta version of the next-generation Osmium. For production
use, see the Osmium version at https://github.com/joto/osmium .

There are a few applications that use the Osmium library in the examples
directory. See the [osmium-contrib](http://github.com/osmcode/osmium-contrib)
repository for more example code.

[![Build Status](https://secure.travis-ci.org/osmcode/libosmium.png)](http://travis-ci.org/osmcode/libosmium)
[![Build status](https://ci.appveyor.com/api/projects/status/mkbg6e6stdgq7c1b?svg=true)](https://ci.appveyor.com/project/Mapbox/libosmium)

Libosmium is developed on Linux, but also works on OSX and Windows (with some
limitations).

## Prerequisites

Because Osmium uses many C++11 features you need a modern compiler and standard
C++ library. Osmium needs at least GCC 4.8 or clang (LLVM) 3.2. (Some parts may
work with older versions.)

Different parts of Osmium (and the applications built on top of it) need
different libraries. You DO NOT NEED to install all of them, just install those
you need for the programs you need.

    boost-iterator, boost-regex
        http://www.boost.org/
        Debian/Ubuntu: libboost-dev
        openSUSE: boost-devel
        Homebrew: boost

    boost-program-options (for parsing command line options in some examples)
        http://www.boost.org/doc/libs/1_54_0/doc/html/program_options.html
        Debian/Ubuntu: libboost-program-options-dev

    Google protocol buffers (for PBF support)
        http://code.google.com/p/protobuf/ (at least version 2.3.0 needed)
        Debian/Ubuntu: libprotobuf-dev protobuf-compiler
        openSUSE: protobuf-devel
        Homebrew: protobuf
        Also see http://wiki.openstreetmap.org/wiki/PBF_Format

    OSMPBF (for PBF support)
        https://github.com/scrosby/OSM-binary
        Debian/Ubuntu: libosmpbf-dev
        (The package in Ubuntu 14.04 and older is too old, install from source
        in these cases.)
        Homebrew: osm-pbf

    Expat (for parsing XML files)
        http://expat.sourceforge.net/
        Debian/Ubuntu: libexpat1-dev
        openSUSE: libexpat-devel
        Homebrew: expat

    zlib (for PBF and for gzip support when reading/writing XML)
        http://www.zlib.net/
        Debian/Ubuntu: zlib1g-dev
        openSUSE: zlib-devel

    bz2lib (for bzip2 support when reading/writing XML)
        http://www.bzip.org/
        Debian/Ubuntu: libbz2-dev

    Google sparsehash
        http://code.google.com/p/google-sparsehash/
        Debian/Ubuntu: libsparsehash-dev
        openSUSE: sparsehash
        Homebrew: google-sparsehash

    GDAL (for OGR support)
        http://gdal.org/
        Debian/Ubuntu: libgdal1-dev
        openSUSE: libgdal-devel
        Homebrew: gdal

    GEOS (for GEOS support)
        http://trac.osgeo.org/geos/
        Debian/Ubuntu: libgeos++-dev
        openSUSE: libgeos-devel
        Homebrew: geos

    libproj (for projection support)
        http://trac.osgeo.org/proj/
        Debian/Ubuntu: libproj-dev

    Doxygen (to build API documentation) and tools
        http://www.stack.nl/~dimitri/doxygen/
        Debian/Ubuntu: doxygen graphviz xmlstarlet
        Homebrew: doxygen

You need to either install the packages for your distribution or install those
libraries from source. Most libraries should be available in all distributions.


## Directories

* include: C/C++ include files. All of Osmium is in those header files which
  are needed for building Osmium applications.

* examples: Osmium example applications.

* test: Tests (see below).

* doc: Config for documentation.


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

To build the documentation you need Doxygen. If cmake can find it it will
enable the `doc` target so you can build the documentation like this:

    make doc

If the 'cppcheck' binary is found, cmake will add another target to the
Makfile which allows you to call cppheck on all `*.cpp` and `*.hpp` files:

    make cppcheck

For Mac users: If you have clang 3.2 or newer, use the system compiler.
If not you have to build the compiler yourself. See the instructions
on http://clang.llvm.org/ .

Preliminary support for cmake is provided. You can use this instead of "make":


## Testing

### Unit Tests

There are a few unit tests using the Catch unit test framework in the "test"
directory. Many more tests are needed, any help appreciated.

For [Catch](https://github.com/philsquared/Catch/) only one header file is
needed which is included (`test/include/catch.hpp`).

To compile these unit tests make sure `BUILD_UNIT_TESTS` is set in the cmake
config, then build the project and call

    ctest

You can run tests matching a pattern by calling

    ctest -R test_name

for instance:

    ctest basic_test_node

will run the `basic_test_node` and `basic_test_node_ref` tests.

### Data Tests

In addition there are some test based on the OSM Test Data Repository at
http://osmcode.org/osm-testdata/ . Make sure `BUILD_DATA_TESTS` is set in the
cmake config, then build the project and call `ctest`.

Some of these tests need Ruby and then 'json' gem installed.

### Valgrind

Running tests with valgrind:

    ctest -D ExperimentalMemCheck


## Osmium on 32bit Machines

Osmium works well on 64 bit machines, but on 32 bit machines there are some
problems. Be aware that not everything will work on 32 bit architectures.
This is mostly due to the 64 bit needed for node IDs. Also Osmium hasn't been
tested well on 32 bit systems. Here are some issues you might run into:

* Google Sparsehash does not work on 32 bit machines in our use case.
* The `mmap` system call is called with a `size_t` argument, so it can't
  give you more than 4GByte of memory on 32 bit systems. This might be a
  problem.

Please report any issues you have and we might be able to solve them.


## Switching from the old Osmium

See `README-changes-from-old-osmium`.


## License

The Osmium Library is available under the Boost Software License. See
LICENSE.txt.


## Authors

The Osmium Library was mainly written and is maintained by Jochen Topf
(jochen@topf.org).

Other authors:
* Peter Körner (github@mazdermind.de) (PBF writer, ...)

