#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <fstream>
#include <iostream>
#include <cstdio>
#include "../../assert.hpp"

int main(int, char*[]) {
	std::cout << "=== running lua code (low level) ===" << std::endl;
	
	{
		std::ofstream out("a_lua_script.lua");
		out << "print('hi from a lua script file')";
	}

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// load file without execute
	sol::load_result script1 = lua.load_file("a_lua_script.lua");
	//execute
	script1();

	// load string without execute
	sol::load_result script2 = lua.load("a = 'test'");
	//execute
	sol::protected_function_result script2result = script2();
	// optionally, check if it worked
	if (script2result.valid()) {
		// yay!
	}
	else {
		// aww
	}

	sol::load_result script3 = lua.load("return 24");
	// execute, get return value
	int value2 = script3();
	c_assert(value2 == 24);

	std::cout << std::endl;

	{
		std::remove("a_lua_script.lua");
	}

	return 0;
}
