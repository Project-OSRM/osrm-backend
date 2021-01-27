#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include "assert.hpp"

int main() {
	std::cout << "=== basic ===" << std::endl;
	// create an empty lua state
	sol::state lua;

	// by default, libraries are not opened
	// you can open libraries by using open_libraries
	// the libraries reside in the sol::lib enum class
	lua.open_libraries(sol::lib::base);
	// you can open all libraries by passing no arguments
	//lua.open_libraries();

	// call lua code directly
	lua.script("print('hello world')");

	// call lua code, and check to make sure it has loaded and run properly:
	auto handler = &sol::script_default_on_error;
	lua.script("print('hello again, world')", handler);

	// Use a custom error handler if you need it
	// This gets called when the result is bad
	auto simple_handler = [](lua_State*, sol::protected_function_result result) {
		// You can just pass it through to let the call-site handle it
		return result;
	};
	// the above lambda is identical to sol::simple_on_error, but it's
	// shown here to show you can write whatever you like

	// 
	{
		auto result = lua.script("print('hello hello again, world') \n return 24", simple_handler);
		if (result.valid()) {
			std::cout << "the third script worked, and a double-hello statement should appear above this one!" << std::endl;
			int value = result;
			c_assert(value == 24);
		}
		else {
			std::cout << "the third script failed, check the result type for more information!" << std::endl;
		}
	}

	{ 
		auto result = lua.script("does.not.exist", simple_handler);
		if (result.valid()) {
			std::cout << "the fourth script worked, which it wasn't supposed to! Panic!" << std::endl;
			int value = result;
			c_assert(value == 24);
		}
		else {
			sol::error err = result;
			std::cout << "the fourth script failed, which was intentional!\t\nError: " << err.what() << std::endl;
		}
	}

	std::cout << std::endl;
	
	return 0;
}