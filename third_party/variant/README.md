# Mapbox Variant

An alternative to `boost::variant` for C++11.

[![Build Status](https://secure.travis-ci.org/mapbox/variant.svg)](https://travis-ci.org/mapbox/variant)
[![Build status](https://ci.appveyor.com/api/projects/status/v9tatx21j1k0fcgy)](https://ci.appveyor.com/project/Mapbox/variant)
[![Coverage Status](https://coveralls.io/repos/mapbox/variant/badge.svg?branch=master&service=github)](https://coveralls.io/r/mapbox/variant?branch=master)


## Why use Mapbox Variant?

Mapbox variant has the same speedy performance of `boost::variant` but is
faster to compile, results in smaller binaries, and has no dependencies.

For example on OS X 10.9 with clang++ and libc++:

Test | Mapbox Variant | Boost Variant
---- | -------------- | -------------
Size of pre-compiled header (release / debug) | 2.8/2.8 MB         | 12/15 MB
Size of simple program linking variant (release / debug)     | 8/24 K             | 12/40 K
Time to compile header     | 185 ms             |  675 ms

(Numbers from an older version of Mapbox variant.)


## Goals

Mapbox `variant` has been a very valuable, lightweight alternative for apps
that can use c++11 or c++14 but that do not want a boost dependency.
Mapbox `variant` has also been useful in apps that do depend on boost, like
mapnik, to help (slightly) with compile times and to majorly lessen dependence
on boost in core headers. The original goal and near term goal is to maintain
external API compatibility with `boost::variant` such that Mapbox `variant`
can be a "drop in". At the same time the goal is to stay minimal: Only
implement the features that are actually needed in existing software. So being
an "incomplete" implementation is just fine.

Currently Mapbox variant doesn't try to be API compatible with the upcoming
variant standard, because the standard is not finished and it would be too much
work. But we'll revisit this decision in the future if needed.

If Mapbox variant is not for you, have a look at [these other
implementations](doc/other_implementations.md).

Want to know more about the upcoming standard? Have a look at our
[overview](doc/standards_effort.md).


## Depends

 - Compiler supporting `-std=c++11` or `-std=c++14`

Tested with:

 - g++-4.7
 - g++-4.8
 - g++-4.9
 - g++-5
 - clang++-3.5
 - clang++-3.6
 - clang++-3.7
 - clang++-3.8
 - Visual Studio 2015

## Usage

There is nothing to build, just include `variant.hpp` and
`recursive_wrapper.hpp` in your project. Include `variant_io.hpp` if you need
the `operator<<` overload for variant.


## Unit Tests

On Unix systems compile and run the unit tests with `make test`.

On Windows run `scripts/build-local.bat`.


## Limitations

* The `variant` can not hold references (something like `variant<int&>` is
  not possible). You might want to try `std::reference_wrapper` instead.


## Deprecations

* The included implementation of `optional` is deprecated and will be removed
  in a future version. See https://github.com/mapbox/variant/issues/64.
* Old versions of the code needed visitors to derive from `static_visitor`.
  This is not needed any more and marked as deprecated. The `static_visitor`
  class will be removed in future versions.


## Benchmarks

The benchmarks depend on:

 - Boost headers (for benchmarking against `boost::variant`)
 - Boost built with `--with-timer` (used for benchmark timing)

On Unix systems set your boost includes and libs locations and run `make test`:

    export LDFLAGS='-L/opt/boost/lib'
    export CXXFLAGS='-I/opt/boost/include'
    make bench


## Check object sizes

    make sizes /path/to/boost/variant.hpp

