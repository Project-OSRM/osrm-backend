#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main(int, char*[]) {
	std::cout << "=== environment state ===" << std::endl;

	sol::state lua;
	lua.open_libraries();
	sol::environment my_env(lua, sol::create);
	// set value, and we need to explicitly allow for 
	// access to "print", since a new environment hides 
	// everything that's not defined inside of it
	// NOTE: hiding also hides library functions (!!)
	// BE WARNED
	my_env["var"] = 50;
	my_env["print"] = lua["print"];

	sol::environment my_other_env(lua, sol::create, lua.globals());
	// do not need to explicitly allow access to "print",
	// since we used the "Set a fallback" version 
	// of the sol::environment constructor
	my_other_env["var"] = 443;

	// output: 50
	lua.script("print(var)", my_env);

	// output: 443
	lua.script("print(var)", my_other_env);


	std::cout << std::endl;

	return 0;
}
