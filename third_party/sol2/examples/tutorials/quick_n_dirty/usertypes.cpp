#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

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
	std::cout << "=== usertypes ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	Doge dog{ 30 };

	lua["dog"] = Doge{};
	lua["dog_copy"] = dog;
	lua["dog_move"] = std::move(dog);
	lua["dog_unique_ptr"] = std::make_unique<Doge>(21);
	lua["dog_shared_ptr"] = std::make_shared<Doge>(51);

	// now we can access these types in Lua
	lua.new_usertype<Doge>( "Doge",
		sol::constructors<Doge(), Doge(int)>(),
		"tailwag", &Doge::tailwag
	);
	lua.script(R"(
		function f (dog)
			if dog == nil then
				print('dog was nil!')
				return
			end
			print('dog wags its tail ' .. dog.tailwag .. ' times!')
		end
	)");

	lua.script(R"(
		dog_lua = Doge.new()

		f(dog_lua)
		f(dog)
		f(dog_copy)
		f(dog_move)
		f(dog)
		f(dog_unique_ptr)
		f(dog_shared_ptr)
		f(nil)
	)");

	std::cout << std::endl;

	return 0;
}
