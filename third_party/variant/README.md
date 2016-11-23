# Mapbox Variant

An header-only alternative to `boost::variant` for C++11 and C++14

[![Build Status](https://secure.travis-ci.org/mapbox/variant.svg)](https://travis-ci.org/mapbox/variant)
[![Build status](https://ci.appveyor.com/api/projects/status/v9tatx21j1k0fcgy)](https://ci.appveyor.com/project/Mapbox/variant)
[![Coverage Status](https://coveralls.io/repos/mapbox/variant/badge.svg?branch=master&service=github)](https://coveralls.io/r/mapbox/variant?branch=master)

## Introduction

Variant's basic building blocks are:
- `variant<Ts...>` - a type-safe representation for sum-types / discriminated unions
- `recursive_wrapper<T>` - a helper type to represent recursive "tree-like" variants
- `apply_visitor(visitor, myVariant)` - to invoke a custom visitor on the variant's underlying type
- `get<T>()` - a function to directly unwrap a variant's underlying type
- `.match([](Type){})` - a variant convenience member function taking an arbitrary number of lambdas creating a visitor behind the scenes and applying it to the variant


### Basic Usage - HTTP API Example

Suppose you want to represent a HTTP API response which is either a JSON result or an error:

```c++
struct Result {
  Json object;
};

struct Error {
  int32_t code;
  string message;
};
```

You can represent this at type level using a variant which is either an `Error` or a `Result`:

```c++
using Response = variant<Error, Result>;

Response makeRequest() {
  return Error{501, "Not Implemented"};
}

Response ret = makeRequest();
```

To see which type the `Response` holds you pattern match on the variant unwrapping the underlying value:

```c++
ret.match([] (Result r) { print(r.object); }
          [] (Error e)  { print(e.message); });
```

Instead of using the variant's convenience `.match` pattern matching function you can create a type visitor functor and use `apply_visitor` manually:

```c++
struct ResponseVisitor {
  void operator()(Result r) const {
    print(r.object);
  }

  void operator()(Error e) const {
    print(e.message);
  }
};

ResponseVisitor visitor;
apply_visitor(visitor, ret);
```

In both cases the compiler makes sure you handle all types the variant can represent at compile.


### Recursive Variants - JSON Example

[JSON](http://www.json.org/) consists of types `String`, `Number`, `True`, `False`, `Null`, `Array` and `Object`.


```c++
struct String { string value; };
struct Number { double value; };
struct True   { };
struct False  { };
struct Null   { };
struct Array  { vector<?> values; };
struct Object { unordered_map<string, ?> values; };
```

This works for primitive types but how do we represent recursive types such as `Array` which can hold multiple elements and `Array` itself, too?

For these use cases Variant provides a `recursive_wrapper` helper type which lets you express recursive Variants.

```c++
struct String { string value; };
struct Number { double value; };
struct True   { };
struct False  { };
struct Null   { };

// Forward declarations only
struct Array;
struct Object;

using Value = variant<String, Number, True, False, Null, recursive_wrapper<Array>, recursive_wrapper<Object>>;

struct Array {
  vector<Value> values;
};

struct Object {
  unordered_map<string, Value> values;
};
```

For walkig the JSON representation you can again either create a `JSONVisitor`:

```c++
struct JSONVisitor {

  void operator()(Null) const {
    print("null");
  }

  // same for all other JSON types
};

JSONVisitor visitor;
apply_visitor(visitor, json);
```

Or use the convenience `.match` pattern matching function:

```c++
json.match([] (Null) { print("null"); },
           ...);
```

To summarize: use `recursive_wrapper` to represent recursive "tree-like" representations:

```c++
struct Empty { };
struct Node;

using Tree = variant<Empty, recursive_wrapper<Node>>;

struct Node {
  uint64_t value;
}
```
### Advanced Usage Tips

Creating type aliases for variants is a great way to reduce repetition.
Keep in mind those type aliases are not checked at type level, though.
We recommend creating a new type for all but basic variant usage:

```c++
// the compiler can't tell the following two apart
using APIResult = variant<Error, Result>;
using FilesystemResult = variant<Error, Result>;

// new type
struct APIResult : variant<Error, Result> {
  using Base = variant<Error, Result>;
  using Base::Base;
}
```


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

Most modern high-level languages provide ways to express sum types directly.
If you're curious have a look at Haskell's pattern matching or Rust's and Swift's enums.


## Depends

 - Compiler supporting `-std=c++11` or `-std=c++14`

Tested with:

 - g++-4.7
 - g++-4.8
 - g++-4.9
 - g++-5.2
 - clang++-3.5
 - clang++-3.6
 - clang++-3.7
 - clang++-3.8
 - clang++-3.9
 - Visual Studio 2015


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

    make bench


## Check object sizes

    make sizes /path/to/boost/variant.hpp

