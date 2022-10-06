
# Vtzero Tutorial

The vtzero header-only library is used to read and write vector tile data
as specified in the [Mapbox Vector Tile
Specification](https://github.com/mapbox/vector-tile-spec). This document
assumes that you are familiar with that specification.

## Overview

The library has basically two parts: The part concerned with decoding existing
vector tiles and the part concerned with creating new vector tiles. You can
use either one without knowing much about the other side, but it is, of course
also possible to read parts of a vector tile and stick it into a new one.

Vtzero is trying to do as little work as possible while still giving you a
reasonably easy to use interface. This means that it will, as much as feasible,
decode the different parts of a vector tile only when you ask for them. Most
importantly it will try to avoid memory allocation and it will not copy data
around unnecessarily but work with references instead.

On the writing side it means that you have to call the API in a specific order
when adding more data to a vector tile. This allows vtzero to avoid multiple
copies of the data.

## Basic types

Vtzero contains several basic small (value) types such as `GeomType`,
`property_value_type`, `index_value`, `index_value_pair`, and `point` which
hold basic values in a type-safe way. Most of them are defined in `types.hpp`
(`point` is in `geometry.hpp`).

Sometimes it is useful to be able to print the values of those types, for
instance when debugging. For this overloads of `operator<<` on `basic_ostream`
are available in `vtzero/output.hpp`. Include this file and you can use the
usual `std::cout << some_value;` to print those values.

## Use of asserts and exceptions

The vtzero library uses a lot of asserts to help you use it correctly. It is
recommended you use debug builds while developing your code, because you'll
get asserts from vtzero telling you when you use it in a wrong way. This is
especially important when writing tiles using the builder classes, because
their methods have to be called in a certain order that might not always be
obvious but is checked by the asserts.

Exceptions, on the other hand, are used when errors occur during the normal run
of vtzero. This is especially important on the reading side when vtzero makes
every effort to handle any kind of input, even if the input data is corrupt
in some way. (Vtzero can't detect all problems though, your code still has to
do its own checking, see the [advanced topics](advanced.md) for some more
information.)

Many vtzero functions can throw exceptions. Most of them fall into these
categories:

* If the underlying protocol buffers data has some kind of problem, you'll
  get an exception from the [protozero
  library](https://github.com/mapbox/protozero/blob/master/doc/tutorial.md#asserts-and-exceptions-in-the-protozero-library).
  They are all derived from `protozero::exception`.
* If the protocol buffers data is okay, but the vector tile data is invalid
  in some way, you'll get an exception from the vtzero library.
* If any memory allocation failed, you'll get a `std::bad_alloc` exception.

All the exceptions thrown directly by the vtzero library are derived from
`vtzero::exception`. These exceptions are:

* A `format_exception` is thrown when vector tile encoding isn't valid
  according to the vector tile specification.
* A `geometry_exception` is thrown when a geometry encoding isn't valid
  according to the vector tile specification.
* A `type_exception` is thrown when a property value is accessed using the
  wrong type.
* A `version_exception` is thrown when an unknown version number is found in
  the layer. Currently vtzero only supports version 1 and 2.
* An `out_of_range_exception` is thrown when an index into the key or value
  table in a layer is out of range. This can only happen if the tile data is
  invalid.

## Include files

Usually you only directly include the following files:

* When reading: `<vtzero/vector_tile.hpp>`
* When writing: `<vtzero/builder.hpp>`
* If you need any of the special indexes: `<vtzero/index.hpp>`
* If you want overloads of `operator<<` for basic types: `<vtzero/output.hpp>`
* If you need the version: `<vtzero/version.hpp>`

## Reading and writing vector tiles

* [Reading vector tiles](reading.md)
* [Writing vector tiles](writing.md)

