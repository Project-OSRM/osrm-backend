#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include "../../assert.hpp"

int main() {
	std::cout << "=== namespacing ===" << std::endl;

	struct my_class {
		int b = 24;

		int f() const {
			return 24;
		}

		void g() {
			++b;
		}
	};

	sol::state lua;
	lua.open_libraries();

	// "bark" namespacing in Lua
	// namespacing is just putting things in a table
	// forces creation if it does not exist
	auto bark = lua["bark"].get_or_create<sol::table>();
	// equivalent-ish:
	//sol::table bark = lua["bark"].force(); // forces table creation
	// equivalent, and more flexible:
	//sol::table bark = lua["bark"].get_or_create<sol::table>(sol::new_table());
	// equivalent, but less efficient/ugly:
	//sol::table bark = lua["bark"] = lua.get_or("bark", lua.create_table());
	bark.new_usertype<my_class>("my_class",
		"f", &my_class::f,
		"g", &my_class::g); // the usual

	// can add functions, as well (just like the global table)
	bark.set_function("print_my_class", [](my_class& self) { std::cout << "my_class { b: " << self.b << " }" << std::endl; });

	// this works
	lua.script("obj = bark.my_class.new()");
	lua.script("obj:g()");

	// calling this function also works
	lua.script("bark.print_my_class(obj)");
	my_class& obj = lua["obj"];
	c_assert(obj.b == 25);

	std::cout << std::endl;

	return 0;
}
