#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

// NOTE:
// There are TWO ways to retrieve the "this"
// object from calls, when it comes to constructors and regular member functions
// please pay attention to both: this is a low-level operation!

int main() {
	struct thing {

		thing(sol::this_state ts) {
			lua_State* L = ts;
			// references the object that called this function
			// in constructors:
			sol::stack_object selfobj(L, -1);
			// the -1 (NEGATIVE one) above 
			// means "off the top fo the stack"
			// (-1 is the top, -2 is one below, etc...)

			// definitely the same
			thing& self = selfobj.as<thing>();
			c_assert(&self == this);
		}

		void func(sol::this_state ts) const {
			lua_State* L = ts;
			// references the object that called this function
			// in regular member functions:
			sol::stack_object selfobj(L, 1);
			// "1" is the bottom of the Lua stack
			// 2 is one up, so on and so forth...
			thing& self = selfobj.as<thing>();

			// definitely the same
			c_assert(&self == this);
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<thing>("thing",
		sol::constructors<thing(sol::this_state)>(),
		"func", &thing::func
	);

	lua.script(R"(
obj = thing.new()
obj:func()
	)");

	return 0;
}
