// sol2 

// The MIT License (MIT)

// Copyright (c) 2013-2018 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "test_sol.hpp"

#include <catch.hpp>

#include <iostream>

TEST_CASE("proxy/function results", "make sure that function results return proper proxies and can be indexed nicely") {
	sol::state lua;
	SECTION("unsafe_function_result") {
		auto ufr = lua.script("return 1, 2, 3, 4");
		int accum = 0;
		for (const auto& r : ufr) {
			int v = r;
			accum += v;
		}
		REQUIRE(accum == 10);
	}
	SECTION("protected_function_result") {
		auto pfr = lua.safe_script("return 1, 2, 3, 4");
		int accum = 0;
		for (const auto& r : pfr) {
			int v = r;
			accum += v;
		}
		REQUIRE(accum == 10);
	}
}

TEST_CASE("proxy/optional conversion", "make sure optional conversions out of a table work properly") {
	sol::state state{};
	sol::table table = state.create_table_with("func", 42);
	sol::optional<sol::function> func = table["func"];
	REQUIRE(func == sol::nullopt);
}

TEST_CASE("proxy/proper-pushing", "allow proxies to reference other proxies and be serialized as the proxy itself and not a function or something") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::io);

	class T {};
	lua.new_usertype<T>("T");

	T t;
	lua["t1"] = &t;
	lua["t2"] = lua["t1"];
	lua.safe_script("b = t1 == t2");
	bool b = lua["b"];
	REQUIRE(b);
}

TEST_CASE("proxy/equality", "check to make sure equality tests work") {
	sol::state lua;
#ifndef __clang__
	REQUIRE((lua["a"] == sol::lua_nil));
	REQUIRE((lua["a"] == nullptr));
	REQUIRE_FALSE((lua["a"] != sol::lua_nil));
	REQUIRE_FALSE((lua["a"] != nullptr));
	REQUIRE_FALSE((lua["a"] == 0));
	REQUIRE_FALSE((lua["a"] == 2));
	REQUIRE((lua["a"] != 0));
	REQUIRE((lua["a"] != 2));
#endif // clang screws up by trying to access int128 types that it doesn't support, even when we don't ask for them

	lua["a"] = 2;

#ifndef __clang__
	REQUIRE_FALSE((lua["a"] == sol::lua_nil));
	REQUIRE_FALSE((lua["a"] == nullptr));
	REQUIRE((lua["a"] != sol::lua_nil));
	REQUIRE((lua["a"] != nullptr));
	REQUIRE_FALSE((lua["a"] == 0));
	REQUIRE((lua["a"] == 2));
	REQUIRE((lua["a"] != 0));
	REQUIRE_FALSE((lua["a"] != 2));
#endif // clang screws up by trying to access int128 types that it doesn't support, even when we don't ask for them
}
