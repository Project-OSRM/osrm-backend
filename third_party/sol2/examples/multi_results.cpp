#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <tuple>
#include "assert.hpp"
#include <iostream>

int main() {
	std::cout << "=== multi results ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// multi-return functions are supported using
	// std::tuple as the transfer type,
	// sol::as_returns for containers,
	// and sol::variadic_results for more special things
	lua.set_function("multi_tuple", [] { 
		return std::make_tuple(10, "goodbye"); 
	});
	lua.script("print('calling multi_tuple')");
	lua.script("print(multi_tuple())");
	lua.script("x, y = multi_tuple()");
	lua.script("assert(x == 10 and y == 'goodbye')");

	auto multi = lua.get<sol::function>("multi_tuple");
	int first;
	std::string second;
	// tie the values
	sol::tie(first, second) = multi();

	// use the values
	c_assert(first == 10);
	c_assert(second == "goodbye");

	// sol::as_returns
	// works with any iterable, 
	// but we show off std::vector here
	lua.set_function("multi_containers", [] (bool add_extra) { 
		std::vector<int> values{55, 66};
		if (add_extra) {
			values.push_back(77);
		}
		return sol::as_returns(std::move(values)); 
	});
	lua.script("print('calling multi_containers')");
	lua.script("print(multi_containers(false))");
	lua.script("a, b, c = multi_containers(true)");
	int a = lua["a"];
	int b = lua["b"];
	int c = lua["c"];

	c_assert(a == 55);
	c_assert(b == 66);
	c_assert(c == 77);

	// sol::variadic_results
	// you can push objects of different types
	// note that sol::this_state is a transparent
	// argument: you don't need to pass
	// that state through Lua
	lua.set_function("multi_vars", [](int a, bool b, sol::this_state L) {
		sol::variadic_results values;
		values.push_back({ L, sol::in_place_type<int>, a });
		values.push_back({ L, sol::in_place_type<bool>, b });
		values.push_back({ L, sol::in_place, "awoo" });
		return values;
	});
	lua.script("print('calling multi_vars')");
	lua.script("print(multi_vars(2, false))");
	lua.script("t, u, v = multi_vars(42, true)");
	int t = lua["t"];
	bool u = lua["u"];
	std::string v = lua["v"];

	c_assert(t == 42);
	c_assert(u);
	c_assert(v == "awoo");

	return 0;
}