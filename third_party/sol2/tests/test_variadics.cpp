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

#include <deque>
#include <set>
#include <functional>
#include <string>

TEST_CASE("variadics/variadic_args", "Check to see we can receive multiple arguments through a variadic") {
	struct structure {
		int x;
		bool b;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.set_function("v", [](sol::this_state, sol::variadic_args va) -> structure {
		int r = 0;
		for (auto v : va) {
			int value = v;
			r += value;
		}
		return { r, r > 200 };
	});

	lua.safe_script("x = v(25, 25)");
	lua.safe_script("x2 = v(25, 25, 100, 50, 250, 150)");
	lua.safe_script("x3 = v(1, 2, 3, 4, 5, 6)");

	structure& lx = lua["x"];
	structure& lx2 = lua["x2"];
	structure& lx3 = lua["x3"];
	REQUIRE(lx.x == 50);
	REQUIRE(lx2.x == 600);
	REQUIRE(lx3.x == 21);
	REQUIRE_FALSE(lx.b);
	REQUIRE(lx2.b);
	REQUIRE_FALSE(lx3.b);
}

TEST_CASE("variadics/required with variadic_args", "Check if a certain number of arguments can still be required even when using variadic_args") {
	sol::state lua;
	lua.set_function("v",
		[](sol::this_state, sol::variadic_args, int, int) {
		});
	{
		auto result = lua.safe_script("v(20, 25, 30)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("v(20, 25)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("v(20)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("variadics/variadic_args get type", "Make sure we can inspect types proper from variadic_args") {
	sol::state lua;

	lua.set_function("f", [](sol::variadic_args va) {
		sol::type types[] = {
			sol::type::number,
			sol::type::string,
			sol::type::boolean
		};
		bool working = true;
		auto b = va.begin();
		for (std::size_t i = 0; i < va.size(); ++i, ++b) {
			sol::type t1 = va.get_type(i);
			sol::type t2 = b->get_type();
			working &= types[i] == t1;
			working &= types[i] == t2;
		}
		REQUIRE(working);
	});

	lua.safe_script("f(1, 'bark', true)");
	lua.safe_script("f(2, 'wuf', false)");
}

TEST_CASE("variadics/variadic_results", "returning a variable amount of arguments from C++") {
	SECTION("as_returns - containers") {
		sol::state lua;

		lua.set_function("f", []() {
			std::set<std::string> results{ "arf", "bark", "woof" };
			return sol::as_returns(std::move(results));
		});
		lua.set_function("g", []() {
			static const std::deque<int> results{ 25, 82 };
			return sol::as_returns(std::ref(results));
		});

		REQUIRE_NOTHROW([&]() {
			lua.safe_script(R"(
	v1, v2, v3 = f()
	v4, v5 = g()
)");
		}());

		std::string v1 = lua["v1"];
		std::string v2 = lua["v2"];
		std::string v3 = lua["v3"];
		int v4 = lua["v4"];
		int v5 = lua["v5"];

		REQUIRE(v1 == "arf");
		REQUIRE(v2 == "bark");
		REQUIRE(v3 == "woof");
		REQUIRE(v4 == 25);
		REQUIRE(v5 == 82);
	}
	SECTION("variadic_results - variadic_args") {
		sol::state lua;

		lua.set_function("f", [](sol::variadic_args args) {
			return sol::variadic_results(args.cbegin(), args.cend());
		});

		REQUIRE_NOTHROW([&]() {
			lua.safe_script(R"(
	v1, v2, v3 = f(1, 'bark', true)
	v4, v5 = f(25, 82)
)");
		}());

		int v1 = lua["v1"];
		std::string v2 = lua["v2"];
		bool v3 = lua["v3"];
		int v4 = lua["v4"];
		int v5 = lua["v5"];

		REQUIRE(v1 == 1);
		REQUIRE(v2 == "bark");
		REQUIRE(v3);
		REQUIRE(v4 == 25);
		REQUIRE(v5 == 82);
	}
	SECTION("variadic_results") {
		sol::state lua;

		lua.set_function("f", [](sol::this_state ts, bool maybe) {
			if (maybe) {
				sol::variadic_results vr;
				vr.push_back({ ts, sol::in_place, 1 });
				vr.push_back({ ts, sol::in_place, 2 });
				vr.insert(vr.cend(), { ts, sol::in_place, 3 });
				return vr;
			}
			else {
				sol::variadic_results vr;
				vr.push_back({ ts, sol::in_place, "bark" });
				vr.push_back({ ts, sol::in_place, "woof" });
				vr.insert(vr.cend(), { ts, sol::in_place, "arf" });
				vr.push_back({ ts, sol::in_place, "borf" });
				return vr;
			}
		});

		REQUIRE_NOTHROW([&]() {
			lua.safe_script(R"(
	v1, v2, v3 = f(true)
	v4, v5, v6, v7 = f(false)
)");
		}());

		int v1 = lua["v1"];
		int v2 = lua["v2"];
		int v3 = lua["v3"];
		std::string v4 = lua["v4"];
		std::string v5 = lua["v5"];
		std::string v6 = lua["v6"];
		std::string v7 = lua["v7"];

		REQUIRE(v1 == 1);
		REQUIRE(v2 == 2);
		REQUIRE(v3 == 3);
		REQUIRE(v4 == "bark");
		REQUIRE(v5 == "woof");
		REQUIRE(v6 == "arf");
		REQUIRE(v7 == "borf");
	}
}

TEST_CASE("variadics/fallback_constructor", "ensure constructor matching behaves properly in the presence of variadic fallbacks") {
	struct vec2 {
		float x = 0, y = 0;
	};

	sol::state lua;

	lua.new_simple_usertype<vec2>("vec2",
		sol::call_constructor, sol::factories([]() { return vec2{}; }, [](vec2 const& v) -> vec2 { return v; }, [](sol::variadic_args va) {
		vec2 res{};
		if (va.size() == 1) {
			res.x = va[0].get<float>();
			res.y = va[0].get<float>();
		}
		else if (va.size() == 2) {
			res.x = va[0].get<float>();
			res.y = va[1].get<float>();
		}
		else {
			throw sol::error("invalid args");
		}
		return res; }));

	REQUIRE_NOTHROW([&]() {
		lua.safe_script("v0 = vec2();");
		lua.safe_script("v1 = vec2(1);");
		lua.safe_script("v2 = vec2(1, 2);");
		lua.safe_script("v3 = vec2(v2)");
	}());

	vec2& v0 = lua["v0"];
	vec2& v1 = lua["v1"];
	vec2& v2 = lua["v2"];
	vec2& v3 = lua["v3"];

	REQUIRE(v0.x == 0);
	REQUIRE(v0.y == 0);
	REQUIRE(v1.x == 1);
	REQUIRE(v1.y == 1);
	REQUIRE(v2.x == 1);
	REQUIRE(v2.y == 2);
	REQUIRE(v3.x == v2.x);
	REQUIRE(v3.y == v2.y);
}
