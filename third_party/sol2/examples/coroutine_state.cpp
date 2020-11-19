#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

int main(int, char*[]) {
	std::cout << "=== coroutine state transfer ===" << std::endl;

	sol::state lua;
	lua.open_libraries();
	sol::function transferred_into;
	lua["f"] = [&lua, &transferred_into](sol::object t, sol::this_state this_L) {
		std::cout << "state of main     : " << (void*)lua.lua_state() << std::endl;
		std::cout << "state of function : " << (void*)this_L.lua_state() << std::endl;
		// pass original lua_State* (or sol::state/sol::state_view)
		// transfers ownership from the state of "t",
		// to the "lua" sol::state
		transferred_into = sol::function(lua, t);
	};

	lua.script(R"(
		i = 0
		function test()
		co = coroutine.create(
			function()
				local g = function() i = i + 1 end
				f(g)
				g = nil
				collectgarbage()
			end
		)
		coroutine.resume(co)
		co = nil
		collectgarbage()
		end
		)");

	// give it a try
	lua.safe_script("test()");
	// should call 'g' from main thread, increment i by 1
	transferred_into();
	// check
	int i = lua["i"];
	c_assert(i == 1);

	std::cout << std::endl;

	return 0;
}
