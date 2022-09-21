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

TEST_CASE("issues/stack overflow", "make sure various operations repeated don't trigger stack overflow") {
	sol::state lua;
	lua.safe_script("t = {};t[0]=20");
	lua.safe_script("lua_function=function(i)return i;end");

	sol::function f = lua["lua_function"];
	std::string teststring = "testtext";
	REQUIRE_NOTHROW([&] {
		for (int i = 0; i < 1000000; ++i) {
			std::string result = f(teststring);
			if (result != teststring)
				throw std::logic_error("RIP");
		}
	}());
	sol::table t = lua["t"];
	int expected = 20;
	REQUIRE_NOTHROW([&] {
		for (int i = 0; i < 1000000; ++i) {
			int result = t[0];
			t.size();
			if (result != expected)
				throw std::logic_error("RIP");
		}
	}());
}

TEST_CASE("issues/stack overflow 2", "make sure basic iterators clean up properly when they're not iterated through (e.g., with empty())") {
	sol::state lua;
	sol::table t = lua.create_table_with(1, "wut");
	int MAX = 50000;
	auto fx = [&]() {
		int a = 50;
		for (int i = 0; i < MAX; ++i) {
			if (t.empty()) {
				a += 4;
			}
			a += 2;
		}
	};
	REQUIRE_NOTHROW(fx());
}
