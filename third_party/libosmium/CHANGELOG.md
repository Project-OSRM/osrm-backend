
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed

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

[unreleased]: https://github.com/osmcode/libosmium/compare/v2.3.0...HEAD
[2.3.0]: https://github.com/osmcode/libosmium/compare/v2.3.0...v2.3.0
[2.2.0]: https://github.com/osmcode/libosmium/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/osmcode/libosmium/compare/v2.0.0...v2.1.0

