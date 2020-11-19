#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1 // MUST be defined to use interop features
#include <sol.hpp>

#include <LuaBridge/LuaBridge.h>

#include <iostream>
#include "../../assert.hpp"

// LuaBridge,
// no longer maintained, by VinnieFalco:
// https://github.com/vinniefalco/LuaBridge

struct A {

	A(int v)
	: v_(v) {
	}

	void print() {
		std::cout << "called A::print" << std::endl;
	}

	int value() const {
		return v_;
	}

private:
	int v_ = 50;
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
			tracking.use(1);
			int index = lua_absindex(L, relindex);
			T* corrected = luabridge::Userdata::get<T>(L, index, true);
			return corrected != nullptr;
		}
	};

	template <typename T>
	struct userdata_getter<extensible<T>> {
		static std::pair<bool, T*> get(lua_State* L, int relindex, void* unadjusted_pointer, record& tracking) {
			(void)unadjusted_pointer;
			int index = lua_absindex(L, relindex);
			if (!userdata_checker<extensible<T>>::check(L, index, type::userdata, no_panic, tracking)) {
				return { false, nullptr };
			}
			T* corrected = luabridge::Userdata::get<T>(L, index, true);
			return { true, corrected };
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
		[](A& from_luabridge) {
			std::cout << "calling 1-argument version with luabridge-created A { " << from_luabridge.value() << " }" << std::endl;
			c_assert(from_luabridge.value() == 24);
		},
		[](A& from_luabridge, int second_arg) {
			std::cout << "calling 2-argument version with luabridge-created A { " << from_luabridge.value() << " } and integer argument of " << second_arg << std::endl;
			c_assert(from_luabridge.value() == 24);
			c_assert(second_arg == 5);
		});
}

void check_with_sol(lua_State* L) {
	sol::state_view lua(L);
	A& obj = lua["obj"];
	(void)obj;
	c_assert(obj.value() == 24);
}

int main(int, char* []) {

	std::cout << "=== interop example (LuaBridge) ===" << std::endl;
	std::cout << "code modified from LuaBridge's examples: https://github.com/vinniefalco/LuaBridge" << std::endl;

	struct closer {
		void operator()(lua_State* L) {
			lua_close(L);
		}
	};

	std::unique_ptr<lua_State, closer> state(luaL_newstate());
	lua_State* L = state.get();
	luaL_openlibs(L);
	
	luabridge::getGlobalNamespace(L)
		.beginNamespace("test")
		.beginClass<A>("A")
		.addConstructor<void (*)(int)>()
		.addFunction("print", &A::print)
		.addFunction("value", &A::value)
		.endClass()
		.endNamespace();

	register_sol_stuff(L);


	if (luaL_dostring(L, R"(
obj = test.A(24)
f(obj) -- call 1 argument version
f(obj, 5) -- call 2 argument version
)")) {
		lua_error(L);
	}

	check_with_sol(L);

	return 0;
}