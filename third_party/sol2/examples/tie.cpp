#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int main (int, char*[]) {

	const auto& code = R"(
	bark_power = 11;

	function woof ( bark_energy )
		return (bark_energy * (bark_power / 4))
	end
)";

	sol::state lua;

	lua.script(code);

	sol::function woof = lua["woof"];
	double numwoof = woof(20);
	c_assert(numwoof == 55.0);

	lua.script( "function f () return 10, 11, 12 end" );

	sol::function f = lua["f"];
	std::tuple<int, int, int> abc = f();
	c_assert(std::get<0>(abc) == 10);
	c_assert(std::get<1>(abc) == 11);
	c_assert(std::get<2>(abc) == 12);
	// or
	int a, b, c;
	sol::tie(a, b, c) = f();
	c_assert(a == 10);
	c_assert(b == 11);
	c_assert(c == 12);

	return 0;
}
