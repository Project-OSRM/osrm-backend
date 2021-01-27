#define SOL_CHECK_ARGUMENTS 1

#include <sol.hpp>
#include "../assert.hpp"

int main() {
	sol::state lua;
	int x = 0;
	lua.set_function("beep", [&x]{ ++x; });
	lua.script("beep()");
	c_assert(x == 1);

	sol::function beep = lua["beep"];
	beep();
	c_assert(x == 2);

	return 0;
}
