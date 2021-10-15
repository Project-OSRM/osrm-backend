#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <string>
#include <set>

int main () {
	sol::state lua;

	lua.set_function("f", []() {
		std::set<std::string> results{ "arf", "bark", "woof" };
		return sol::as_returns(std::move(results));
	});
	
	lua.script("v1, v2, v3 = f()");

	std::string v1 = lua["v1"];
	std::string v2 = lua["v2"];
	std::string v3 = lua["v3"];

	c_assert(v1 == "arf");
	c_assert(v2 == "bark");
	c_assert(v3 == "woof");

	return 0;		
}
