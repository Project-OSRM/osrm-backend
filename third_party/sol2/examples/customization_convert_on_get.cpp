#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include <iomanip>
#include "assert.hpp"

struct number_shim {
	double num = 0;
};

namespace sol {

	template <>
	struct lua_type_of<number_shim> : std::integral_constant<sol::type, sol::type::poly> {};

	namespace stack {
		template <>
		struct checker<number_shim> {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				// check_usertype is a backdoor for directly checking sol2 usertypes
				if (!check_usertype<number_shim>(L, index) && !stack::check<double>(L, index)) {
					handler(L, index, type_of(L, index), type::userdata, "expected a number_shim or a number");
					return false;
				}
				tracking.use(1);
				return true;
			}
		};

		template <>
		struct getter<number_shim> {
			static number_shim get(lua_State* L, int index, record& tracking) {
				if (check_usertype<number_shim>(L, index)) {
					number_shim& ns = get_usertype<number_shim>(L, index, tracking);
					return ns;
				}
				number_shim ns{};
				ns.num = stack::get<double>(L, index, tracking);
				return ns;
			}
		};

	} // namespace stack
} // namespace sol

int main() {
	sol::state lua;

	// Create a pass-through style of function
	lua.safe_script("function f ( a ) return a end");
	lua.set_function("g", [](double a) {
		number_shim ns;
		ns.num = a;
		return ns;
	});

	lua.script("vf = f(25) vg = g(35)");
	
	number_shim thingsf = lua["vf"];
	number_shim thingsg = lua["vg"];

	c_assert(thingsf.num == 25);
	c_assert(thingsg.num == 35);

	return 0;
}
