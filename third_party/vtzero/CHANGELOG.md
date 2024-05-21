
# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/)
This project adheres to [Semantic Versioning](https://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


## [1.1.0] - 2020-06-11

### Changed

* Now needs protozero 1.7.0 or above.
* Use `protozero::basic_pbf_builder` to make buffer type configurable. This
  allows you to create the final vector tile in any type of buffer, not just
  `std::string`. See documentation for details.
* Switch to catch2 for testing.

### Fixed

* Examples `vtzero-create` and `vtzero-streets` now commit features written.
* Various fixes and small cleanups, mostly based on clang-tidy reports.


## [1.0.3] - 2018-07-17

### Added

* New `copy_id()` helper function on feature builder copies ID (if it exists)
  from an existing feature.
* New `copy_properties()` helper funtion on feature builder copies all
  properties from an existing feature, optionally using a `property_mapper`.
* New `feature::for_each_property_indexes()` member function.

### Fixed

* The example program `vtzero-stats` now catches exceptions and exists with
  an error message.
* Fix an assert where a wrong iterator was checked.


## [1.0.2] - 2018-06-26

### Fixed

* `layer_builder::add_feature()` did not work, because it didn't commit
  the features it added.


## [1.0.1] - 2018-04-12

### Added

* Some documentation and tests.

### Changed

* Catch exceptions in vtzero-streets example and output error message.
* Adds a template parameter to the `create_property_map` function allowing
  mapping between value types.

### Fixed

* The indexes returned by `feature::next_property_indexes()` are now
  checked against the size of the key/value tables in the layer. If
  an index is too large a `vtzero::out_of_range_exception` is returned.
  This way the user code doesn't have to check this. The function
  `feature::for_each_property()` now also uses these checks.


## [1.0.0] - 2018-03-09

First release


[unreleased]: https://github.com/osmcode/libosmium/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/osmcode/libosmium/compare/v1.0.3...v1.1.0
[1.0.3]: https://github.com/osmcode/libosmium/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/osmcode/libosmium/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/osmcode/libosmium/compare/v1.0.0...v1.0.1

