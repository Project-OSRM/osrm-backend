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
#include <algorithm>
#include <numeric>
#include <vector>

std::string free_function() {
	INFO("free_function()");
	return "test";
}

struct object {
	std::string operator()() {
		INFO("member_test()");
		return "test";
	}
};

int plop_xyz(int x, int y, std::string z) {
	INFO(x << " " << y << " " << z);
	return 11;
}

TEST_CASE("tables/as enums", "Making sure enums can be put in and gotten out as values") {
	enum direction {
		up,
		down,
		left,
		right
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua["direction"] = lua.create_table_with(
		"up", direction::up,
		"down", direction::down,
		"left", direction::left,
		"right", direction::right);

	sol::object obj = lua["direction"]["up"];
	bool isdir = obj.is<direction>();
	REQUIRE(isdir);
	auto dir = obj.as<direction>();
	REQUIRE(dir == direction::up);
}

TEST_CASE("tables/as enum classes", "Making sure enums can be put in and gotten out as values") {
	enum class direction {
		up,
		down,
		left,
		right
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua["direction"] = lua.create_table_with(
		"up", direction::up,
		"down", direction::down,
		"left", direction::left,
		"right", direction::right);

	sol::object obj = lua["direction"]["up"];
	bool isdir = obj.is<direction>();
	REQUIRE(isdir);
	auto dir = obj.as<direction>();
	REQUIRE(dir == direction::up);
}

TEST_CASE("tables/cleanup", "make sure tables leave the stack balanced") {
	sol::state lua;
	lua.open_libraries();

	auto f = [] { return 5; };
	for (int i = 0; i < 30; i++) {
		std::string name = std::string("init") + std::to_string(i);
		int top = lua_gettop(lua);
		lua[name] = f;
		int aftertop = lua_gettop(lua);
		REQUIRE(aftertop == top);
		int val = lua[name]();
		REQUIRE(val == 5);
	}
}

TEST_CASE("tables/nested cleanup", "make sure tables leave the stack balanced") {
	sol::state lua;
	lua.open_libraries();

	lua.safe_script("A={}");
	auto f = [] { return 5; };
	for (int i = 0; i < 30; i++) {
		std::string name = std::string("init") + std::to_string(i);
		int top = lua_gettop(lua);
		auto A = lua["A"];
		int beforetop = lua_gettop(lua);
		REQUIRE(beforetop == top);
		A[name] = f;
		int aftertop = lua_gettop(lua);
		REQUIRE(aftertop == top);
		int val = A[name]();
		REQUIRE(val == 5);
	}
}

TEST_CASE("tables/new_enum", "Making sure enums can be put in and gotten out as values") {
	enum class direction {
		up,
		down,
		left,
		right
	};

	SECTION("variadics") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_enum("direction",
			"up", direction::up,
			"down", direction::down,
			"left", direction::left,
			"right", direction::right);

		direction d = lua["direction"]["left"];
		REQUIRE(d == direction::left);
		auto result = lua.safe_script("direction.left = 50", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
		d = lua["direction"]["left"];
		REQUIRE(d == direction::left);
	}
	SECTION("initializer_list") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_enum<direction>("direction", { { "up", direction::up }, { "down", direction::down }, { "left", direction::left }, { "right", direction::right } });

		direction d = lua["direction"]["left"];
		REQUIRE(d == direction::left);
		auto result = lua.safe_script("direction.left = 50", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
		d = lua["direction"]["left"];
		REQUIRE(d == direction::left);
	}
}

TEST_CASE("tables/for_each", "Testing the use of for_each to get values from a lua table") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.safe_script(
		"arr = {\n"
		"[0] = \"Hi\",\n"
		"[1] = 123.45,\n"
		"[2] = \"String value\",\n"
		// Does nothing
		//"[3] = nil,\n"
		//"[nil] = 3,\n"
		"[\"WOOF\"] = 123,\n"
		"}");
	sol::table tbl = lua["arr"];
	std::size_t tablesize = 4;
	std::size_t iterations = 0;
	auto fx = [&iterations](sol::object key, sol::object value) {
		++iterations;
		sol::type keytype = key.get_type();
		switch (keytype) {
		case sol::type::number:
			switch (key.as<int>()) {
			case 0:
				REQUIRE((value.as<std::string>() == "Hi"));
				break;
			case 1:
				REQUIRE((value.as<double>() == 123.45));
				break;
			case 2:
				REQUIRE((value.as<std::string>() == "String value"));
				break;
			case 3:
				REQUIRE((value.is<sol::lua_nil_t>()));
				break;
			}
			break;
		case sol::type::string:
			if (key.as<std::string>() == "WOOF") {
				REQUIRE((value.as<double>() == 123));
			}
			break;
		case sol::type::lua_nil:
			REQUIRE((value.as<double>() == 3));
			break;
		default:
			break;
		}
	};
	auto fxpair = [&fx](std::pair<sol::object, sol::object> kvp) { fx(kvp.first, kvp.second); };
	tbl.for_each(fx);
	REQUIRE(iterations == tablesize);

	iterations = 0;
	tbl.for_each(fxpair);
	REQUIRE(iterations == tablesize);
}

TEST_CASE("tables/for_each empty", "empty tables should not crash") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.safe_script("arr = {}");
	sol::table tbl = lua["arr"];
	REQUIRE(tbl.empty());
	std::size_t tablesize = 0;
	std::size_t iterations = 0;
	auto fx = [&iterations](sol::object key, sol::object value) {
		++iterations;
		sol::type keytype = key.get_type();
		switch (keytype) {
		case sol::type::number:
			switch (key.as<int>()) {
			case 0:
				REQUIRE((value.as<std::string>() == "Hi"));
				break;
			case 1:
				REQUIRE((value.as<double>() == 123.45));
				break;
			case 2:
				REQUIRE((value.as<std::string>() == "String value"));
				break;
			case 3:
				REQUIRE((value.is<sol::lua_nil_t>()));
				break;
			}
			break;
		case sol::type::string:
			if (key.as<std::string>() == "WOOF") {
				REQUIRE((value.as<double>() == 123));
			}
			break;
		case sol::type::lua_nil:
			REQUIRE((value.as<double>() == 3));
			break;
		default:
			break;
		}
	};
	auto fxpair = [&fx](std::pair<sol::object, sol::object> kvp) { fx(kvp.first, kvp.second); };
	tbl.for_each(fx);
	REQUIRE(iterations == tablesize);

	iterations = 0;
	tbl.for_each(fxpair);
	REQUIRE(iterations == tablesize);

	iterations = 0;
	for (const auto& kvp : tbl) {
		fxpair(kvp);
		++iterations;
	}
	REQUIRE(iterations == tablesize);
}

TEST_CASE("tables/iterators", "Testing the use of iteratrs to get values from a lua table") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.safe_script(
		"arr = {\n"
		"[0] = \"Hi\",\n"
		"[1] = 123.45,\n"
		"[2] = \"String value\",\n"
		// Does nothing
		//"[3] = nil,\n"
		//"[nil] = 3,\n"
		"[\"WOOF\"] = 123,\n"
		"}");
	sol::table tbl = lua["arr"];
	std::size_t tablesize = 4;
	std::size_t iterations = 0;

	int begintop = 0;
	int endtop = 0;
	{
		test_stack_guard s(lua.lua_state(), begintop, endtop);
		for (auto& kvp : tbl) {
			[&iterations](sol::object key, sol::object value) {
				++iterations;
				sol::type keytype = key.get_type();
				switch (keytype) {
				case sol::type::number:
					switch (key.as<int>()) {
					case 0:
						REQUIRE((value.as<std::string>() == "Hi"));
						break;
					case 1:
						REQUIRE((value.as<double>() == 123.45));
						break;
					case 2:
						REQUIRE((value.as<std::string>() == "String value"));
						break;
					case 3:
						REQUIRE((value.is<sol::lua_nil_t>()));
						break;
					}
					break;
				case sol::type::string:
					if (key.as<std::string>() == "WOOF") {
						REQUIRE((value.as<double>() == 123));
					}
					break;
				case sol::type::lua_nil:
					REQUIRE((value.as<double>() == 3));
					break;
				default:
					break;
				}
			}(kvp.first, kvp.second);
		}
	}
	REQUIRE(begintop == endtop);
	REQUIRE(iterations == tablesize);
}

TEST_CASE("tables/variables", "Check if tables and variables work as intended") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::os);
	lua.get<sol::table>("os").set("name", "windows");
	{
		auto result = lua.safe_script("assert(os.name == \"windows\")", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("tables/create", "Check if creating a table is kosher") {
	sol::state lua;
	lua["testtable"] = sol::table::create(lua.lua_state(), 0, 0, "Woof", "Bark", 1, 2, 3, 4);
	sol::object testobj = lua["testtable"];
	REQUIRE(testobj.is<sol::table>());
	sol::table testtable = testobj.as<sol::table>();
	REQUIRE((testtable["Woof"] == std::string("Bark")));
	REQUIRE((testtable[1] == 2));
	REQUIRE((testtable[3] == 4));
}

TEST_CASE("tables/create local", "Check if creating a table is kosher") {
	sol::state lua;
	lua["testtable"] = lua.create_table(0, 0, "Woof", "Bark", 1, 2, 3, 4);
	sol::object testobj = lua["testtable"];
	REQUIRE(testobj.is<sol::table>());
	sol::table testtable = testobj.as<sol::table>();
	REQUIRE((testtable["Woof"] == std::string("Bark")));
	REQUIRE((testtable[1] == 2));
	REQUIRE((testtable[3] == 4));
}

TEST_CASE("tables/create local named", "Check if creating a table is kosher") {
	sol::state lua;
	sol::table testtable = lua.create_table("testtable", 0, 0, "Woof", "Bark", 1, 2, 3, 4);
	sol::object testobj = lua["testtable"];
	REQUIRE(testobj.is<sol::table>());
	REQUIRE((testobj == testtable));
	REQUIRE_FALSE((testobj != testtable));
	REQUIRE((testtable["Woof"] == std::string("Bark")));
	REQUIRE((testtable[1] == 2));
	REQUIRE((testtable[3] == 4));
}

TEST_CASE("tables/create-with-local", "Check if creating a table is kosher") {
	sol::state lua;
	lua["testtable"] = lua.create_table_with("Woof", "Bark", 1, 2, 3, 4);
	sol::object testobj = lua["testtable"];
	REQUIRE(testobj.is<sol::table>());
	sol::table testtable = testobj.as<sol::table>();
	REQUIRE((testtable["Woof"] == std::string("Bark")));
	REQUIRE((testtable[1] == 2));
	REQUIRE((testtable[3] == 4));
}

TEST_CASE("tables/function variables", "Check if tables and function calls work as intended") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::os);
	auto run_script = [](sol::state& lua) -> void {
		lua.safe_script("assert(os.fun() == \"test\")");
	};

	lua.get<sol::table>("os").set_function("fun",
		[]() {
			INFO("stateless lambda()");
			return "test";
		});
	REQUIRE_NOTHROW(run_script(lua));

	lua.get<sol::table>("os").set_function("fun", &free_function);
	REQUIRE_NOTHROW(run_script(lua));

	// l-value, canNOT optimise
	// prefer value semantics unless wrapped with std::reference_wrapper
	{
		auto lval = object();
		lua.get<sol::table>("os").set_function("fun", &object::operator(), lval);
	}
	REQUIRE_NOTHROW(run_script(lua));

	auto reflval = object();
	lua.get<sol::table>("os").set_function("fun", &object::operator(), std::ref(reflval));
	REQUIRE_NOTHROW(run_script(lua));

	// stateful lambda: non-convertible, cannot be optimised
	int breakit = 50;
	lua.get<sol::table>("os").set_function("fun",
		[&breakit]() {
			INFO("stateful lambda() with breakit:" << breakit);
			return "test";
		});
	REQUIRE_NOTHROW(run_script(lua));

	// r-value, cannot optimise
	lua.get<sol::table>("os").set_function("fun", &object::operator(), object());
	REQUIRE_NOTHROW(run_script(lua));

	// r-value, cannot optimise
	auto rval = object();
	lua.get<sol::table>("os").set_function("fun", &object::operator(), std::move(rval));
	REQUIRE_NOTHROW(run_script(lua));
}

TEST_CASE("tables/operator[]", "Check if operator[] retrieval and setting works properly") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.safe_script("foo = 20\nbar = \"hello world\"");
	// basic retrieval
	std::string bar = lua["bar"];
	int foo = lua["foo"];
	REQUIRE(bar == "hello world");
	REQUIRE(foo == 20);
	// test operator= for stringification
	// errors due to ambiguous operators
	bar = lua["bar"];

	// basic setting
	lua["bar"] = 20.4;
	lua["foo"] = "goodbye";

	// doesn't modify the actual values obviously.
	REQUIRE(bar == "hello world");
	REQUIRE(foo == 20);

	// function setting
	lua["test"] = plop_xyz;
	{
		auto result = lua.safe_script("assert(test(10, 11, \"hello\") == 11)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	// function retrieval
	sol::function test = lua["test"];
	REQUIRE(test.call<int>(10, 11, "hello") == 11);

	// setting a lambda
	lua["lamb"] = [](int x) {
		return x * 2;
	};

	{
		auto result = lua.safe_script("assert(lamb(220) == 440)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	// function retrieval of a lambda
	sol::function lamb = lua["lamb"];
	REQUIRE(lamb.call<int>(220) == 440);

	// test const table retrieval
	auto assert1 = [](const sol::table& t) {
		std::string a = t["foo"];
		double b = t["bar"];
		REQUIRE(a == "goodbye");
		REQUIRE(b == 20.4);
	};

	REQUIRE_NOTHROW(assert1(lua.globals()));
}

TEST_CASE("tables/operator[] valid", "Test if proxies on tables can lazily evaluate validity") {
	sol::state lua;
	bool isFullScreen = false;
	auto fullscreennopers = lua["fullscreen"]["nopers"];
	auto fullscreen = lua["fullscreen"];
	REQUIRE_FALSE(fullscreennopers.valid());
	REQUIRE_FALSE(fullscreen.valid());

	lua["fullscreen"] = true;

	REQUIRE_FALSE(fullscreennopers.valid());
	REQUIRE(fullscreen.valid());
	isFullScreen = lua["fullscreen"];
	REQUIRE(isFullScreen);

	lua["fullscreen"] = false;
	REQUIRE_FALSE(fullscreennopers.valid());
	REQUIRE(fullscreen.valid());
	isFullScreen = lua["fullscreen"];
	REQUIRE_FALSE(isFullScreen);
}

TEST_CASE("tables/operator[] optional", "Test if proxies on tables can lazily evaluate validity") {
	sol::state lua;

	sol::optional<int> test1 = lua["no_exist_yet"];
	bool present = (bool)test1;
	REQUIRE_FALSE(present);

	lua["no_exist_yet"] = 262;
	sol::optional<int> test2 = lua["no_exist_yet"];
	present = (bool)test2;
	REQUIRE(present);
	REQUIRE(test2.value() == 262);

	sol::optional<int> nope = lua["nope"]["kek"]["hah"];
	auto nope2 = lua.get<sol::optional<int>>(std::make_tuple("nope", "kek", "hah"));
	present = (bool)nope;
	REQUIRE_FALSE(present);
	present = (bool)nope2;
	REQUIRE_FALSE(present);
	lua.create_named_table("nope", "kek", lua.create_table_with("hah", 1));
	sol::optional<int> non_nope = lua["nope"]["kek"]["hah"].get<sol::optional<int>>();
	sol::optional<int> non_nope2 = lua.get<sol::optional<int>>(std::make_tuple("nope", "kek", "hah"));
	present = (bool)non_nope;
	REQUIRE(present);
	present = (bool)non_nope2;
	REQUIRE(present);
	REQUIRE(non_nope.value() == 1);
	REQUIRE(non_nope2.value() == 1);

	INFO("Keys: nope, kek, hah");
	lua.set(std::make_tuple("nope", "kek", "hah"), 35);
	sol::optional<int> non_nope3 = lua["nope"]["kek"]["hah"].get<sol::optional<int>>();
	sol::optional<int> non_nope4 = lua.get<sol::optional<int>>(std::make_tuple("nope", "kek", "hah"));
	present = (bool)non_nope3;
	REQUIRE(present);
	present = (bool)non_nope4;
	REQUIRE(present);
	REQUIRE(non_nope3.value() == 35);
	REQUIRE(non_nope4.value() == 35);
}

TEST_CASE("tables/add", "Basic test to make sure the 'add' feature works") {
	static const int sz = 120;

	sol::state lua;
	sol::table t = lua.create_table(sz, 0);

	std::vector<int> bigvec(sz);
	std::iota(bigvec.begin(), bigvec.end(), 1);

	for (std::size_t i = 0; i < bigvec.size(); ++i) {
		t.add(bigvec[i]);
	}
	for (std::size_t i = 0; i < bigvec.size(); ++i) {
		int val = t[i + 1];
		REQUIRE(val == bigvec[i]);
	}
}

TEST_CASE("tables/raw set and raw get", "ensure raw setting and getting works through metatables") {
	sol::state lua;
	sol::table t = lua.create_table();
	t[sol::metatable_key] = lua.create_table_with(
		sol::meta_function::new_index, [](lua_State* L) { return luaL_error(L, "nay"); },
		sol::meta_function::index, [](lua_State* L) { return luaL_error(L, "nay"); });
	t.raw_set("a", 2.5);
	double la = t.raw_get<double>("a");
	REQUIRE(la == 2.5);
}

TEST_CASE("tables/boolean keys", "make sure boolean keys don't get caught up in `is_integral` specializations") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.safe_script(R"(
tbl = {}
tbl[true] = 10
tbl[1] = 20

print(tbl[true])
print(tbl[1])
)");
	sol::table tbl = lua["tbl"];
	int v1 = tbl[true];
	int v2 = tbl[1];
	REQUIRE(v1 == 10);
	REQUIRE(v2 == 20);

	tbl[true] = 30;
	tbl[1] = 40;
	v1 = tbl[true];
	v2 = tbl[1];
	REQUIRE(v1 == 30);
	REQUIRE(v2 == 40);
}

TEST_CASE("tables/optional move", "ensure pushing a sol::optional<T> rvalue correctly moves the contained object into tables") {
	sol::state sol_state;
	struct move_only {
		int secret_code;
		move_only(const move_only&) = delete;
		move_only(move_only&&) = default;
	};
	sol_state["requires_move"] = sol::optional<move_only>{ move_only{ 0x4D } };
	REQUIRE(sol_state["requires_move"].get<move_only>().secret_code == 0x4D);
}
