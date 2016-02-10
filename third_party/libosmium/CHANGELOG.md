
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


## [2.6.0] - 2016-02-04

### Added

- The new handler osmium::handler::CheckOrder can be used to check that a
  file is properly ordered.
- Add new method to build OSM nodes, ways, relations, changesets, and areas
  in buffers that wraps the older Builder classes. The new code is much easier
  to use and very flexible. There is no documentation yet, but the tests in
  `test/t/builder/test_attr.cpp` can give you an idea how it works.
- Add util class to get memory usage of current process on Linux.

### Changed

- New Buffer memory management speeds up Buffer use, because it doesn't clear
  the memory unnecessarily.

### Fixed

- osmium::Box::extend() function now ignores invalid locations.
- Install of external library headers.
- Check way has at least one node before calling `is_closed()` in area
  assembler.
- Declaration/definition of some friend functions was in the wrong namespace.


## [2.5.4] - 2015-12-03

### Changed

- Included gdalcpp.hpp header was updated to version 1.1.1.
- Included protozero library was updated to version 1.2.3.
- Workarounds for missing constexpr support in Visual Studio removed. All
  constexpr features we need are supported now.
- Some code cleanup after running clang-tidy on the code.
- Re-added `Buffer::value_type` typedef. Turns out it is needed when using
  `std::back_inserter` on the Buffer.

### Fixed

- Bugs with Timestamp code on 32 bit platforms. This necessitated
  some changes in Timestamp which might lead to changes in user
  code.
- Bug in segment intersection code (which appeared on i686 platform).


## [2.5.3] - 2015-11-17

### Added

- `osmium::make_diff_iterator()` helper function.

### Changed

- Deprecated `osmium::Buffer::set_full_callback()`.
- Removed DataFile class which was never used anywhere.
- Removed unused and obscure `Buffer::value_type` typedef.

### Fixed

- Possible overrun in Buffer when using the full-callback.
- Incorrect swapping of Buffer.


## [2.5.2] - 2015-11-06

# Fixed

- Writing data through an OutputIterator was extremly slow due to
  lock contention.


## [2.5.1] - 2015-11-05

### Added

- Header `osmium/fwd.hpp` with forward declarations of the most commonly
  used Osmium classes.

### Changed

- Moved `osmium/io/overwrite.hpp` to `osmium/io/writer_options.hpp`
  If you still include the old file, you'll get a warning.


## [2.5.0] - 2015-11-04

### Added

- Helper functions to make input iterator ranges and output iterators.
- Add support for reading o5m and o5c files.
- Option for osmium::io::Writer to fsync file after writing.
- Lots of internal asserts() and other robustness checks.

### Changed

- Updated included protozero library to version 1.2.0.
- Complete overhaul of the I/O system making it much more robust against
  wrong data and failures during I/O operations.
- Speed up PBF writing by running parts of it in parallel.
- OutputIterator doesn't hold an internal buffer any more, but it uses
  one in Writer. Calling flush() on the OutputIterator isn't needed any
  more.
- Reader now throws when trying to read after eof or an error.
- I/O functions that used to throw std::runtime_error now throw
  osmium::io_error or derived.
- Optional parameters on osmium::io::Writer now work in any order.

### Fixed

- PBF reader now decodes locations of invisible nodes properly.
- Invalid Delta encode iterator dereference.
- Lots of includes fixed to include (only) what's used.
- Dangling reference in area assembly code.


## [2.4.1] - 2015-08-29

### Fixed

- CRC calculation of tags and changesets.


## [2.4.0] - 2015-08-29

### Added

- Checks that user names, member roles and tag keys and values are not longer
  than 256 * 4 bytes. That is the maximum length 256 Unicode characters
  can have in UTF-8 encoding.
- Support for GDAL 2. GDAL 1 still works.

### Changed

- Improved CMake build scripts.
- Updated internal version of Protozero to 1.1.0.
- Removed `toogr*` examples. They are in their own repository now.
  See https://github.com/osmcode/osm-gis-export.
- Files about to be memory-mapped (for instance index files) are now set
  to binary mode on Windows so the application doesn't have to do this.

### Fixed

- Hanging program when trying to open file with an unknown file format.
- Building problems with old boost versions.
- Initialization errors in PBF writer.
- Bug in byte swap code.
- Output on Windows now always uses binary mode, even when writing to
  stdout, so OSM xml and opl files always use LF line endings.


## [2.3.0] - 2015-08-18

### Added

- Allow instantiating osmium::geom::GEOSFactory with existing GEOS factory.
- Low-level functions to support generating a architecture- and endian-
  independant CRC from OSM data. This is intended to be uses with boost::crc.
- Add new debug output format. This format is not intended to be read
  automatically, but for human consumption. It formats the data nicely.
- Make writing of metadata configurable for XML and OPL output (use
  `add_metadata=false` as file option).

### Changed

- Changed `add_user()` and `add_role()` in builders to use string length
  without the 0-termination.
- Improved code setting file format from suffix/format argument.
- Memory mapping utility class now supports readonly, private writable or
  shared writable operation.
- Allow empty version (0) in PBF files.
- Use utf8cpp header-only lib instead of boost for utf8 decoding. The library
  is included in the libosmium distribution.
- New PBF reader and writer based on the protozero. A complete rewrite of the
  code for reading and writing OSM PBF files. It doesn't use the Google
  protobuf library and it doesn't use the OSMPBF/OSM-Binary library any more.
  Instead is uses the protozero lightweight protobuf header library which is
  included in the code. Not only does the new code have less dependencies, it
  is faster and more robust. https://github.com/mapbox/protozero

### Fixed

- Various smaller bug fixes.
- Add encoding for relation member roles in OPL format.
- Change character encoding to new format in OPL: variable length hex code
  between % characters instead of a % followed by 4-digit hex code. This is
  necessary because unicode characters can be longer than the 4-digit hex
  code.
- XML writer: The linefeed, carriage return, and tab characters are now
  escaped properly.
- Reading large XML files could block.

## [2.2.0] - 2015-07-04

### Added

- Conversion functions for some low-level types.
- BoolVector index class.
- `min_op`/`max_op` utility functions.
- More tests here and there.
- Helper methods `is_between()` and `is_visible_at()` to DiffObject.
- GeoJSON factory using the RapidJSON library.
- Support for tile calculations.
- Create simple polygons from ways in geom factories.
- `MemoryMapping` and `TypedMemoryMapping` helper classes.
- `close()` function to `mmap_vector_base` class.
- Function on `Buffer` class to get iterator to specific offset.
- Explicit cast operator from `osmium::Timestamp` to `uint32_t`.

### Changed

- Throw exception on illegal values in functions parsing strings to get ids,
  versions, etc.
- Improved error message for geometry exceptions.

### Fixed

- Throw exception from `dump_as_array()` and `dump_as_list()` functions if not
  implemented in an index.
- After writing OSM files, program could stall up to a second.
- Dense location store was written out only partially.
- Use `uint64_t` as counter in benchmarks, so there can be no overflows.
- Example programs now read packed XML files, too.
- Refactoring of memory mapping code. Removes leak on Windows.
- Better check for invalid locations.
- Mark `cbegin()` and `cend()` of `mmap_vector_base` as const functions.

## [2.1.0] - 2015-03-31

### Added

- When writing PBF files, sorting the PBF stringtables is now optional.
- More tests and documentation.

### Changed

- Some functions are now declared `noexcept`.
- XML parser fails now if the top-level element is not `osm` or `osmChange`.

### Fixed

- Race condition in PBF reader.
- Multipolygon collector was accessing non-existent NodeRef.
- Doxygen documentation wan't showing all classes/functions due to a bug in
  Doxygen (up to version 1.8.8). This version contains a workaround to fix
  this.

[unreleased]: https://github.com/osmcode/libosmium/compare/v2.6.0...HEAD
[2.6.0]: https://github.com/osmcode/libosmium/compare/v2.5.4...v2.6.0
[2.5.4]: https://github.com/osmcode/libosmium/compare/v2.5.3...v2.5.4
[2.5.3]: https://github.com/osmcode/libosmium/compare/v2.5.2...v2.5.3
[2.5.2]: https://github.com/osmcode/libosmium/compare/v2.5.1...v2.5.2
[2.5.1]: https://github.com/osmcode/libosmium/compare/v2.5.0...v2.5.1
[2.5.0]: https://github.com/osmcode/libosmium/compare/v2.4.1...v2.5.0
[2.4.1]: https://github.com/osmcode/libosmium/compare/v2.4.0...v2.4.1
[2.4.0]: https://github.com/osmcode/libosmium/compare/v2.3.0...v2.4.0
[2.3.0]: https://github.com/osmcode/libosmium/compare/v2.2.0...v2.3.0
[2.2.0]: https://github.com/osmcode/libosmium/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/osmcode/libosmium/compare/v2.0.0...v2.1.0

