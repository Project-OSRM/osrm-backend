#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int main () {
	struct protect_me {
		int gen(int x) {
			return x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<protect_me>("protect_me", 
		"gen", sol::protect( &protect_me::gen )
	);

	lua.script(R"__(
	pm = protect_me.new()
	value = pcall(pm.gen,"wrong argument")
	)__");
	bool value = lua["value"];
	c_assert(!value);

	return 0;
}
