#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include <vector>
#include <set>

int main(int , const char*[]) {
	
	std::cout << "=== args_from_container ===" << std::endl;

	sol::state lua;
	lua.open_libraries();

	lua.script("function f (a, b, c, d) print(a, b, c, d) end");

	sol::function f = lua["f"];

	std::vector<int> v2{ 3, 4 };
	f(1, 2, sol::as_args(v2));

	std::set<int> v4{ 3, 1, 2, 4 };
	f(sol::as_args(v4));

	int v3[] = { 2, 3, 4 };
	f(1, sol::as_args(v3));

	std::cout << std::endl;

	return 0;
}