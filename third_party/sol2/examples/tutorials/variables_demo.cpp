#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <tuple>
#include "../assert.hpp"
#include <utility> // for std::pair

int main() {

	sol::state lua;
	/*
	lua.script_file("variables.lua");
	*/
	lua.script(R"(
config = {
	fullscreen = false,
	resolution = { x = 1024, y = 768 }
}
	)");
	// the type "sol::state" behaves 
	// exactly like a table!
	bool isfullscreen = lua["config"]["fullscreen"]; // can get nested variables
	sol::table config = lua["config"];
	c_assert(isfullscreen);

	// can also get it using the "get" member function
	// auto replaces the unqualified type name
	auto resolution = config.get<sol::table>("resolution");

	// table and state can have multiple things pulled out of it too
	std::tuple<int, int> xyresolutiontuple = resolution.get<int, int>("x", "y");
	c_assert(std::get<0>(xyresolutiontuple) == 1024);
	c_assert(std::get<1>(xyresolutiontuple) == 768);

	// test variable
	auto bark = lua["config"]["bark"];
	if (bark.valid()) {
		// branch not taken: config and/or bark are not variables
	}
	else {
		// Branch taken: config and bark are existing variables
	}

	// can also use optional
	sol::optional<int> not_an_integer = lua["config"]["fullscreen"];
	if (not_an_integer) {
		// Branch not taken: value is not an integer
	}

	sol::optional<bool> is_a_boolean = lua["config"]["fullscreen"];
	if (is_a_boolean) {
		// Branch taken: the value is a boolean
	}

	sol::optional<double> does_not_exist = lua["not_a_variable"];
	if (does_not_exist) {
		// Branch not taken: that variable is not present
	}

	// this will result in a value of '24'
	// (it tries to get a number, and fullscreen is
	// not a number
	int is_defaulted = lua["config"]["fullscreen"].get_or(24);
	c_assert(is_defaulted == 24);

	// This will result in the value of the config, which is 'false'
	bool is_not_defaulted = lua["config"]["fullscreen"];
	c_assert(!is_not_defaulted);

	return 0;
}
