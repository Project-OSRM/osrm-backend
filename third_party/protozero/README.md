# protozero

Minimalistic protocol buffer decoder and encoder in C++.

Designed for high performance. Suitable for writing zero copy parsers and
encoders with minimal need for run-time allocation of memory.

Low-level: this is designed to be a building block for writing a very
customized decoder for a stable protobuf schema. If your protobuf schema is
changing frequently or lazy decoding is not critical for your application then
this approach offers no value: just use the C++ API that can be generated with
the Google Protobufs `protoc` program.

[![Github Build Status](https://github.com/mapbox/protozero/actions/workflows/ci.yml/badge.svg)](https://github.com/mapbox/protozero/actions/workflows/ci.yml)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/mapbox/protozero?svg=true)](https://ci.appveyor.com/project/Mapbox/protozero)
[![Packaging status](https://repology.org/badge/tiny-repos/protozero.svg)](https://repology.org/metapackage/protozero)

## Depends

* C++14 compiler
* CMake
* Some tests depend on the Google Protobuf library, but use of Protozero
  doesn't need it


## How it works

The protozero code does **not** read `.proto` files used by the usual Protobuf
implementations. The developer using protozero has to manually "translate" the
`.proto` description into code. This means there is no way to access any of the
information from the `.proto` description. This results in a few restrictions:

* The names of the fields are not available.
* Enum names are not available, you'll have to use the values they are defined
  with.
* Default values are not available.
* Field types have to be hardcoded. The library does not know which types to
  expect, so the user of the library has to supply the right types. Some checks
  are made using `assert()`, but mostly the user has to take care of that.

The library will make sure not to overrun the buffer it was given, but
basically all other checks have to be made in user code!


## Documentation

You have to have a working knowledge of how
[protocol buffer encoding works](https://developers.google.com/protocol-buffers/docs/encoding).

* Read the [tutorial](doc/tutorial.md) for an introduction on how to use
  Protozero.
* Some advanced topics are described in an [extra document](doc/advanced.md).
* There is a table of all types and functions in the
  [cheat sheet](doc/cheatsheet.md).
* Read the [upgrading instructions](UPGRADING.md) if you are upgrading from
  an older version of Protozero.

The build process will also build the Doxygen-based reference documentation if
you have Doxygen installed. Then open `doc/html/index.html` in your browser to
read it.


## Endianness

Protozero uses a very simplistic test to check the byte order of the system it
compiles on. If this check is wrong, you'll get test failures. If this is the
case, please [open an issue](https://github.com/mapbox/protozero/issues) and
tell us about your system.


## Building tests

Extensive tests are included. Build them using CMake:

    mkdir build
    cd build
    cmake ..
    make

Call `ctest` to run the tests.

The unit and reader tests are always build, the writer tests are only build if
the Google Protobuf library is found when running CMake.

See `test/README.md` for more details about the test.


## Coverage report

To get a coverage report set `CXXFLAGS` and `LDFLAGS` before calling CMake:

    CXXFLAGS="--coverage" LDFLAGS="--coverage" cmake ..

Then call `make` as usual and run the tests using `ctest`.

If you are using `g++` use `gcov` to generate a report (results are in `*.gcov`
files):

    gcov -lp $(find test/ -name '*.o')

If you are using `clang++` use `llvm-cov` instead:

    llvm-cov gcov -lp $(find test/ -name '*.o')

If you are using `g++` you can use `gcovr` to generate nice HTML output:

    mkdir -p coverage
    gcovr . -r SRCDIR --html --html-details -o coverage/index.html

Open `coverage/index.html` in your browser to see the report.


## Clang-tidy

After the CMake step, run

    make clang-tidy

to check the code with [clang-tidy](https://clang.llvm.org/extra/clang-tidy/).
You might have to set `CLANG_TIDY` in CMake config.


## Cppcheck

For extra checks with [Cppcheck](https://cppcheck.sourceforge.io/) you can,
after the CMake step, call

    make cppcheck


## Installation

After the CMake step, call `make install` to install the include files in
`/usr/local/include/protozero`.

If you are using CMake to build your own software, you can copy the file
`cmake/FindProtozero.cmake` and use it in your build. See the file for
details.


## Who is using Protozero?

* [Carmen](https://github.com/mapbox/carmen-cache)
* [Libosmium](https://github.com/osmcode/libosmium)
* [Mapbox GL Native](https://github.com/mapbox/mapbox-gl-native)
* [Mapbox Vector Tile library](https://github.com/mapbox/vector-tile)
* [Mapnik](https://github.com/mapbox/mapnik-vector-tile)
* [OSRM](https://github.com/Project-OSRM/osrm-backend)
* [Tippecanoe](https://github.com/mapbox/tippecanoe)
* [Vtzero](https://github.com/mapbox/vtzero)

Are you using Protozero? Tell us! Send a pull request with changes to this
README.


