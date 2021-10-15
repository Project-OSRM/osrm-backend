#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "../../assert.hpp"
#include <iostream>

struct Doge {
	int tailwag = 50;

	Doge() {
	}

	Doge(int wags)
		: tailwag(wags) {
	}

	~Doge() {
		std::cout << "Dog at " << this << " is being destroyed..." << std::endl;
	}
};

int main(int, char* []) {
	std::cout << "=== userdata memory reference ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	Doge dog{}; // Kept alive somehow

	// Later...
	// The following stores a reference, and does not copy/move
	// lifetime is same as dog in C++
	// (access after it is destroyed is bad)
	lua["dog"] = &dog;
	// Same as above: respects std::reference_wrapper
	lua["dog"] = std::ref(dog);
	// These two are identical to above
	lua.set( "dog", &dog );
	lua.set( "dog", std::ref( dog ) );


	Doge& dog_ref = lua["dog"]; // References Lua memory
	Doge* dog_pointer = lua["dog"]; // References Lua memory
	Doge dog_copy = lua["dog"]; // Copies, will not affect lua

	lua.new_usertype<Doge>("Doge",
		"tailwag", &Doge::tailwag
	);

	dog_copy.tailwag = 525;
	// Still 50
	lua.script("assert(dog.tailwag == 50)");

	dog_ref.tailwag = 100;
	// Now 100
	lua.script("assert(dog.tailwag == 100)");

	dog_pointer->tailwag = 345;
	// Now 345
	lua.script("assert(dog.tailwag == 345)");

	std::cout << std::endl;

	return 0;
}
