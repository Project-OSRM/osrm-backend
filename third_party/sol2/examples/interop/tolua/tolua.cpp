#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1 // MUST be defined to use interop features
#include <sol.hpp>

#include "Player.h"
#include <tolua++.h>
// pick or replace the include
// with whatever generated file you've created
#include "tolua_Player.h"

#include <iostream>
#include "../../assert.hpp"

// tolua code lifted from some blog, if the link dies
// I don't know where else you're gonna find the reference,
// http://usefulgamedev.weebly.com/tolua-example.html

namespace sol {
namespace stack {
	template <typename T>
	struct userdata_checker<extensible<T>> {
		template <typename Handler>
		static bool check(lua_State* L, int relindex, type index_type, Handler&& handler, record& tracking) {
			tracking.use(1);
			// just marking unused parameters for no compiler warnings
			(void)index_type;
			(void)handler;
			int index = lua_absindex(L, relindex);
			std::string name = sol::detail::short_demangle<T>();
			tolua_Error tolua_err;
			return tolua_isusertype(L, index, name.c_str(), 0, &tolua_err);
		}
	};
}
} // namespace sol::stack

void register_sol_stuff(lua_State* L) {
	// grab raw state and put into state_view
	// state_view is cheap to construct
	sol::state_view lua(L);
	// bind and set up your things: everything is entirely self-contained
	lua["f"] = sol::overload(
		[](Player& from_tolua) {
			std::cout << "calling 1-argument version with tolua-created Player { health:" << from_tolua.getHealth() << " }" << std::endl;
			c_assert(from_tolua.getHealth() == 4);
		},
		[](Player& from_tolua, int second_arg) {
			std::cout << "calling 2-argument version with tolua-created Player { health: " << from_tolua.getHealth() << " } and integer argument of " << second_arg << std::endl;
			c_assert(from_tolua.getHealth() == 4);
			c_assert(second_arg == 5);
		});
}

void check_with_sol(lua_State* L) {
	sol::state_view lua(L);
	Player& obj = lua["obj"];
	(void)obj;
	c_assert(obj.getHealth() == 4);
}

int main(int, char* []) {

	std::cout << "=== interop example (tolua) ===" << std::endl;
	std::cout << "(code lifted from a sol2 user's use case: https://github.com/ThePhD/sol2/issues/511#issuecomment-331729884)" << std::endl;

	lua_State* L = luaL_newstate();

	luaL_openlibs(L);	// initalize all lua standard library functions
	tolua_open(L);		  // initalize tolua
	tolua_Player_open(L); // make Player class accessible from LUA


	register_sol_stuff(L);

	const auto code = R"(
obj = Player:new()
obj:setHealth(4)

f(obj) -- call 1 argument version
f(obj, 5) -- call 2 argument version
)";

	if (luaL_dostring(L, code)) {
		lua_error(L); // crash on error
	}

	check_with_sol(L);

	return 0;
}