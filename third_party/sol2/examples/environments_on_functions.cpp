#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

int main(int, char**) {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// Environments can set on functions (scripts), userdata and threads
	// let's look at functions

	lua.script("f = function() return test end");
	sol::function f = lua["f"];
	
	sol::environment env_f(lua, sol::create);
	env_f["test"] = 31;
	sol::set_environment(env_f, f);

	// the function returns the value from the environment table
	int result = f();
	c_assert(result == 31);

	
	// You can also protect from variables
	// being set without the 'local' specifier
	lua.script("g = function() test = 5 end");
	sol::function g = lua["g"];
	sol::environment env_g(lua, sol::create);
	env_g.set_on(g); // same as set_environment

	g();
	// the value can be retrieved from the env table
	int test = env_g["test"];
	c_assert(test == 5);


	// the global environment
	// is not polluted at all, despite both functions being used and set
	sol::object global_test = lua["test"];
	c_assert(!global_test.valid());


	// You can retrieve environments in C++
	// and check the environment of functions
	// gotten from Lua

	// get the environment from any sol::reference-styled type,
	// including sol::object, sol::function, sol::table, sol::userdata ...
	lua.set_function("check_f_env",
		// capture necessary variable in C++ lambda
		[&env_f]( sol::object target ) {
			// pull out the environment from func using
			// sol::env_key constructor
			sol::environment target_env(sol::env_key, target);
			int test_env_f = env_f["test"];
			int test_target_env = target_env["test"];
			// the environment for f the one gotten from `target`
			// are the same
			c_assert(test_env_f == test_target_env);
			c_assert(test_env_f == 31);
			c_assert(env_f == target_env);
		}
	);
	lua.set_function("check_g_env",
		[&env_g](sol::function target) {
			// equivalent:
			sol::environment target_env = sol::get_environment(target);
			int test_env_g = env_g["test"];
			int test_target_env = target_env["test"];
			c_assert(test_env_g == test_target_env);
			c_assert(test_env_g == 5);
			c_assert(env_g == target_env);
		}
	);

	lua.script("check_f_env(f)");
	lua.script("check_g_env(g)");

	return 0;
}
