#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main () {
	
	std::cout << "=== variadic_args shifting constructor ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", [](sol::variadic_args va) {
		int r = 0;
		sol::variadic_args shifted_va(va.lua_state(), 3);
		for (auto v : shifted_va) {
			int value = v;
			r += value;
		}
		return r;
	});
    
	lua.script("x = f(1, 2, 3, 4)");
	lua.script("x2 = f(8, 200, 3, 4)");
	lua.script("x3 = f(1, 2, 3, 4, 5, 6)");
	
	lua.script("print(x)"); // 7
	lua.script("print(x2)"); // 7
	lua.script("print(x3)"); // 18

	std::cout << std::endl;

	return 0;
}
