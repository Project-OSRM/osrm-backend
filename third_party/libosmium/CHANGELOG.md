
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


## [2.11.0] - 2017-01-14

### Added

- New index::RelationsMap(Stash|Index) classes implementing an index for
  looking up parent relation IDs given a member relation ID.
- Add `get_noexcept()` method to all index maps. For cases where ids are
  often not in the index using this can speed up a program considerably.
- New non-const WayNodeList::operator[].
- Default constructed "invalid" Coordinates.
- Tile constructor from web mercator coordinates and some helper
  functions for tile arithmetic.
- Tag matcher matching keys using a regex.
- New `envelope()` functions on `NodeRefList`, `Way`, and `Area` returning a
  `Box` object with the geometric envelope of the object.
- Add `amenity_list` example.

### Changed

- Replaced the implementation for the web mercator projection using the usual
  tan-formula with a polynomial approximation which is much faster and good
  enough for OSM data which only has ~1cm resolution anyway. See
  https://github.com/osmcode/mercator-projection for all the details and
  benchmarks. You can disable this by defining the macro
  `OSMIUM_USE_SLOW_MERCATOR_PROJECTION` before including any of the Osmium
  headers.
- Removed the outdated `Makefile`. Always use CMake directly to build.
- Refactoring of `osmium::apply()` removing the resursive templates for faster
  compile times and allowing rvalue handlers.
- Lots of code and test cleanups and more documentation.

### Fixed

- Handle endianess on FreeBSD properly.
- Fixed doxygen config for reproducible builds.


## [2.10.3] - 2016-11-20

### Changed

- Round out ObjectPointerCollection implementation and test it.
- Updated embedded protozero to 1.4.5.


## [2.10.2] - 2016-11-16

### Changed

- Updated embedded protozero to 1.4.4.

### Fixed

- Buffer overflow in osmium::Buffer.


## [2.10.1] - 2016-11-15

### Changed

- Updated embedded protozero to 1.4.3.

### Fixed

- Made IdSet work on 32bit systems.
- Fixed endianness check for WKB tests.


## [2.10.0] - 2016-11-11

### Added

- The `Reader` can take an additional optional `read_meta` flag. If this is
  set to false the PBF input will ignore metadata on OSM objects (like version,
  timestamp, uid, ...) which speeds up file reading by 10 to 20%.
- New `IdSet` virtual class with two implementations: `IdSetDense` and
  `IdSetSmall`. Used to efficiently store a set of Ids. This is often needed
  to track, for instance, which nodes are needed for ways, etc.
- Added more examples and better documented existing examples.
- Add a benchmark "mercator" converting all node locations in a file to
  WebMercator and creating geometries in WKB format.

### Changed

- Better queue handling makes I/O faster in some circumstances.
- The `FindOsmium.cmake` CMake script can now check a current enough libosmium
  version is found.
- Builders can now be constructed with a reference to parent builder.
- Made builders more robust by adding asserts that will catch common usage
  problems.
- Calling `OSMObjectBuilder::add_user()` is now optional, and the method was
  renamed to `set_user()`. (`add_user()` is marked as deprecated.)
- Benchmarks now show compiler and compiler options used.
- `Builder::add_item()` now takes a reference instead of pointer (old version
  of the function marked as deprecated).
- GEOS support is deprecated. It does not work any more for GEOS 3.6 or newer.
  Reason is the changed interface in GEOS 3.6. If there is interest for the
  GEOS support, we can add support back in later (but probably using the
  GEOS C API which is more stable than the C++ API). Some tests using GEOS
  were rewritten to work without it.
- The `BoolVector` has been deprecated in favour of the new `IdSet` classes.
- Lots of code cleanups and improved API documentation in many places.
- The relations collector can now tell you whether a relation member was in
  the input data. See the new `is_available()` and
  `get_availability_and_offset()` methods.
- Updated embedded Catch unit test header to version 1.5.8.

### Fixed

- Parsing of coordinates starting with decimal dot and coordinates in
  scientific notation.
- `~` operator for `entity_bits` doesn't set unused bits any more.
- Progress bar can now be (temporarily) removed, to allow other output.


## [2.9.0] - 2016-09-15

### Added

- Support for reading OPL files.
- For diff output OSM objects in buffers can be marked as only in one or the
  other file. The OPL and debug output formats support diff output based on
  this.
- Add documentation and range checks to `Tile` struct.
- More documentation.
- More examples and more extensive comments on examples.
- Support for a progress report in `osmium::io::Reader()` and a `ProgressBar`
  utility class to use it.
- New `OSMObject::set_timestamp(const char*)` function.

### Changed

- Parse coordinates in scientific notations ourselves.
- Updated included protozero version to 1.4.2.
- Lots of one-argument constructors are now explicit.
- Timestamp parser now uses our own implementation instead of strptime.
  This is faster and independant of locale settings.
- More cases of invalid areas with duplicate segments are reported as
  errors.

### Fixed

- Fixed a problem limiting cache file sizes on Windows to 32 bit.
- Fixed includes.
- Exception messages for invalid areas do not report "area contains no rings"
  any more, but "invalid area".


## [2.8.0] - 2016-08-04

### Added

- EWKT support.
- Track `pop` type calls and queue underruns when `OSMIUM_DEBUG_QUEUE_SIZE`
  environment variable is set.

### Changed

- Switched to newest protozero v1.4.0. This should deliver some speedups
  when parsing PBF files. This also removes the DeltaEncodeIterator class,
  which isn't needed any more.
- Uses `std::unordered_map` instead of `std::map` in PBF string table code
  speeding up writing of PBF files considerably.
- Uses less memory when writing PBF files (smaller string table by default).
- Removes dependency on sparsehash and boost program options libraries for
  examples.
- Cleaned up threaded queue code.

### Fixed

- A potentially very bad bug was fixed: When there are many and/or long strings
  in tag keys and values and/or user names and/or relation roles, the string
  table inside a PBF block would overflow. I have never seen this happen for
  normal OSM data, but that doesn't mean it can't happen. The result is that
  the strings will all be mixed up, keys for values, values for user names or
  whatever.
- Automatically set correct SRID when creating WKB and GEOS geometries.
  Note that this changes the behaviour of libosmium when creating GEOS
  geometries. Before we created them with -1 as SRID unless set otherwise.
  Manual setting of the SRID on the GEOSGeometryFactory is now deprecated.
- Allow coordinates of nodes in scientific notation when reading XML files.
  This shouldn't be used really, but sometimes you can find them.


## [2.7.2] - 2016-06-08

### Changed

- Much faster output of OSM files in XML, OPL, or debug formats.

### Fixed

- Parsing and output of coordinates now faster and always uses decimal dot
  independant of locale setting.
- Do not output empty discussion elements in changeset XML output.
- Data corruption regression in mmap based indexes.


## [2.7.1] - 2016-06-01

### Fixes

- Update version number in version.hpp.


## [2.7.0] - 2016-06-01

### Added

- New functions for iterating over specific item types in buffers
  (`osmium::memory::Buffer::select()`), over specific subitems
  (`osmium::OSMObject::subitems()`), and for iterating over all rings of
  an area (`osmium::Areas::outer_rings()`, `inner_rings()`).
- Debug output optionally prints CRC32 when `add_crc32` file option is set.

### Changed

- XML parser will not allow any XML entities which are usually not used in OSM
  files anyway. This can help avoiding DOS attacks.
- Removed SortedQueue implementation which was never used.
- Also incorporate Locations in NodeRefs into CRC32 checksums. This means
  all checksums will be different compared to earlier versions of libosmium.
- The completely new algorithm for assembling multipolygons is much faster,
  has better error reporting, generates statistics and can build more complex
  multipolygons correctly. The ProblemReporter classes have changed to make
  this happen, if you have written your own, you have to fix it.
- Sparse node location stores are now only sorted if needed, ie. when nodes
  come in unordered.

### Fixed

- Output operator for Location shows full precision.
- Undefined behaviour in WKB writer and `types_from_string()` function.
- Fix unsigned overflow in pool.hpp.
- OSM objects are now ordered by type (nodes, then ways, then relations),
  then ID, then version, then timestamp. Ordering by timestamp is normally
  not necessary, because there can't be two objects with same type, ID, and
  version but different timestamp. But this can happen when diffs are
  created from OSM extracts, so we check for this here. This change also
  makes sure IDs are always ordered by absolute IDs, positives first, so
  order is 0, 1, -1, 2, -2, ...
- Data corruption bug fixed in disk based indexes (used for the node
  location store for instance). This only affected you, if you created
  and index, closed it, and re-opened it (possibly in a different process)
  and if there were missing nodes. If you looked up those nodes, you got
  location (0,0) back instead of an error.
- Memory corruption bug showing up with GDAL 2.


## [2.6.1] - 2016-02-22

### Added

- Add `WITH_PROFILING` option to CMake config. When enabled, this sets the
  `-fno-omit-frame-pointer` compiler option.

### Changed

- Massive speed improvements when building multipolygons.
- Uses (and includes) new version 1.3.0 of protozero library.
- Removed dependency on Boost Iterator for PBF writer.
- Example program `osmium_area_test` now uses `cerr` instead of `cout` for
  debug output.


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
- I/O functions that used to throw `std::runtime_error` now throw
  `osmium::io_error` or derived.
- Optional parameters on `osmium::io::Writer` now work in any order.

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

[unreleased]: https://github.com/osmcode/libosmium/compare/v2.10.3...HEAD
[2.10.3]: https://github.com/osmcode/libosmium/compare/v2.10.2...v2.10.3
[2.10.2]: https://github.com/osmcode/libosmium/compare/v2.10.1...v2.10.2
[2.10.1]: https://github.com/osmcode/libosmium/compare/v2.10.0...v2.10.1
[2.10.0]: https://github.com/osmcode/libosmium/compare/v2.9.0...v2.10.0
[2.9.0]: https://github.com/osmcode/libosmium/compare/v2.8.0...v2.9.0
[2.8.0]: https://github.com/osmcode/libosmium/compare/v2.7.2...v2.8.0
[2.7.2]: https://github.com/osmcode/libosmium/compare/v2.7.1...v2.7.2
[2.7.1]: https://github.com/osmcode/libosmium/compare/v2.7.0...v2.7.1
[2.7.0]: https://github.com/osmcode/libosmium/compare/v2.6.1...v2.7.0
[2.6.1]: https://github.com/osmcode/libosmium/compare/v2.6.0...v2.6.1
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

