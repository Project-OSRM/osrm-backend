
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](https://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


## [2.15.6] - 2020-06-27

### Added

* Add `IdSetSmall::merge_sorted` function.

### Changed

* Little optimization for IdSetSmall: Don't add the same id twice in a row.

### Fixed

* Do not build areas with "recursion depth > 20". This happens when there
  are complex multipolygon with many rings touching in single points. This
  is a quick fix that hopefully keeps us going until we find a better
  solution.

## [2.15.5] - 2020-04-21

### Added

* Additional constructor for `builder::attr::member_type(_string)` taking
  char type making it even easier to generate test data.
* Allow single C string or `std::string` as argument for `builder::attr::_tag`.
  Must contain key and value separated by the equal sign.
* New `builder::attr::_t()` function to set tags from comma-separated string.
* New `nwr_array` iterator.
* Support for the PROJ library has now been declared deprecated. The old
  PROJ API (up to version PROJ 6) is currently still available, but will
  be removed in a future version. Support for the new PROJ API will not be
  in libosmium. See https://github.com/osmcode/osmium-proj for some code
  that might help you if you need this.

### Changed

* Check how much space is available in file system before resizing memory
  mapped file (not on Windows). This means we can, at least in some cases,
  show an error message instead of crashing the program.

### Fixed

* Parsing coordinates in PBF files did not work correctly if an lat/lon
  offset was specified (which almost never happens).
* Make OPL parser more strict: Attributes can only be specified once.
* Do not close stdout after writing OSM file to it.

## [2.15.4] - 2019-11-28

### Added

* Add osmium::Options::empty() for consistency with STL containers.

### Fixed

* Massive reduction of memory consumption in area assembly code. For some
  very complex polygons memory usage can drop from multiple gigabytes to just
  megabytes.

## [2.15.3] - 2019-09-16

### Added

* New header option "sorting" when reading and writing PBFs. If the header
  option "sorting" is set to `Type_then_ID`, the optional header property
  `Sort.Type_then_ID` is set on writing to PBF files. When reading PBF files
  with this header property, the "sorting" header option is set accordingly.

### Fixed

* Do not propagate C++ exception through C code. We are using the Expat
  XML parser, a C library. It calls callbacks in our code. When those
  callbacks throw, the exception was propagated through the C code. This
  did work in the tests, but that behaviour isn't guaranteed (C++
  standard says it is implementation defined). This fixes it by catching
  the exception and rethrowing it later.

## [2.15.2] - 2019-08-16

### Added

* Instead of handler classes, the `apply` function can now also take
  lambdas (or objects from classes implementing `operator()`).
* Add swap, copy constructor and assignment operator to IdSetDense.

### Changed

* Enable use of the old proj API in proj version 6. This is a stopgap
  solution until we find a better one.
* Better error messages when there is an error parsing a timestamp.
* Cleaned up a lot of code based on clang-tidy warnings.
* Ignore <bbox> or <bounds> subelement of <way> or <relation>. <bounds>
  elements are created by Overpass API as subelements of ways or relations
  when the "out bb" format is used. <bbox> subelements turn up in files
  downloaded from http://download.openstreetmap.fr/replication . Libosmium
  used to throw an error  like "Unknown element in <way>: bbox". With this
  commit, these subelements are ignored, ie. there is no error any more,
  but the data is not read.
* Add swap, copy constructor and assignment operator to IdSetDense.
* Update included catch.hpp to 1.12.2.
* Retire use of `OSMIUM_NORETURN` macro. Use `[[noreturn]]` instead.

### Fixed

* Do not build areas with more than 100 locations where rings touch.
  Places where rings touch are unusual for normal multipolygons and the
  algorithm in libosmium that assembles multipolygons does not handle
  them well. If there are too many touching points it becomes very slow.
  This is not a problem for almost all multipolygons. As I am writing
  this there are only three relations in the OSM database with more than
  100 touching points, all of them rather weird boundaries in the US.
  With this commit libosmium will simply ignore those areas to keep the
  processing speed within reasonable bounds.


## [2.15.1] - 2019-02-26

### Added

* More tests.
* CMake config: also find clang-tidy-7.

### Changed

* Example and benchmark programs now don't crash with exceptions any more
  but report them properly.

### Fixed

* Compile with NDEBUG in RelWithDebInfo mode.
* Correctly throw exception in `multimap::dump_as_list()`.
* Integer truncation on 32 bit systems in `MemoryUsage`.
* Exception specification on some functions.
* Forwarding references that might have hidden copy/move constructors.


## [2.15.0] - 2018-12-07

### Added

* Function `dump_as_array()` to dump sparse array indexes.
* Set the `xml_josm_upload` header option when reading XML files.
* New function `OSMObject::remove_tags()` marks tags on OSM objects as
  removed.
* More tests.

### Changed

* When reading OSM files Libosmium now has less memory overhead, especially
  when reading PBF files. This works by using more, but smaller buffers.
* The `TagsFilter` class is now based on the `TagsFilterBase` template
  class which allows setting the result type. This allows the filter to
  return more data depending on the rule that matched.
* Use enums for many constants instead of (static) const(expr) variables.
* Make `chunk_bits` in `IdSetDense` configurable.
* Hardcode `%lld` format instead of using `<cinttypes>` PRI macro.
* Update included gdalcpp to version 1.2.0.

### Fixed

* The gzip/bzip2 compression code was overhauled and is better tested now.
  This fixes some bugs on Windows.


## [2.14.2] - 2018-07-23

### Fixed

* PBF reader and writer depended on byte order of system architecture.
* Removed an unreliable test that didn't work on some architectures.


## [2.14.1] - 2018-07-23

### Changed

* Libosmium now needs the newest Protozero version 1.6.3.
* Removes dependency on the utfcpp library for conversions between Unicode
  code points and UTF-8. We have our own functions for this now. This also
  gives us more control on where errors are thrown in this code.
* Add support for using the CRC32 implementation from the zlib library in
  addition to the one from Boost. It is significantly faster and means we
  have one less dependency, because zlib is needed anyway in almost all
  programs using Osmium due to its use in the PBF format. Set macro
  `OSMIUM_TEST_CRC_USE_BOOST` before compiling the tests, if you want to
  run the tests with the boost library code, otherwise it will use the
  zlib code. Note that to use this you have to change your software slightly,
  see the documentation of the `CRC_zlib` class for details.
* Add a `clear_user()` function to OSMObject and Changeset which allows
  removing the user name of an entity without re-creating it in a new buffer.
* In Osmium the 0 value of the Timestamp is used to denote the "invalid"
  Timestamp, and its output using the `to_iso()` function is the empty
  string. But this is the wrong output for OSM XML files, where a
  timestamp that's not set should still be output as
  1970-01-01T00:00:00Z. This version introduces a new `to_is_all()`
  function which will do this and uses that function in the XML writer.
* Use `protozero::byteswap_inplace` instead of `htonl`/`ntohl`. Makes the
  code simpler and also works on Windows.
* Marked `MultipolygonCollector` class as deprecated. Use the
  `MultipolygonManager` class introduced in 2.13.0 instead.
* Lots of code cleanups especially around `assert`s. Libosmium checks out
  clean with `clang-tidy` now. Some documentation updates.

### Fixed

* Fix compilation error when `fileno()` is a macro (as in OpenBSD 6.3).
* Make `Box` output consistent with the output of a single `Location`
  and avoids problems with some locales.


## [2.14.0] - 2018-03-31

### Added

* Add `ReaderWithProgressBar` class. This wraps an `osmium::io::Reader` and an
  `osmium::ProgressBar` into a nice little package allowing easier use in the
  common case.
* Add polygon implementation for WKT and GeoJSON geometry factories. (Thanks
  to Horace Williams.)
* Various tests.

### Changed

* Add git submodule with `osm-testdata` repository. Before this the repository
  had to be installed externally. Now a submodule update can be used to get the
  correct version of the osm-testdata repository.
* The XML file reader was rewritten to be more strict. Cases where it could
  be tricked into failing badly were removed. There are now more tests for the
  XML parser.
* Replaced `strftime` by our own implementation. Uses a specialized
  implementation for our use case instead the more general `strftime`.
  Benchmarked this to be faster.
* Changed the way IDs are parsed from strings. No asserts are used any more but
  checks are done and an exception is thrown when IDs are out of range. This
  also changes the way negative values are handled. The value `-1` is now
  always accepted for all IDs and returned as `0`. This deprecates the
  `string_to_user_id()` function, use `string_to_uid()` instead which returns a
  different type.
* It was always a bit confusing that some of the util classes and functions are
  directly in the `osmium` namespace and some are in `osmium::util`. The
  `osmium::util` namespace is now declared `inline`. which allows all util
  classes and functions to be addressed directly in the `osmium` namespace
  while keeping backwards compatibility.
* An error is now thrown when the deprecated `pbf_add_metadata` file format
  option is used. Use `add_metadata` instead.
* Extended the `add_metadata` file format option. In addition to allowing the
  values `true`, `yes`, `false`, and `no`, the new values `all` and `none`
  are now recognized. The option can also be set to a list of attributes
  separated by the `+` sign. Attributes are `version`, `timestamp`,
  `changeset`, `uid`, and `user`. All output formats have been updated to
  only output the specified attributes. This is based on the new
  `osmium::metadata_options` class which stores information about what metadata
  an `OSMObject` has or should have. (Thanks to Michael Reichert.)
* The `<` (less than) operator on `OSMObject`s now ignores the case when
  one or both of the timestamps on the objects are not set at all. This
  allows better handling of OSM data files with reduced metadata.
* Allow `version = -1` and `changeset = -1` in PBF input. This value is
  sometimes used by other programs to denote "no value". Osmium uses the `0`
  for this.
* The example programs using the `getopt_long` function have been rewritten to
  work without it. This makes using libosmium on Windows easier, where this
  function is not available.
* Removed the embedded protozero from repository. Like other dependencies you
  have to install protozero first. If you check out the protozero repository
  in the same directory where you checked out libosmium, libosmium's CMake
  will find it.
* Various code cleanups, fixing of include order, etc.
* Remove need for `winsock2` library in Windows by using code from Protozero.
  (Thanks alex85k.)
* Add MSYS2 build to Appveyor and fixed some Windows compile issues. (Thanks
  to alex85k.)
* Use array instead of map to store input/output format creators.
* Update included `catch.hpp` to version 1.12.1.

### Fixed

* Remove check for lost ways in multipolygon assembler. This rules out too many
  valid multipolygons, more specifically more complex ones with touching inner
  rings.
* Use different macro magic for registering index maps. This allows the maps
  to be used for several types at the same time.
* Lots of code was rewritten to fix warnings reported by `clang-tidy` making
  libosmium more robust.
* Make ADL work for `begin()`/`end()` of `InputIterator<Reader>`.
* Various fixes to make the code more robust, including an undefined behaviour
  in the debug output format and a buffer overflow in the o5m parser.
* Range checks in o5m parser throw exceptions now instead of triggering
  assertions.
* Better checking that PBF data is in range.
* Check `read` and `write` system calls for `EINTR`.
* Use tag and type from protozero to make PBF parser more robust.
* Test `testdata-multipolygon` on Windows was using the wrong executable name.


## [2.13.1] - 2017-08-25

### Added

- New "blackhole" file format which throws away all data written into it.
  Used for benchmarking.

### Changed

- When reading OPL files, CRLF file endings are now handled correctly.
- Reduce the max number of threads allowed for the `Pool` to 32. This should
  still be plenty and might help with test failures on some architectures.

### Fixed

- Tests now run correctly independent of git `core.autocrlf` setting.
- Set binary mode for all files on Windows in example code.
- Low-level file functions now set an invalid parameter handler on Windows
  to properly handle errors.
- Restore earlier behaviour allowing zero-length mmap. It is important to
  allow zero-length memory mapping, because it is possible that such an index
  is empty, for instance when one type of object is missing from an input
  file as in https://github.com/osmcode/osmium-tool/issues/65. Drawback is
  that files must be opened read-write for this to work, even if we only
  want to read from them.
- Use Approx() to compare floating point values in tests.
- Fix broken `Item` test on 32 bit platforms.


## [2.13.0] - 2017-08-15

### Added

- New `RelationsManager` class superseeds the `relations::Collector` class.
  The new class is much more modular and easier to extend. If you are using
  the Collector class, you are encouraged to switch.
- New `MultipolygonManager` based on the `RelationsManager` class superseeds
  the `MultipolygonCollector` class. The examples have been changed to use the
  new class and all users are encouraged to switch. There is also a
  `MultipolygonManagerLegacy` class if you still need old-style multipolygon
  support (see below).
- New `FlexMem` index class that works with input files of any size and
  stores the index in memory. This should now be used as the default index
  for node location stores. Several example programs now use this index.
- New `CallbackBuffer` class, basically a convenient wrapper around the
  `Buffer` class with an additional callback function that is called whenever
  the buffer is full.
- Introduce new `ItemStash` class for storing OSM objects in memory.
- New `osmium::geom::overlaps()` function to check if two `Box` objects
  overlap.
- Add function `IdSet::used_memory()` to get estimate of memory used in the
  set.
- New `is_defined()` and `is_undefined()` methods on `Location` class.
- Tests for all provided example programs. (Some tests currently fail
  on Windows for the `osmium_index_lookup` program.)

### Changed

- The area `Assembler` now doesn't work with old-style multipolygons (those
  are multipolygon relations with the tags on the outer ways(s) instead of
  on the relation) any more. Because old-style multipolygons are now (mostly)
  gone from the OSM database this is usually what you want. The new
  `AssemblerLegacy` class can be used if you actually need support for
  old-style multipolygons, for instance if you are working with historical
  data. (In that case you also need to use the `MultipolygonManagerLegacy`
  class instead of the `MultipolygonManager` class.)
- Changes for consistent ordering of OSM data: OSM data can come in any order,
  but usual OSM files are ordered by type, ID, and version. These changes
  extend this ordering to negative IDs which are sometimes used for objects
  that have not been uploaded to the OSM server yet. The negative IDs are
  ordered now before the positive ones, both in order of their absolute value.
  This is the same ordering as JOSM uses.
- Multipolygon assembler now checks for three or more overlapping segments
  which are always an error and can report them.
- Enable use of user-provided `thread::Pool` instances in `Reader` and
  `Writer` for special use cases.
- Growing a `Buffer` will now work with any capacity parameter, it is
  always rounded up for proper alignment. Buffer constructor with three
  arguments will now check that commmitted is not larger than capacity.
- Updated embedded protozero to 1.5.2.
- Update version of Catch unit test framework to 1.9.7.
- And, as always, lots of small code cleanups and more tests.

### Fixed

- Buffers larger than 2^32 bytes do now work.
- Output coordinate with value of -2^31 correctly.
- Changeset comments with more than 2^16 characters are now allowed. The new
  maximum size is 2^32.
- `ChangesetDiscussionBuilder::add_comment_text()` could fail silently instead
  of throwing an exception.
- Changeset bounding boxes are now always output to OSM files (any format)
  if at least one of the corners is defined. This is needed to handle broken
  data from the main OSM database which contains such cases. The OPL reader
  has also been fixed to handle this case.
- In the example `osmium_location_cache_create`, the index file written is
  always truncated first.


## [2.12.2] - 2017-05-03

### Added

- Add two argument (key, value) overload of `TagMatcher::operator()`.

### Changed

- Detect, report, and remove duplicate ways in multipolygon relations.
- Change EOF behaviour of Reader: The `Reader::read()` function will now
  always return an invalid buffer exactly once to signal EOF.
- Update QGIS multipolygon project that is part of the test suite to show
  more problem types.
- Copy multipolygon QGIS file for tests to build dir in cmake step.
- Some code cleanups and improved debug output in multipolygon code.
- Refactor I/O code to simplify code.
- Disable some warnings on MSVC.
- Various small code and build script changes.

### Fixed

- Two bugs in area assembler affecting very complex multipolygons and
  multipolygons with overlapping or nearly overlapping lines.
- Invalid use of iterators leading to undefined behaviour in area assembler
  code.
- Area assembler stats were not correctly counting inner rings that are
  areas in their own right.
- Fix a thread problem valgrind found that might or might not be real.
- Read OPL file correctly even if trailing newline in file is missing.
- Include order for `osmium/index/map` headers and
  `osmium/index/node_locations_map.hpp` (or
  `osmium/handler/node_locations_for_ways.hpp`) doesn't matter any more.


## [2.12.1] - 2017-04-10

### Added

- New `TagsFilter::set_default_result()` function.

### Changed

- Use larger capacity for `Buffer` if necessary for alignment instead of
  throwing an exception. Minimum buffer size is now 64 bytes.
- Check order of input data in relations collector. The relations collector
  can not deal with history data or a changes file. This was documented as a
  requirement, but often lead to problems, because this was ignored by users.
  So it now checks that the input data it gets is ordered and throws an
  exception otherwise.
- When writing an OSM file, set generator to libosmium if not set by app.

### Fixed

- Infinite loop in `Buffer::reserve_space()`. (Issue #202.)
- `ObjectPointerCollection::unique()` now removes elements at end.
- Tests comparing double using `==` operator.
- Build on Cygwin.


## [2.12.0] - 2017-03-07

### Added

- `TagMatcher` and `TagsFilter` classes for more flexibly matching tags and
  selecting objects based on tags. This obsoletes the less flexible classes
  based on `osmium::tags::Filter` classes.
- Extended `index::RelationsMap(Stash|Index)` classes to also allow
  parent-to-member lookups.
- New `nrw_array` helper class.
- `ObjectPointerCollection::unique()` function.

### Changed

- Area assembler can now detect invalid locations and report them in the
  stats and through the problem reporter. If the new config option
  `ignore_invalid_locations` is set, the Assembler will pretend they weren't
  even referenced in the ways. (Issue #195.)
- `osmium::area::Assembler::operator()` will now return a boolean reporting
  whether building of the area(s) was successful.
- Split up area `Assembler` class into three classes: The
  `detail::BasicAssembler` is now the parent class. `Assembler` is the child
  class for usual use. The new `GeomAssembler` also derives from
  `BasicAssembler` and builds areas without taking tags into account at all.
  This is to support osm2pgsql which does tag handling itself. (Issue #194.)
- The `Projection` class can do any projection supported by the Proj.4
  library. As a special case it now uses our own Mercator projection
  functions when the web mercator projection (EPSG 3857) is used. This is
  much faster than going through Proj.4.
- Better error messages for low-level file utility functions.
- Mark `build_tag_list*` functions in `builder_helper.hpp` as deprecated. You
  should use the functions from `osmium/builder/attr.hpp` instead.
- Improved performance of the `osmium::tags::match_(any|all|none)_of`
  functions.
- Improved performance of string comparison in `tags::Filter`.
- Update version of Catch unit test framework to 1.8.1. This meant some
  tests had to be updated.
- Use `get_noexcept()` in `NodeLocationsForWays` handler.
- And lots of code and test cleanups...

### Fixed

- Terminate called on full non-auto-growing buffer. (Issue #189.)
- When file formats were used that were not compiled into the binary, it
  terminated instead of throwing. (Issue #197.)
- Windows build problem related to including two different winsock versions.
- Windows build problem related to forced build for old Windows versions.
  (Issue #196.)
- Clear stream contents in ProblemReporterException correctly.
- Add `-pthread` compiler and linker options on Linux/OSX. This should fix
  a problem where some linker versions will not link binaries correctly when
  the `--as-needed` option is used.
- The `Filter::count()` method didn't compile at all.
- XML reader doesn't fail on relation member ref=0 any more.


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

[unreleased]: https://github.com/osmcode/libosmium/compare/v2.15.6...HEAD
[2.15.6]: https://github.com/osmcode/libosmium/compare/v2.15.5...v2.15.6
[2.15.5]: https://github.com/osmcode/libosmium/compare/v2.15.4...v2.15.5
[2.15.4]: https://github.com/osmcode/libosmium/compare/v2.15.3...v2.15.4
[2.15.3]: https://github.com/osmcode/libosmium/compare/v2.15.2...v2.15.3
[2.15.2]: https://github.com/osmcode/libosmium/compare/v2.15.1...v2.15.2
[2.15.1]: https://github.com/osmcode/libosmium/compare/v2.15.0...v2.15.1
[2.15.0]: https://github.com/osmcode/libosmium/compare/v2.14.2...v2.15.0
[2.14.2]: https://github.com/osmcode/libosmium/compare/v2.14.1...v2.14.2
[2.14.1]: https://github.com/osmcode/libosmium/compare/v2.14.0...v2.14.1
[2.14.0]: https://github.com/osmcode/libosmium/compare/v2.13.1...v2.14.0
[2.13.1]: https://github.com/osmcode/libosmium/compare/v2.13.0...v2.13.1
[2.13.0]: https://github.com/osmcode/libosmium/compare/v2.12.2...v2.13.0
[2.12.2]: https://github.com/osmcode/libosmium/compare/v2.12.1...v2.12.2
[2.12.1]: https://github.com/osmcode/libosmium/compare/v2.12.0...v2.12.1
[2.12.0]: https://github.com/osmcode/libosmium/compare/v2.11.0...v2.12.0
[2.11.0]: https://github.com/osmcode/libosmium/compare/v2.10.3...v2.11.0
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

