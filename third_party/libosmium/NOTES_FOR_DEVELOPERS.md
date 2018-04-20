
# Notes for Developers

Read this if you want to contribute to Libosmium.


## Namespace

All Osmium code MUST be in the `osmium` namespace or one of its sub-namespaces.


## Include-Only

Osmium is a include-only library. You can't compile the library itself. There
is no `libosmium.so` or `libosmium.dll`.

One drawback ist that you can't have static data in classes, because there
is no place to put this data.

All free functions must be declared `inline`.


## Coding Conventions

These coding conventions have been changing over time and some code is still
different.

* All include files have `#ifdef` guards around them, macros are the path name
  in all uppercase where the slashes (`/`) have been changed to underscore (`_`).
* Class names begin with uppercase chars and use CamelCase. Smaller helper
  classes are usually defined as struct and have lowercase names.
* Macros (and only macros) are all uppercase. Use macros sparingly, usually
  a simple (maybe constexpr) inline function is better. Undef macros after use
  if possible.
* Macros should only be used for controlling which parts of the code should be
  included when compiling or to avoid major code repetitions.
* Variables, attributes, and function names are lowercase with
  `underscores_between_words`.
* Class attribute names start with `m_` (member).
* Use `descriptive_variable_names`, exceptions are well-established conventions
  like `i` for a loop variable. Iterators are usually called `it`.
* Declare variables where they are first used (C++ style), not at the beginning
  of a function (old C style).
* Names from external namespaces (even `std`) are always mentioned explicitly.
  Do not use `using` (except for `std::swap`). This way we can't even by
  accident pollute the namespace of the code using Osmium.
* Always use the standard swap idiom: `using std::swap; swap(foo, bar);`.
* `#include` directives appear in three "blocks" after the copyright notice.
  The blocks are separated by blank lines. First block contains `#include`s for
  standard C/C++ includes, second block for any external libs used, third
  block for osmium internal includes. Within each block `#include`s are usually
  sorted by path name. All `#include`s use `<>` syntax not `""`.
* Names not to be used from outside the library should be in a namespace
  called `detail` under the namespace where they would otherwise appear. If
  whole include files are never meant to be included from outside they should
  be in a subdirectory called `detail`.
* All files have suffix `.hpp`.
* Closing } of all classes and namespaces should have a trailing comment
  with the name of the class/namespace.
* All constructors with one (or more arguments if they have a default) should
  be declared "explicit" unless there is a reason for them not to be. Document
  that reason.
* If a class has any of the special methods (copy/move constructor/assigment,
  destructor) it should have all of them, possibly marking them as default or
  deleted.
* Typedefs have `names_like_this_type` which end in `_type`. Typedefs should
  use the new `using foo_type = bar` syntax instead of the old
  `typedef bar foo_type`.
* Template parameters are single uppercase letters or start with uppercase `T`
  and use CamelCase.
* Always use `typename` in templates, not `class`: `template <typename T>`.
* The ellipsis in a variadic template never has a space to the left of it and
  always has a space to the right: `template <typename... TArgs>` etc.

Keep to the indentation and other styles used in the code.


## C++11

Osmium uses C++11 and you can use its features such as auto, lambdas,
threading, etc. There are a few features we do not use, because even modern
compilers don't support them yet. This list might change as we get more data
about which compilers support which feature and what operating system versions
or distributions have which versions of these compilers installed.

Use `include/osmium/util/compatibility.hpp` if there are compatibility problems
between compilers due to different C++11 support.


## Operating systems

Usually all code must work on Linux, OSX, and Windows. Execptions are allowed
for some minor functionality, but please discuss this first.

When writing code and tests, care must be taken that everything works with the
CR line ending convention used on Linux and OSX and the CRLF line ending used
on Windows. Note that `git` can be run with different settings regarding line
ending rewritings on different machines making things more difficult. Some
files have been "forced" to LF line endings using `.gitattributes` files.


## 32bit systems

Libosmium tries to work on 32bit systems whereever possible. But there are
some parts which will not work on 32bit systems, mainly because the amount
of main memory available is not enough for it to work anyway.


## Checking your code

The Osmium makefiles use pretty draconian warning options for the compiler.
This is good. Code MUST never produce any warnings, even with those settings.
If absolutely necessary pragmas can be used to disable certain warnings in
specific areas of the code.

If the static code checker `cppcheck` is installed, the CMake configuration
will add a new build target `cppcheck` that will check all `.cpp` and `.hpp`
files. Cppcheck finds some bugs that gcc/clang doesn't. But take the result
with a grain of salt, it also sometimes produces wrong warnings.

Set `BUILD_HEADERS=ON` in the CMake config to enable compiling all include
files on their own to check whether dependencies are all okay. All include
files MUST include all other include files they depend on.

Call `cmake/iwyu.sh` to check for proper includes and forward declarations.
This uses the clang-based `include-what-you-use` program. Note that it does
produce some false reports and crashes often. The `osmium.imp` file can be
used to define mappings for iwyu. See the IWYU tool at
<https://include-what-you-use.org/>.


## Testing

There are unit tests using the Catch Unit Test Framework in the `test`
directory, some data tests in `test/osm-testdata` and tests of the examples in
`test/examples`. They are built by the default cmake config. Run `ctest` to
run them. We can always use more tests.

Tests are run automatically using the Travis (Linux/Mac) and Appveyor (Windows)
services. We automatically create coverage reports on Codevoc.io. Note that
the coverage percentages reported are not always accurate, because code that
is not used in tests at all will not necessarily end up in the binary and
the code coverage tool will not know it is there.

[![Travis Build Status](https://secure.travis-ci.org/osmcode/libosmium.svg)](https://travis-ci.org/osmcode/libosmium)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/osmcode/libosmium?svg=true)](https://ci.appveyor.com/project/Mapbox/libosmium)
[![Coverage Status](https://codecov.io/gh/osmcode/libosmium/branch/master/graph/badge.svg)](https://codecov.io/gh/osmcode/libosmium)


## Documenting the code

All namespaces, classes, functions, attributes, etc. should be documented.

Osmium uses the [Doxygen](http://www.doxygen.org) source code documentation
system. If it is installed, the CMake configuration will add a new build
target, so you can build it with `make doc`.

