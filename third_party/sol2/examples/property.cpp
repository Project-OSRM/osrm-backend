#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <iostream>

class Player {
public:
	int get_hp() const {
		return hp;
	}

	void set_hp( int value ) {
		hp = value;
	}

	int get_max_hp() const {
		return hp;
	}

	void set_max_hp( int value ) {
		maxhp = value;
	}

private:
	int hp = 50;
	int maxhp = 50;
};

int main (int, char*[]) {

	std::cout << "=== properties from C++ functions ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set("theplayer", Player());

	// Yes, you can register after you set a value and it will
	// connect up the usertype automatically
	lua.new_usertype<Player>( "Player",
		"hp", sol::property(&Player::get_hp, &Player::set_hp),
		"maxHp", sol::property(&Player::get_max_hp, &Player::set_max_hp)
	);

	const auto& code = R"(
	-- variable syntax, calls functions
	theplayer.hp = 20
	print('hp:', theplayer.hp)
	print('max hp:', theplayer.maxHp)
	)";

	lua.script(code);

	return 0;
}
