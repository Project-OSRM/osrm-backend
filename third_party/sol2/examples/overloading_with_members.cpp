#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <iostream>

struct pup {
	int barks = 0;

	void bark () {
		++barks; // bark!
	}

	bool is_cute () const { 
		return true;
	}
};

void ultra_bark( pup& p, int barks) {
	for (; barks --> 0;) p.bark();
}

void picky_bark( pup& p, std::string s) {
	if ( s == "bark" )
	    p.bark();
}

int main () {
	std::cout << "=== overloading with members ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function( "bark", sol::overload( 
		ultra_bark, 
		[]() { return "the bark from nowhere"; } 
	) );

	lua.new_usertype<pup>( "pup",
		// regular function
		"is_cute", &pup::is_cute,
		// overloaded function
		"bark", sol::overload( &pup::bark, &picky_bark )
	);

	const auto& code = R"(
	barker = pup.new()
	print(barker:is_cute())
	barker:bark() -- calls member function pup::bark
	barker:bark("meow") -- picky_bark, no bark
	barker:bark("bark") -- picky_bark, bark

	bark(barker, 20) -- calls ultra_bark
	print(bark()) -- calls lambda which returns that string
	)";

	lua.script(code);

	pup& barker = lua["barker"];
	std::cout << barker.barks << std::endl;
	c_assert(barker.barks == 22);

	std::cout << std::endl;
	return 0;
}