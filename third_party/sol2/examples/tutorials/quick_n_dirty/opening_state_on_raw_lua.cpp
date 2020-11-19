#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int use_sol2(lua_State* L) {
	sol::state_view lua(L);
	lua.script("print('bark bark bark!')");
	return 0;
}

int main(int, char*[]) {
	std::cout << "=== opening sol::state_view on raw Lua ===" << std::endl;

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	lua_pushcclosure(L, &use_sol2, 0);
	lua_setglobal(L, "use_sol2");

	if (luaL_dostring(L, "use_sol2()")) {
		lua_error(L);
		return -1;
	}

	std::cout << std::endl;

	return 0;
}
