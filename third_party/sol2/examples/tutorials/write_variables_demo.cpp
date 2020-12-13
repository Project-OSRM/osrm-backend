#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main() {

	sol::state lua;

	// open those basic lua libraries 
	// again, for print() and other basic utilities
	lua.open_libraries(sol::lib::base);

	// value in the global table
	lua["bark"] = 50;

	// a table being created in the global table
	lua["some_table"] = lua.create_table_with(
		"key0", 24,
		"key1", 25,
		lua["bark"], "the key is 50 and this string is its value!");

	// Run a plain ol' string of lua code
	// Note you can interact with things set through Sol in C++ with lua!
	// Using a "Raw String Literal" to have multi-line goodness: 
	// http://en.cppreference.com/w/cpp/language/string_literal
	lua.script(R"(
		
	print(some_table[50])
	print(some_table["key0"])
	print(some_table["key1"])

	-- a lua comment: access a global in a lua script with the _G table
	print(_G["bark"])

	)");

	return 0;
}
