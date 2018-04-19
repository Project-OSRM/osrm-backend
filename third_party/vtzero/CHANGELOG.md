
# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/)
This project adheres to [Semantic Versioning](https://semver.org/).

## [unreleased] -

### Added

### Changed

### Fixed


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

