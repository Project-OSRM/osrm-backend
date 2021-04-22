#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int main () {
	sol::state lua;
    
	lua.set_function("bark", []( sol::this_state s, int a, int b ){
		lua_State* L = s; // current state
		return a + b + lua_gettop(L);
	});
	
	lua.script("first = bark(2, 2)"); // only takes 2 arguments, NOT 3
		
	// Can be at the end, too, or in the middle: doesn't matter
	lua.set_function("bark", []( int a, int b, sol::this_state s ){
		lua_State* L = s; // current state
		return a + b + lua_gettop(L);
	});

	lua.script("second = bark(2, 2)"); // only takes 2 arguments
	int first = lua["first"];
	c_assert(first == 6);
	int second = lua["second"];
	c_assert(second == 6);

	return 0;
}