# vtzero

Tiny and fast vector tile decoder and encoder in C++.

Implements the [Mapbox Vector Tile Specification 2.x](https://www.mapbox.com/vector-tiles/specification).

[![Build Status](https://travis-ci.org/mapbox/vtzero.svg?branch=master)](https://travis-ci.org/mapbox/vtzero)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/mapbox/vtzero?svg=true)](https://ci.appveyor.com/project/Mapbox/vtzero)
[![Coverage Status](https://codecov.io/gh/mapbox/vtzero/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/vtzero)


## Depends

* C++11 compiler (GCC 4.8 or higher, clang 3.5 or higher, ...)
* CMake
* [Protozero](https://github.com/mapbox/protozero) version >= 1.6.0


## Build

First clone `protozero`:

```
git clone git@github.com:mapbox/protozero.git
```

Then clone `vtzero` beside `protozero`. The `vtzero` build system will, among
several places, look for `protozero` at `../protozero`. (If you would like to
use `protozero` from a different path you can set `PROTOZERO_INCLUDE_DIR` in
the CMake configuration step.)

Then, inside the `vtzero` directory do:

```
git submodule update --init
```

Finally, to build the examples and tests do:

```
mkdir build
cd build
cmake ..
make
```

Call `ctest` to run the tests.


## Examples

Several examples are provided to show usage of the library.

Call

    examples/vtzero-create

to create test tile named `test.mvt`.

Call

    examples/vtzero-show TILE-FILE

to show the contents of `TILE-FILE`.

You can use

    examples/vtzero-check TILE-FILE

to check vector tile for validity.


## Docs

Extensive documentation is available:
* [Tutorial](doc/tutorial.md) (start here)
* [Reading vector tiles](doc/reading.md)
* [Writing vector tiles](doc/writing.md)
* [Advanced vtzero topics](doc/advanced.md)

Make sure to read all of it before using vtzero. Vtzero isn't the simplest
library to use, because it always chooses performance over ease of use.

If [Doxygen](http://www.stack.nl/~dimitri/doxygen/) is installed on your
system, the build process will automatically create the API docs for you.
The results will be in your build directory under `doc/html`.


## Authors

Jochen Topf (jochen@topf.org),
Dane Springmeyer (dane@mapbox.com)

