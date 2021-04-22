#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

struct some_class {
	int bark = 2012;
};

sol::table open_mylib(sol::this_state s) {
	sol::state_view lua(s);

	sol::table module = lua.create_table();
	module["func"] = []() { 
		/* super cool function here */
		return 2;
	};
	// register a class too
	module.new_usertype<some_class>("some_class",
		"bark", &some_class::bark
	);

 	return module;
}

int main() {
	std::cout << "=== require ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::package, sol::lib::base);
	// sol::c_call takes functions at the template level
	// and turns it into a lua_CFunction
	// alternatively, one can use sol::c_call<sol::wrap<callable_struct, callable_struct{}>> to make the call
	// if it's a constexpr struct
	lua.require("my_lib", sol::c_call<decltype(&open_mylib), &open_mylib>);

	// run some code against your require'd library
	lua.safe_script(R"(
s = my_lib.some_class.new()
assert(my_lib.func() == 2)
s.bark = 20
)");

	some_class& s = lua["s"];
	c_assert(s.bark == 20);
	std::cout << "s.bark = " << s.bark << std::endl;

	std::cout << std::endl;

	return 0;
}