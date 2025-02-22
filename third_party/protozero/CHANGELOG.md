
# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/)
This project adheres to [Semantic Versioning](https://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


## [1.7.1] - 2022-01-10

### Changed

- Don't build tests if the standard CMake `BUILD_TESTING` variable is set to
  off.
- Now needs CMake 3.5.0 or greater.
- Update included catch2 framework to current version v2.13.8.
- Only enable clang-tidy make target if protobuf was found.
- Allow setting C++ version to compile with in CMake config.

### Fixed

- Fixes undefined behaviour in `float` and `double` byteswap.
- Add missing includes of "config.hpp".
- Avoid narrowing conversion by doing an explicit `static_cast`.


## [1.7.0] - 2020-06-08

### Added

- Support for buffer types other that `std::string`. `pbf_writer` is now
  just a typedef for `basic_pbf_writer<std::string>`. Other buffer types
  can be used with `basic_pbf_writer`. See `doc/advanced.md` for details.

### Changed

- Switched to *catch2* for testing.
- Some minor tweaks.

### Fixed

- Removed some undefined behaviour.


## [1.6.8] - 2019-08-15

### Changed

- Various code cleanups due to clang-tidy warnings.

### Fixed

- Made `data_view::compare` noexcept.


## [1.6.7] - 2018-02-21

### Fixed

- Signed-unsigned comparison on 32 bit systems.


## [1.6.6] - 2018-02-20

### Fixed

- Fixed several place with possible undefined behaviour.


## [1.6.5] - 2018-02-05

### Fixed

- Avoid UB: Do not calculate pointer outside array bounds.
- Specify proto2 syntax in .proto files to appease protoc.


## [1.6.4] - 2018-11-08

### Added

- Add function `data()` to get the not yet read data from a `pbf_reader`.
- New `add_packed_fixed()` template function for `pbf_writer`.
- New `length_of_varint()` helper function calculates how long a varint
  would be for a specified value.

### Changed

- More consistent implementation of operators as free friend functions.

### Fixed

- Fixed some zigzag encoding tests on MSVC.
- Add extra cast so we do an xor with unsigned ints.
- No more bitwise operations on signed integers in varint decoder.
- No more bitwise operations on signed integers in zigzag encoder/decoder.


## [1.6.3] - 2018-07-17

### Changed

- Moved `byteswap_inplace` functions from detail into protozero namespace.
  They can be useful outsize protozero.
- More asserts and unit tests and small cleanups.


## [1.6.2] - 2018-03-09

### Changed

- Update included catch.hpp to v1.12.0.
- Move basic unit tests into their own directory (`test/unit`).
- Improved clang-tidy config and fixed some code producing warnings.

### Fixed

- Buffer overflow in pbf-decoder tool.


## [1.6.1] - 2017-11-16

### Added

- Document internal handling of varints.
- Add aliases for fixed iterators, too.

### Changed

- The `const_fixed_iterator` is now a random access iterator making code
  using it potentially more performant (for instance when using
  `std::distance`)
- Overloads `std::distance` for the varint and svarint iterators. This is
  better than the workaround with the `rage_size` function used before.

### Fixed

- Rename `.proto` files in some tests to be unique. This solves a problem
  when building with newer versions of the Google Protobuf library.
- Floating point comparisons in tests are now always correctly done using
  `Approx()`.


## [1.6.0] - 2017-10-24

### Added

- Comparison functions (<, <=, >, >=) for `data_view`. Allows use in `std::map`
  for instance.
- Tool `pbf-decoder` for decoding raw messages. This has limited use for
  normal users, but it can be used for fuzzing.

### Changed

- Protozero now uses CMake to build the tests etc. This does not affect
  simple users of the library, but if you are using CMake yourself you might
  want to use the `cmake/FindProtozero.cmake` module provided. The README
  contains more information about build options.
- Moved `data_view` class from `types.hpp` into its own header file
  `data_view.hpp`.
- Implementation of the `const_fixed_iterator` to use only a single pointer
  instead of two.
- Made `operator==` and `operator!=` on `data_view` constexpr.
- The `pbf_reader` constructor taking a `std::pair` is deprecated. Use one
  of the other constructors instead.

### Fixed

- Varints where the last byte was larger than what would fit in 64bit were
  triggering undefined behaviour. This can only happen when the message
  being decoded was corrupt in some way.
- Do not assert when reading too long varints for bools any more. A valid
  encoder should never generate varints with more than one byte for bools,
  but if they are longer that's not really a problem, so just handle it.
- Throw exception if the length of a packed repeated field of a fixed-length
  type is invalid. The length must always be a multiple of the size of the
  underlying type. This can only happen if the data is corrupted in some way,
  a valid encoder would never generate data like this.
- Throw an exception when reading invalid tags. This can only happen if the
  data is corrupted in some way, a valid encoder would never generate invalid
  tags.


## [1.5.3] - 2017-09-22

### Added

- More documentation.
- New `size()` method on iterator range used for packed repeated fields to
  find out how many elements there are in the range. This is much faster
  compared to the `std::difference()` call you had to do before, because the
  varints don't have to be fully decoded. See [Advanced
  Topics](doc/advanced.md) for details.

### Changed

- Updated clang-tidy settings in Makefiles and fixed a lot of minor issues
  reported by clang-tidy.
- Update included catch.hpp to version 1.10.0.
- Miscellaneous code cleanups.
- Support for invalid state in `pbf_writer` and `packed_repeated_fields`.
  This fixes move construction and move assignement in `pbf_writer` and
  disables the copy construction and copy assignement which don't have
  clear semantics. It introduces an invalid or empty state in the
  `pbf_writer`, `pbf_builder`, and `packed_repeated_fields` classes used for
  default-constructed, moved from, or committed objects. There is a new
  `commit()` function for `pbf_writer` and the `packed_repeated_fields` which
  basically does the same as the destructor but can be called explicitly.

### Fixed

- The `empty()` method of the iterator range now returns a `bool` instead of
  a `size_t`.


## [1.5.2] - 2017-06-30

### Added

- Add missing two-parameter version of `pbf_message::next()` function.
- Add `data_view::empty()` function.
- Add missing versions of `add_bytes()`, `add_string()`, and `add_message()`
  to `pbf_builder`.

### Changed

- Clarify include file usage in tutorial.
- Updated included Catch unit test framework to version 1.9.6 and updated
  tests to work with the current version.
- Make some constructors explicit (best practice to avoid silent conversions).

### Fixed

- Important bugfix in `data_view` equality operator. The equality operator is
  actually never used in the protozero code itself, but users of protozero
  might use it. This is a serious bug that could lead to buffer overrun type
  problems.


## [1.5.1] - 2017-01-14

### Added

- Better documentation for `tag_and_type()` in doc/advanced.md.

### Fixed

- Fixed broken "make doc" build.


## [1.5.0] - 2017-01-12

### Added

- Add `add_bytes_vectored()` methods to `pbf_writer` and `pbf_builder`. This
  allows single-copy scatter-gather type adding of data that has been prepared
  in pieces to a protobuf message.
- New functions to check the tag and wire type at the same time: Two parameter
  version of `pbf_reader::next()` and `pbf_reader::tag_and_type()` can be used
  together with the free function `tag_and_type()` to easily and quickly check
  that not only the tag but also the wire type is correct for a field.

### Changed

- `packed_field_*` classes now work with `pbf_builder`.
- Reorganized documentation. Advanced docs are now under doc/advanced.md.

### Fixed

- `packed_field` class is now non-copyable because data can get corrupted if
  you copy it around.
- Comparison operators of `data_view` now have const& parameters.
- Make zigzag encoding/decoding functions constexpr.


## [1.4.5] - 2016-11-18

### Fixed

- Undefined behaviour in packed fixed iterator. As a result, the macro
  `PROTOZERO_DO_NOT_USE_BARE_POINTER` is not used any more.


## [1.4.4] - 2016-11-15

### Fixed

- Byteswap implementation.


## [1.4.3] - 2016-11-15

### Fixed

- Undefined behaviour in byte swapping code.
- Rename some parameters to avoid "shadow" warning from some compilers.


## [1.4.2] - 2016-08-27

### Fixed

- Compile fix: Variable shadowing.


## [1.4.1] - 2016-08-21

### Fixed

- GCC 4.8 compile fixed

### Added

- New ability to dynamically require the module as a node module to ease
  building against from other node C++ modules.

## [1.4.0] - 2016-07-22

### Changed

- Use more efficient new `skip_varint()` function when iterating over
  packed varints.
- Split `decode_varint()` function into two functions speeding up the
  common case where a varint is only one byte long.
- Introduce new class `iterator_range` used instead of `std::pair` of
  iterators. This way the objects can be used in range-based for loops.
  Read UPGRADING.md for details.
- Introduce new class `data_view` and functions using and returning it.
  Read UPGRADING.md for details.


## [1.3.0] - 2016-02-18

### Added

- Added `config.hpp` header which now includes all the macro magic to
  configure the library for different architectures etc.
- New way to create repeated packed fields without using an iterator.
- Add `rollback()` function to `pbf_writer` for "manual" rollback.

### Changed

- Various test and documentation cleanups.
- Rename `pbf_types.hpp` to `types.hpp`.


## [1.2.3] - 2015-11-30

### Added

- Added `config.hpp` header which now includes all the macro magic to
  configure the library for different architectures etc.

### Fixed

- Unaligned access to floats/doubles on some ARM architectures.


## [1.2.2] - 2015-10-13

### Fixed

- Fix the recently broken writing of bools on big-endian architectures.


## [1.2.1] - 2015-10-12

### Fixed

- Removed unneeded code (1-byte "swap") which lead to test failures.


## [1.2.0] - 2015-10-08

### Added

- `pbf_message` and `pbf_builder` template classes wrapping `pbf_reader`
  and `pbf_writer`, respectively. The new classes are the preferred
  interface now.

### Changed

- Improved byte swapping operation.
- Detect some types of data corruption earlier and throw.


## [1.1.0] - 2015-08-22

### Changed

- Make pbf reader and writer code endianess-aware.


[unreleased]: https://github.com/osmcode/libosmium/compare/v1.7.1...HEAD
[1.7.1]: https://github.com/osmcode/libosmium/compare/v1.7.0...v1.7.1
[1.7.0]: https://github.com/osmcode/libosmium/compare/v1.6.8...v1.7.0
[1.6.8]: https://github.com/osmcode/libosmium/compare/v1.6.7...v1.6.8
[1.6.7]: https://github.com/osmcode/libosmium/compare/v1.6.6...v1.6.7
[1.6.6]: https://github.com/osmcode/libosmium/compare/v1.6.5...v1.6.6
[1.6.5]: https://github.com/osmcode/libosmium/compare/v1.6.4...v1.6.5
[1.6.4]: https://github.com/osmcode/libosmium/compare/v1.6.3...v1.6.4
[1.6.3]: https://github.com/osmcode/libosmium/compare/v1.6.2...v1.6.3
[1.6.2]: https://github.com/osmcode/libosmium/compare/v1.6.1...v1.6.2
[1.6.1]: https://github.com/osmcode/libosmium/compare/v1.6.0...v1.6.1
[1.6.0]: https://github.com/osmcode/libosmium/compare/v1.5.3...v1.6.0
[1.5.3]: https://github.com/osmcode/libosmium/compare/v1.5.2...v1.5.3
[1.5.2]: https://github.com/osmcode/libosmium/compare/v1.5.1...v1.5.2
[1.5.1]: https://github.com/osmcode/libosmium/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/osmcode/libosmium/compare/v1.4.5...v1.5.0
[1.4.5]: https://github.com/osmcode/libosmium/compare/v1.4.4...v1.4.5
[1.4.4]: https://github.com/osmcode/libosmium/compare/v1.4.3...v1.4.4
[1.4.3]: https://github.com/osmcode/libosmium/compare/v1.4.2...v1.4.3
[1.4.2]: https://github.com/osmcode/libosmium/compare/v1.4.1...v1.4.2
[1.4.1]: https://github.com/osmcode/libosmium/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/osmcode/libosmium/compare/v1.3.0...v1.4.0
[1.3.0]: https://github.com/osmcode/libosmium/compare/v1.2.3...v1.3.0
[1.2.3]: https://github.com/osmcode/libosmium/compare/v1.2.2...v1.2.3
[1.2.2]: https://github.com/osmcode/libosmium/compare/v1.2.1...v1.2.2
[1.2.1]: https://github.com/osmcode/libosmium/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/osmcode/libosmium/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/osmcode/libosmium/compare/v1.0.0...v1.1.0

