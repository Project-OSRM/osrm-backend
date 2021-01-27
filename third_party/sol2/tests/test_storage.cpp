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

#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1

#include <sol.hpp>

#include <catch.hpp>

TEST_CASE("storage/registry construction", "ensure entries from the registry can be retrieved") {
	const auto& code = R"(
function f()
    return 2
end
)";

	sol::state lua;
	{
		auto r = lua.safe_script(code, sol::script_pass_on_error);
		REQUIRE(r.valid());
	}
	sol::function f = lua["f"];
	sol::reference r = lua["f"];
	sol::function regf(lua, f);
	sol::reference regr(lua, sol::ref_index(f.registry_index()));
	bool isequal = f == r;
	REQUIRE(isequal);
	isequal = f == regf;
	REQUIRE(isequal);
	isequal = f == regr;
	REQUIRE(isequal);
}

TEST_CASE("storage/registry construction empty", "ensure entries from the registry can be retrieved") {
	sol::state lua;
	sol::function f = lua["f"];
	sol::reference r = lua["f"];
	sol::function regf(lua, f);
	sol::reference regr(lua, sol::ref_index(f.registry_index()));
	bool isequal = f == r;
	REQUIRE(isequal);
	isequal = f == regf;
	REQUIRE(isequal);
	isequal = f == regr;
	REQUIRE(isequal);
}

TEST_CASE("storage/main thread", "ensure round-tripping and pulling out thread data even on 5.1 with a backup works") {
	sol::state lua;
	{
		sol::stack_guard g(lua);
		lua_State* orig = lua;
		lua_State* ts = sol::main_thread(lua, lua);
		REQUIRE(ts == orig);
	}
	{
		sol::stack_guard g(lua);
		lua_State* orig = lua;
		lua_State* ts = sol::main_thread(lua);
		REQUIRE(ts == orig);
	}
}
