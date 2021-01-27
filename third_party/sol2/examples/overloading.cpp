#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

inline int my_add(int x, int y) {
	return x + y;
}

inline std::string make_string(std::string input) {
	return "string: " + input;
}

int main() {
	std::cout << "=== overloading ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// you can overload functions
	// just pass in the different functions
	// you want to pack into a single name:
	// make SURE they take different types!

	lua.set_function("func", sol::overload(
		[](int x) { return x; }, 
		make_string, 
		my_add
	));

	// All these functions are now overloaded through "func"
	lua.script(R"(
print(func(1))
print(func("bark"))
print(func(1,2))
)");

	std::cout << std::endl;

	return 0;
}