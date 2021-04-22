#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int main(int, char*[]) {
	sol::state lua;
	lua.script("function func (a, b) return (a + b) * 2 end");

	sol::reference func_ref = lua["func"];

	// for some reason, you need to use the low-level API
	func_ref.push(); // function on stack now

	// maybe this is in a lua_CFunction you bind,
	// or maybe you're trying to work with a pre-existing system
	// maybe you've used a custom lua_load call, or you're working
	// with state_view's load(lua_Reader, ...) call...
	// here's a little bit of how you can work with the stack
	lua_State* L = lua.lua_state();
	sol::stack_aligned_function func(L, -1);
	lua_pushinteger(L, 5); // argument 1, using plain API
	lua_pushinteger(L, 6); // argument 2
	
	// take 2 arguments from the top, 
	// and use "stack_aligned_function" to call
	int result = func(sol::stack_count(2));

	// make sure everything is clean
	c_assert(result == 22);
	c_assert(lua.stack_top() == 0); // stack is empty/balanced

	return 0;
}
