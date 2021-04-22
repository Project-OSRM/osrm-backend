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

#include <unordered_map>
#include <vector>

struct two_things {
	int a;
	bool b;
};

struct number_shim {
	double num = 0;
};

namespace sol {

	// First, the expected size
	// Specialization of a struct
	template <>
	struct lua_size<two_things> : std::integral_constant<int, 2> {};

	// Then, the expected type
	template <>
	struct lua_type_of<two_things> : std::integral_constant<sol::type, sol::type::poly> {};

	// do note specialize size for this because it is our type
	template <>
	struct lua_type_of<number_shim> : std::integral_constant<sol::type, sol::type::poly> {};

	// Now, specialize various stack structures
	namespace stack {

		template <>
		struct checker<two_things> {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				// Check first and second second index for being the proper types
				bool success = stack::check<int>(L, index, handler)
					&& stack::check<bool>(L, index + 1, handler);
				tracking.use(2);
				return success;
			}
		};

		template <>
		struct getter<two_things> {
			static two_things get(lua_State* L, int index, record& tracking) {
				// Get the first element
				int a = stack::get<int>(L, index);
				// Get the second element,
				// in the +1 position from the first
				bool b = stack::get<bool>(L, index + 1);
				// we use 2 slots, each of the previous takes 1
				tracking.use(2);
				return two_things{ a, b };
			}
		};

		template <>
		struct pusher<two_things> {
			static int push(lua_State* L, const two_things& things) {
				int amount = stack::push(L, things.a);
				amount += stack::push(L, things.b);
				// Return 2 things
				return amount;
			}
		};

		template <>
		struct checker<number_shim> {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				// check_usertype is a backdoor for directly checking sol2 usertypes
				if (!check_usertype<number_shim>(L, index) && !stack::check<double>(L, index)) {
					handler(L, index, type_of(L, index), type::userdata, "expected a number_shim or a number");
					return false;
				}
				tracking.use(1);
				return true;
			}
		};

		template <>
		struct getter<number_shim> {
			static number_shim get(lua_State* L, int index, record& tracking) {
				if (check_usertype<number_shim>(L, index)) {
					number_shim& ns = get_usertype<number_shim>(L, index, tracking);
					return ns;
				}
				number_shim ns{};
				ns.num = stack::get<double>(L, index, tracking);
				return ns;
			}
		};

	} // namespace stack
} // namespace sol

TEST_CASE("customization/split struct", "using the newly documented customization points to handle different kinds of classes") {
	sol::state lua;

	// Create a pass-through style of function
	auto result1 = lua.safe_script("function f ( a, b, c ) return a + c, b end");
	REQUIRE(result1.valid());
	lua.set_function("g", [](int a, bool b, int c, double d) {
		return std::make_tuple(a + c, b, d + 2.5);
	});

	// get the function out of Lua
	sol::function f = lua["f"];
	sol::function g = lua["g"];

	two_things thingsf = f(two_things{ 24, true }, 1);
	two_things thingsg;
	double d;
	sol::tie(thingsg, d) = g(two_things{ 25, false }, 2, 34.0);
	REQUIRE(thingsf.a == 25);
	REQUIRE(thingsf.b);

	REQUIRE(thingsg.a == 27);
	REQUIRE_FALSE(thingsg.b);
	REQUIRE(d == 36.5);
}

TEST_CASE("customization/get_ check_usertype", "using the newly documented customization points to handle different kinds of classes") {
	sol::state lua;

	// Create a pass-through style of function
	auto result1 = lua.safe_script("function f ( a ) return a end");
	REQUIRE(result1.valid());
	lua.set_function("g", [](double a) {
		number_shim ns;
		ns.num = a;
		return ns;
	});

	auto result2 = lua.safe_script("vf = f(25) vg = g(35)", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	number_shim thingsf = lua["vf"];
	number_shim thingsg = lua["vg"];
	
	REQUIRE(thingsf.num == 25);
	REQUIRE(thingsg.num == 35);
}
