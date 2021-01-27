## Sol 2.20

[![Join the chat in Discord: https://discord.gg/buxkYNT](https://img.shields.io/badge/Discord-Chat!-brightgreen.svg)](https://discord.gg/buxkYNT)

[![Linux & Max OSX Build Status](https://travis-ci.org/ThePhD/sol2.svg?branch=develop)](https://travis-ci.org/ThePhD/sol2)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/n38suofr21e9uk7h?svg=true)](https://ci.appveyor.com/project/ThePhD/sol2)
[![Documentation Status](https://readthedocs.org/projects/sol2/badge/?version=latest)](http://sol2.readthedocs.io/en/latest/?badge=latest)

Sol is a C++ library binding to Lua. It currently supports all Lua versions 5.1+ (LuaJIT 2.x included). Sol aims to be easy to use and easy to add to a project.
The library is header-only for easy integration with projects.

## Documentation

Find it [here](http://sol2.rtfd.io/). A run-through kind of tutorial is [here](http://sol2.readthedocs.io/en/latest/tutorial/all-the-things.html)! The API documentation goes over most cases (particularly, the "api/usertype" and "api/proxy" and "api/function" sections) that should still get you off your feet and going, and there's an examples directory [here](https://github.com/ThePhD/sol2/tree/develop/examples) as well.

## Sneak Peek

```cpp
#include <sol/sol.hpp>
#include <cassert>

int main() {
    sol::state lua;
    int x = 0;
    lua.set_function("beep", [&x]{ ++x; });
    lua.script("beep()");
    assert(x == 1);
}
```

```cpp
#include <sol/sol.hpp>
#include <cassert>

struct vars {
    int boop = 0;
};

int main() {
    sol::state lua;
    lua.new_usertype<vars>("vars", "boop", &vars::boop);
    lua.script("beep = vars.new()\n"
               "beep.boop = 1");
    assert(lua.get<vars>("beep").boop == 1);
}
```

More examples are given in the examples directory [here](https://github.com/ThePhD/sol2/tree/develop/examples). 

## Supporting

Help the project grow on [patreon](https://www.patreon.com/thephd)!

You can also [donate to support Sol](https://www.paypal.me/LMeneide), which is always appreciated! There are reward tiers for patrons on patreon, too!

You can also help out the library by submitting pull requests to fix anything or add anything you think would be helpful! This includes making small, useful examples of something you haven't seen, or fixing typos and bad code in the documentation.

## Presentations

"A Sun For the Moon - A Zero-Overhead Lua Abstraction using C++"  
ThePhD
Lua Workshop 2016 - Mashape, San Francisco, CA  
[Deck](https://github.com/ThePhD/sol2/blob/develop/docs/presentations/2016.10.14%20-%20ThePhD%20-%20No%20Overhead%20C%20Abstraction.pdf)

"Wrapping Lua C in C++ - Efficiently, Nicely, and with a Touch of Magic"  
ThePhD
Boston C++ Meetup November 2017 - CiC (Milk Street), Boston, MA  
[Deck](https://github.com/ThePhD/sol2/blob/develop/docs/presentations/2017.11.08%20-%20ThePhD%20-%20Wrapping%20Lua%20C%20in%20C%2B%2B.pdf)

"Biting the CMake Bullet"  
ThePhD
Boston C++ Meetup February 2018 - CiC (Main Street), Cambridge, MA  
[Deck](https://github.com/ThePhD/sol2/blob/develop/docs/presentations/2018.02.06%20-%20ThePhD%20-%20Biting%20the%20CMake%20Bullet.pdf)

"Compile Fast, Run Faster, Scale Forever: A look into the sol2 Library"  
ThePhD
C++Now 2018 - Hudson Commons, Aspen Physics Center, Aspen, Colorado  
[Deck](https://github.com/ThePhD/sol2/blob/develop/docs/presentations/2018.05.10%20-%20ThePhD%20-%20Compile%20Fast%2C%20Run%20Faster%2C%20Scale%20Forever.pdf)

"Scripting at the Speed of Thought: Using Lua in C++ with sol3"  
ThePhD
CppCon 2018 - 404 Keystone, Meydenbauer Center, Aspen, Colorado  
[Deck](https://github.com/ThePhD/sol2/blob/develop/docs/presentations/2018.09.28%20-%20ThePhD%20-%20Scripting%20at%20the%20Speed%20of%20Thought.pdf)

## Creating a single header

You can grab a single header (and the single forward header) out of the library [here](https://github.com/ThePhD/sol2/tree/develop/single/sol). For stable version, check the releases tab on github for a provided single header file for maximum ease of use. A script called `single.py` is provided in the repository if there's some bleeding edge change that hasn't been published on the releases page. You can run this script to create a single file version of the library so you can only include that part of it. Check `single.py --help` for more info.

If you use CMake, you can also configure and generate a project that will generate the sol2_single_header for you. You can also include the project using Cmake. Run CMake for more details. Thanks @Nava2, @alkino, @mrgreywater and others for help with making the CMake build a reality.

## Features

- [Fastest in the land](http://sol2.readthedocs.io/en/latest/benchmarks.html) (see: sol bar in graph).
- Supports retrieval and setting of multiple types including: 
  * `std::string`, `std::wstring`, `std::u16string` and `std::u32string` support (and for views).
  * understands and works with containers such as `std::map/unordered_map`, c-style arrays, vectors, non-standard custom containers and more.
  * user-defined types, with or **without** registering that type 
  * `std::unique_ptr`, `std::shared_ptr`, and optional support of other pointer types like `boost::shared_ptr`.
  * custom `optional<T>` that works with references.
  * C++17 support for variants and similar new types.
- Lambda, function, and member function bindings are supported.
- Intermediate type for checking if a variable exists.
- Simple API that completely abstracts away the C stack API, including `protected_function` with the ability to use an error-handling function.
- `operator[]`-style manipulation of tables
- C++ type representations in lua userdata as `usertype`s with guaranteed cleanup.
- Customization points to allow your C++ objects to be pushed and retrieved from Lua as multiple consecutive objects, or anything else you desire!
- Overloaded function calls: `my_function(1); my_function("Hello")` in the same lua script route to different function calls based on parameters
- Support for tables, nested tables, table iteration with `table.for_each` / `begin()` and `end()` iterators.
- Zero overhead for usertype function call lookup when using `SOL_USE_BOOST`, safe for critical applications

## Supported Compilers

Sol makes use of C++11 **and** C++14 features. GCC 5.x.x and Clang 3.6.x (with `-std=c++1z` and appropriate standard library) 
or higher should be able to compile without problems. However, the officially supported and CI-tested compilers are:

- GCC 5.x.x+ (MinGW 5.x.x+)
- Clang 3.6.x+
- Visual Studio 2015 Community (Visual C++ 14.0)+

Please make sure you use the `-std=c++1y`, `-std=c++14`, `-std=c++1z`, `-std=c++17` or better standard flags 
(some of these flags are the defaults in later versions of GCC, such as 6+ and better).

Older compilers (GCC 4.9.x, Clang 3.4.x seem to be the lowest) can work with versions as late 
as [v2.17.5](https://github.com/ThePhD/sol2/releases/tag/v2.17.5), with the flag `-std=c++14` or `-std=c++1y`.

sol2 is checked by-hand for other platforms as well, including Android-based builds with GCC and iOS-based builds out of XCode with Apple-clang. It should work on both of these platforms, so long as you have the proper standards flags.

## Running the Tests

Testing on Travis-CI and Appveyor use CMake. You can generate the tests by running CMake and configuring `TESTS`, `TESTS_SINGLE`, `TESTS_EXAMPLES`, and `EXAMPLES` to be on. Make sure `SINGLE` is also on.

You will need any flavor of python3 and an available compiler. The testing suite will build its own version of Lua and LuaJIT, so you do not have to.

## License

Sol is distributed with an MIT License. You can see LICENSE.txt for more info.
