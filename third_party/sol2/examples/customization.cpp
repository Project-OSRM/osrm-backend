#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include "assert.hpp"

struct two_things {
	int a;
	bool b;
};

namespace sol {

	// First, the expected size
	// Specialization of a struct
	template <>
	struct lua_size<two_things> : std::integral_constant<int, 2> {};

	// Then, specialize the type
	// this makes sure Sol can return it properly
	template <>
	struct lua_type_of<two_things> : std::integral_constant<sol::type, sol::type::poly> {};

	// Now, specialize various stack structures
	namespace stack {

		template <>
		struct checker<two_things> {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				// indices can be negative to count backwards from the top of the stack,
				// rather than the bottom up
				// to deal with this, we adjust the index to
				// its absolute position using the lua_absindex function 
				int absolute_index = lua_absindex(L, index);
				// Check first and second second index for being the proper types
				bool success = stack::check<int>(L, absolute_index, handler)
					&& stack::check<bool>(L, absolute_index + 1, handler);
				tracking.use(2);
				return success;
			}
		};

		template <>
		struct getter<two_things> {
			static two_things get(lua_State* L, int index, record& tracking) {
				int absolute_index = lua_absindex(L, index);
				// Get the first element
				int a = stack::get<int>(L, absolute_index);
				// Get the second element, 
				// in the +1 position from the first
				bool b = stack::get<bool>(L, absolute_index + 1);
				// we use 2 slots, each of the previous takes 1
				tracking.use(2);
				return two_things{ a, b };
			}
		};

		template <>
		struct pusher<two_things> {
			static int push(lua_State* L, const two_things& things) {
				int amount = stack::push(L, things.a);
				// amount will be 1: int pushes 1 item
				amount += stack::push(L, things.b);
				// amount 2 now, since bool pushes a single item
				// Return 2 things
				return amount;
			}
		};

	}
}

int main() {
	std::cout << "=== customization ===" << std::endl;
	std::cout << std::boolalpha;
	
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// Create a pass-through style of function
	lua.script("function f ( a, b ) print(a, b) return a, b end");

	// get the function out of Lua
	sol::function f = lua["f"];

	two_things things = f(two_things{ 24, false });
	c_assert(things.a == 24);
	c_assert(things.b == false);
	// things.a == 24
	// things.b == true

	std::cout << "things.a: " << things.a << std::endl;
	std::cout << "things.b: " << things.b << std::endl;
	std::cout << std::endl;

	return 0;
}
