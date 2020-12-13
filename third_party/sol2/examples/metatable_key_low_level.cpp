#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int main () {

	struct bark {
		int operator()(int x) {
			return x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<bark>("bark",
		sol::meta_function::call_function, &bark::operator()
	);

	bark b;
	lua.set("b", &b);

	sol::table b_as_table = lua["b"];		
	sol::table b_metatable = b_as_table[sol::metatable_key];
	sol::function b_call = b_metatable["__call"];
	sol::function b_as_function = lua["b"];

	int result1 = b_as_function(1);
	// pass 'self' directly to argument
	int result2 = b_call(b, 1);
	c_assert(result1 == result2);
	c_assert(result1 == 1);
	c_assert(result2 == 1);
}
