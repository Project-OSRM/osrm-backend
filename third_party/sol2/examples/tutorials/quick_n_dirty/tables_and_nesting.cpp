#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "../../assert.hpp"

int main(int, char*[]) {

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.script(R"(
		abc = { [0] = 24 }
		def = { 
			ghi = { 
				bark = 50, 
				woof = abc 
			} 
		}
	)");

	sol::table abc = lua["abc"];
	sol::table def = lua["def"];
	sol::table ghi = lua["def"]["ghi"];

	int bark1 = def["ghi"]["bark"];
	int bark2 = lua["def"]["ghi"]["bark"];
	c_assert(bark1 == 50);
	c_assert(bark2 == 50);

	int abcval1 = abc[0];
	int abcval2 = ghi["woof"][0];
	c_assert(abcval1 == 24);
	c_assert(abcval2 == 24);

	sol::optional<int> will_not_error = lua["abc"]["DOESNOTEXIST"]["ghi"];
	c_assert(will_not_error == sol::nullopt);
	
	int also_will_not_error = lua["abc"]["def"]["ghi"]["jklm"].get_or(25);
	c_assert(also_will_not_error == 25);

	// if you don't go safe,
	// will throw (or do at_panic if no exceptions)
	//int aaaahhh = lua["boom"]["the_dynamite"];

	return 0;
}
