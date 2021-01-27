#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int func_1(int value) {
	return 20 + value;
}

std::string func_2(std::string text) {
	return "received: " + text;
}

sol::variadic_results fallback(sol::this_state ts, sol::variadic_args args) {
	sol::variadic_results r;
	if (args.size() == 2) {
		r.push_back({ ts, sol::in_place, args.get<int>(0) + args.get<int>(1) });
	}
	else {
		r.push_back({ ts, sol::in_place, 52 });
	}
	return r;
}

int main(int, char*[]) {
	std::cout << "=== overloading with fallback ===" << std::endl;

	sol::state lua;
	lua.open_libraries();

	lua.set_function("f", sol::overload(
		func_1,
		func_2,
		fallback
	));

	lua.script("print(f(1))"); // func_1
	lua.script("print(f('hi'))"); // func_2
	lua.script("print(f(22, 11))"); // fallback
	lua.script("print(f({}))"); // fallback

	return 0;
}
