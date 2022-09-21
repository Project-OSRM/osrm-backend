#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1 // MUST be defined to use interop features
#include <sol.hpp>

#include <luwra.hpp>

#include <iostream>
#include "../../assert.hpp"

// luwra,
// another C++ wrapper library:
// https://github.com/vapourismo/luwra

struct ABC {
	ABC()
	: v_(0) {
	}
	ABC(int value)
	: v_(value) {
	}
	int value() const {
		return v_;
	}
	void setValue(int v) {
		v_ = v;
	}
	void overload1() {
		std::cout << "call overload1" << std::endl;
	}
	void overload2(int) {
		std::cout << "call overload2" << std::endl;
	}

private:
	int v_;
};

namespace sol {
namespace stack {
	template <typename T>
	struct userdata_checker<extensible<T>> {
		template <typename Handler>
		static bool check(lua_State* L, int relindex, type index_type, Handler&& handler, record& tracking) {
			// just marking unused parameters for no compiler warnings
			(void)index_type;
			(void)handler;
			// using 1 element
			tracking.use(1);
			int index = lua_absindex(L, relindex);
			if (lua_getmetatable(L, index) == 1) {
				luaL_getmetatable(L, luwra::internal::UserTypeReg<T>::name.c_str());
				bool is_correct_type = lua_rawequal(L, -2, -1) == 1;
				lua_pop(L, 2);
				return is_correct_type;
			}
			return false;
		}
	};

	template <typename T>
	struct userdata_getter<extensible<T>> {
		static std::pair<bool, T*> get(lua_State* L, int relindex, void* unadjusted_pointer, record& tracking) {
			// you may not need to specialize this method every time:
			// some libraries are compatible with sol2's layout
			int index = lua_absindex(L, relindex);
			if (!userdata_checker<extensible<T>>::check(L, index, type::userdata, no_panic, tracking)) {
				return { false, nullptr };
			}
			return { true, static_cast<T*>(unadjusted_pointer) };
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
		[](ABC& from_luwra) {
			std::cout << "calling 1-argument version with luwra-created ABC { " << from_luwra.value() << " }" << std::endl;
			c_assert(from_luwra.value() == 24);
		},
		[](ABC& from_luwra, int second_arg) {
			std::cout << "calling 2-argument version with luwra-created ABC { " << from_luwra.value() << " } and integer argument of " << second_arg << std::endl;
			c_assert(from_luwra.value() == 24);
			c_assert(second_arg == 5);
		});
}

void check_with_sol(lua_State* L) {
	sol::state_view lua(L);
	ABC& obj = lua["obj"];
	(void)obj;
	c_assert(obj.value() == 24);
}

int main(int, char* []) {

	std::cout << "=== interop example (luwra) ===" << std::endl;
	std::cout << "code modified from luwra's documentation examples: https://github.com/vapourismo/luwra" << std::endl;

	luwra::StateWrapper state;

	state.registerUserType<ABC(int)>("ABC", { LUWRA_MEMBER(ABC, value), LUWRA_MEMBER(ABC, setValue) }, {});

	register_sol_stuff(state);

	state.runString(R"(
obj = ABC(24)
f(obj) -- call 1 argument version
f(obj, 5) -- call 2 argument version
)");

	check_with_sol(state);

	return 0;
}