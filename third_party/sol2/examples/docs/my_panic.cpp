#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>
#include <iostream>

inline void my_panic(sol::optional<std::string> maybe_msg) {
	std::cerr << "Lua is in a panic state and will now abort() the application" << std::endl;
	if (maybe_msg) {
		const std::string& msg = maybe_msg.value();
		std::cerr << "\terror message: " << msg << std::endl;
	}
	// When this function exits, Lua will exhibit default behavior and abort()
}

int main (int, char*[]) {
	sol::state lua(sol::c_call<decltype(&my_panic), &my_panic>);
	// or, if you already have a lua_State* L
	// lua_atpanic( L, sol::c_call<decltype(&my_panic)>, &my_panic> );
	// or, with state/state_view:
	// sol::state_view lua(L);
	// lua.set_panic( sol::c_call<decltype(&my_panic)>, &my_panic> );

	// uncomment the below to see
	//lua.script("boom_goes.the_dynamite");

	return 0;
}
