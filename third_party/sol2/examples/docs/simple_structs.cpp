#define SOL_CHECK_ARGUMENTS 1

#include <sol.hpp>
#include "../assert.hpp"

struct vars {
	int boop = 0;

	int bop () const {
		return boop + 1;
	}
};

int main() {
	sol::state lua;
	lua.new_usertype<vars>("vars", 
		"boop", &vars::boop,
		"bop", &vars::bop);
	lua.script("beep = vars.new()\n"
		"beep.boop = 1\n"
		"bopvalue = beep:bop()");

	vars& beep = lua["beep"];
	int bopvalue = lua["bopvalue"];

	c_assert(beep.boop == 1);
	c_assert(lua.get<vars>("beep").boop == 1);
	c_assert(beep.bop() == 2);
	c_assert(bopvalue == 2);

	return 0;
}
