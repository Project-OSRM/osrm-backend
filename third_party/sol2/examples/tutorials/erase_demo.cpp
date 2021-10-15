#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

int main() {

	sol::state lua;
	lua["bark"] = 50;
	sol::optional<int> x = lua["bark"];
	// x will have a value

	lua["bark"] = sol::nil;
	sol::optional<int> y = lua["bark"];
	// y will not have a value

	return 0;
}
