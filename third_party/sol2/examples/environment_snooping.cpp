#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

// Simple sol2 version of the below
void simple(sol::this_state ts, sol::this_environment te) {
	sol::state_view lua = ts;
	if (te) {
		sol::environment& env = te;
		sol::environment freshenv = lua["freshenv"];
		bool is_same_env = freshenv == env;
		std::cout << "this_environment -- env == freshenv : " << is_same_env << std::endl;
	}
	std::cout << "this_environment -- no environment present" << std::endl;
}

// NOTE:
// THIS IS A LOW-LEVEL EXAMPLE, using pieces of sol2
// to facilitate better usage
// It is recommended you just do the simple version, as it is basically this code
// but it is sometimes useful to show the hoops you need to jump through to use the Lua C API
void complicated(sol::this_state ts) {
	lua_State* L = ts;

	lua_Debug info;
	// Level 0 means current function (this C function, which is useless for our purposes)
	// Level 1 means next call frame up the stack. This is probably the environment we're looking for?
	int level = 1;
	int pre_stack_size = lua_gettop(L);
	if (lua_getstack(L, level, &info) != 1) {
		// failure: call it quits
		std::cout << "error: unable to traverse the stack" << std::endl;
		lua_settop(L, pre_stack_size);
		return;
	}
	// the "f" identifier is the most important here
	// it pushes the function running at `level` onto the stack:
	// we can get the environment from this
	// the rest is for printing / debugging purposes
	if (lua_getinfo(L, "fnluS", &info) == 0) {
		// failure?
		std::cout << "manually -- error: unable to get stack information" << std::endl;
		lua_settop(L, pre_stack_size);
		return;
	}

	// Okay, so all the calls worked.
	// Print out some information about this "level"
	std::cout << "manually -- [" << level << "] " << info.short_src << ":" << info.currentline
		<< " -- " << (info.name ? info.name : "<unknown>") << "[" << info.what << "]" << std::endl;

	// Grab the function off the top of the stack
	// remember: -1 means top, -2 means 1 below the top, and so on...
	// 1 means the very bottom of the stack, 
	// 2 means 1 more up, and so on to the top value...
	sol::function f(L, -1);
	// The environment can now be ripped out of the function
	sol::environment env(sol::env_key, f);
	if (!env.valid()) {
		std::cout << "manually -- error: no environment to get" << std::endl;
		lua_settop(L, pre_stack_size);
		return;
	}
	sol::state_view lua(L);
	sol::environment freshenv = lua["freshenv"];
	bool is_same_env = freshenv == env;
	std::cout << "manually -- env == freshenv : " << is_same_env << std::endl;
}

int main() {
	std::cout << "=== environment snooping ===" << std::endl;
	sol::state lua;

	sol::environment freshenv(lua, sol::create, lua.globals());
	lua["freshenv"] = freshenv;
	lua.set_function("f", simple);
	lua.set_function("g", complicated);

	lua.script("f()", freshenv);
	lua.script("g()", freshenv);

	std::cout << std::endl;

	return 0;
}
