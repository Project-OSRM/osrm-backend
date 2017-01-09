
## 0.4.0

 - Now defaulting to clang 3.9.1 for building binaries
 - clang++ binaries no longer distribute libc++ headers on OS X (instead they depend on system)
 - Reduced size of v8 libs by striping debug symbols
 - Fixed minjur latest package to build in Release mode
 - Added polylabel 1.0.2, protozero 1.4.5, rocksdb v4.13, libosmium 2.10.3, llvm 3.9.1,
   boost 1.63.0, cmake 3.7.1, cairo 1.14.6, freetype 2.6.5, harfbuzz 1.3.0, jpeg_turbo 1.5.0,
   libpng 1.6.24, pixman 0.34.0, sqlite 3.14.1, twemproxy 0.4.1,
 - Removed llvm 3.8.0, clang 3.8.0, llvm 3.9.0, luabind, luajit
 - Rebuilt libpq 9.5.2, libtiff 4.0.6, utfcpp 2.3.4, minjur latest

Changes: https://github.com/mapbox/mason/compare/v0.3.0...v0.4.0

## 0.3.0

 - Updated android compile flags
 - Added v8 `5.1.281.47` and `3.14.5.10`
 - Fixed boost library name reporting
 - Added tippecanoe `1.15.1`
 - Added `iwyu` and `asan_symbolize` python script to llvm+clang++ packages

Changes: https://github.com/mapbox/mason/compare/v0.2.0...v0.3.0

## 0.2.0

 - Added icu 58.1, mesa egl, boost 1.62.0, gdb 7.12, Android NDK r13b, binutils latest,
   variant 1.1.4, geometry 0.9.0, geojson 0.4.0, pkgconfig 0.29.1, wagyu 1.0
 - Removed boost *all* packages
 - Renamed `TRAVIS_TOKEN` to `MASON_TRAVIS_TOKEN`
 - Now including llvm-ar and llvm-ranlib in clang++ package
 - Now setting secure variables in mason rather than .travis.yml per package

Changes: https://github.com/mapbox/mason/compare/v0.1.1...v0.2.0

## 0.1.1

 - Added binutils 2.27, expat 2.2.0, mesa 13.0.0, and llvm 4.0.0 (in-development)
 - Improved mason.cmake to support packages that report multiple static libs
 - Improved llvm >= 3.8.1 packages to support `-flto` on linux

## 0.1.0
 - First versioned release