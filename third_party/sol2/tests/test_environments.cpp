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

TEST_CASE("environments/get", "Envronments can be taken out of things like Lua functions properly") {
	sol::state lua;
	sol::stack_guard luasg(lua);
	lua.open_libraries(sol::lib::base);

	auto result1 = lua.safe_script("f = function() return test end", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::function f = lua["f"];

	sol::environment env_f(lua, sol::create);
	env_f["test"] = 31;
	sol::set_environment(env_f, f);

	int result = f();
	REQUIRE(result == 31);

	auto result2 = lua.safe_script("g = function() test = 5 end", sol::script_pass_on_error);
	REQUIRE(result2.valid());
	sol::function g = lua["g"];
	sol::environment env_g(lua, sol::create);
	env_g.set_on(g);

	g();

	int test = env_g["test"];
	REQUIRE(test == 5);

	sol::object global_test = lua["test"];
	REQUIRE(!global_test.valid());

	auto result3 = lua.safe_script("h = function() end", sol::script_pass_on_error);
	REQUIRE(result3.valid());

	lua.set_function("check_f_env",
		[&lua, &env_f](sol::object target) {
			sol::stack_guard sg(lua);
			sol::environment target_env(sol::env_key, target);
			int test_env_f = env_f["test"];
			int test_target_env = target_env["test"];
			REQUIRE(test_env_f == test_target_env);
			REQUIRE(test_env_f == 31);
			REQUIRE(env_f == target_env);
		});
	lua.set_function("check_g_env",
		[&lua, &env_g](sol::function target) {
			sol::stack_guard sg(lua);
			sol::environment target_env = sol::get_environment(target);
			int test_env_g = env_g["test"];
			int test_target_env = target_env["test"];
			REQUIRE(test_env_g == test_target_env);
			REQUIRE(test_env_g == 5);
			REQUIRE(env_g == target_env);
		});
	lua.set_function("check_h_env",
		[&lua](sol::function target) {
			sol::stack_guard sg(lua);
			sol::environment target_env = sol::get_environment(target);
		});

	auto checkf = lua.safe_script("check_f_env(f)");
	REQUIRE(checkf.valid());
	auto checkg = lua.safe_script("check_g_env(g)");
	REQUIRE(checkg.valid());
	auto checkh = lua.safe_script("check_h_env(h)");
	REQUIRE(checkh.valid());
}

TEST_CASE("environments/shadowing", "Environments can properly shadow and fallback on variables") {

	sol::state lua;
	lua["b"] = 2142;

	SECTION("no fallback") {
		sol::environment plain_env(lua, sol::create);
		auto result1 = lua.safe_script("a = 24", plain_env, sol::script_pass_on_error);
		REQUIRE(result1.valid());
		sol::optional<int> maybe_env_a = plain_env["a"];
		sol::optional<int> maybe_global_a = lua["a"];
		sol::optional<int> maybe_env_b = plain_env["b"];
		sol::optional<int> maybe_global_b = lua["b"];

		REQUIRE(maybe_env_a != sol::nullopt);
		REQUIRE(maybe_env_a.value() == 24);
		REQUIRE(maybe_env_b == sol::nullopt);

		REQUIRE(maybe_global_a == sol::nullopt);
		REQUIRE(maybe_global_b != sol::nullopt);
		REQUIRE(maybe_global_b.value() == 2142);
	}
	SECTION("fallback") {
		sol::environment env_with_fallback(lua, sol::create, lua.globals());
		auto result1 = lua.safe_script("a = 56", env_with_fallback, sol::script_pass_on_error);
		REQUIRE(result1.valid());
		sol::optional<int> maybe_env_a = env_with_fallback["a"];
		sol::optional<int> maybe_global_a = lua["a"];
		sol::optional<int> maybe_env_b = env_with_fallback["b"];
		sol::optional<int> maybe_global_b = lua["b"];

		REQUIRE(maybe_env_a != sol::nullopt);
		REQUIRE(maybe_env_a.value() == 56);
		REQUIRE(maybe_env_b != sol::nullopt);
		REQUIRE(maybe_env_b.value() == 2142);

		REQUIRE(maybe_global_a == sol::nullopt);
		REQUIRE(maybe_global_b != sol::nullopt);
		REQUIRE(maybe_global_b.value() == 2142);
	}
	SECTION("from name") {
		sol::environment env_with_fallback(lua, sol::create, lua.globals());
		lua["env"] = env_with_fallback;
		sol::environment env = lua["env"];
		auto result1 = lua.safe_script("a = 56", env, sol::script_pass_on_error);
		REQUIRE(result1.valid());
		sol::optional<int> maybe_env_a = env["a"];
		sol::optional<int> maybe_global_a = lua["a"];
		sol::optional<int> maybe_env_b = env["b"];
		sol::optional<int> maybe_global_b = lua["b"];

		REQUIRE(maybe_env_a != sol::nullopt);
		REQUIRE(maybe_env_a.value() == 56);
		REQUIRE(maybe_env_b != sol::nullopt);
		REQUIRE(maybe_env_b.value() == 2142);

		REQUIRE(maybe_global_a == sol::nullopt);
		REQUIRE(maybe_global_b != sol::nullopt);
		REQUIRE(maybe_global_b.value() == 2142);
	}
	SECTION("name with newtable") {
		lua["blank_env"] = sol::new_table(0, 1);
		sol::environment plain_env = lua["blank_env"];
		auto result1 = lua.safe_script("a = 24", plain_env, sol::script_pass_on_error);
		REQUIRE(result1.valid());

		sol::optional<int> maybe_env_a = plain_env["a"];
		sol::optional<int> maybe_global_a = lua["a"];
		sol::optional<int> maybe_env_b = plain_env["b"];
		sol::optional<int> maybe_global_b = lua["b"];

		REQUIRE(maybe_env_a != sol::nullopt);
		REQUIRE(maybe_env_a.value() == 24);
		REQUIRE(maybe_env_b == sol::nullopt);

		REQUIRE(maybe_global_a == sol::nullopt);
		REQUIRE(maybe_global_b != sol::nullopt);
		REQUIRE(maybe_global_b.value() == 2142);
	}
}

TEST_CASE("environments/functions", "see if environments on functions are working properly") {

	SECTION("basic") {
		sol::state lua;

		auto result1 = lua.safe_script("a = function() return 5 end", sol::script_pass_on_error);
		REQUIRE(result1.valid());

		sol::function a = lua["a"];

		int result0 = a();
		REQUIRE(result0 == 5);

		sol::environment env(lua, sol::create);
		sol::set_environment(env, a);

		int value = a();
		REQUIRE(value == 5);
	}
	SECTION("return environment value") {
		sol::state lua;

		auto result1 = lua.safe_script("a = function() return test end", sol::script_pass_on_error);
		REQUIRE(result1.valid());

		sol::function a = lua["a"];
		sol::environment env(lua, sol::create);
		env["test"] = 5;
		env.set_on(a);

		// the function returns the value from the environment table
		int result = a();
		REQUIRE(result == 5);
	}

	SECTION("set environment value") {
		sol::state lua;
		auto result1 = lua.safe_script("a = function() test = 5 end", sol::script_pass_on_error);
		REQUIRE(result1.valid());

		sol::function a = lua["a"];
		sol::environment env(lua, sol::create);
		sol::set_environment(env, a);

		a();

		// the value can be retrieved from the env table
		int result = env["test"];
		REQUIRE(result == 5);

		// the global environment is not polluted
		auto gtest = lua["test"];
		REQUIRE(!gtest.valid());
	}
}

TEST_CASE("environments/this_environment", "test various situations of pulling out an environment") {
	static std::string code = "return (f(10))";

	sol::state lua;

	lua["f"] = [](sol::this_environment te, int x, sol::this_state ts) {
		if (te) {
			sol::environment& env = te;
			return x + static_cast<int>(env["x"]);
		}
		sol::state_view lua = ts;
		return x + static_cast<int>(lua["x"]);
	};

	sol::environment e(lua, sol::create, lua.globals());
	lua["x"] = 5;
	e["x"] = 20;
	SECTION("from Lua script") {
		auto result1 = lua.safe_script(code, e, sol::script_pass_on_error);
		REQUIRE(result1.valid());
		int value = result1;
		REQUIRE(value == 30);
	}
	SECTION("from C++") {
		sol::function f = lua["f"];
		e.set_on(f);
		int value = f(10);
		REQUIRE(value == 30);
	}
	SECTION("from C++, with no env") {
		sol::function f = lua["f"];
		int value = f(10);
		REQUIRE(value == 15);
	}
}
