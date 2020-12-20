#include <kaguya/kaguya.hpp>

#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1 // MUST be defined to use interop features
#include <sol.hpp>

#include <iostream>
#include "../../assert.hpp"

// kaguya code lifted from README.md,
// written by satoren:
// https://github.com/satoren/kaguya
// Copyright satoren
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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
			// use kaguya's own detail wrapper check to see if it's correct
			bool is_correct_type = kaguya::detail::object_wrapper_type_check(L, index);
			return is_correct_type;
		}
	};

	template <typename T>
	struct userdata_getter<extensible<T>> {
		static std::pair<bool, T*> get(lua_State* L, int relindex, void* unadjusted_pointer, record& tracking) {
			// you may not need to specialize this method every time:
			// some libraries are compatible with sol2's layout

			// kaguya's storage of data is incompatible with sol's
			// it stores the data directly in the pointer, not a pointer inside of the `void*`
			// therefore, leave the raw userdata pointer as-is,
			// if it's of the right type
			int index = lua_absindex(L, relindex);
			if (!kaguya::detail::object_wrapper_type_check(L, index)) {
				return { false, nullptr };
			}
			// using 1 element
			tracking.use(1);
			kaguya::ObjectWrapperBase* base = kaguya::object_wrapper(L, index);
			return { true, static_cast<T*>(base->get()) };
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
		[](ABC& from_kaguya) {
			std::cout << "calling 1-argument version with kaguya-created ABC { " << from_kaguya.value() << " }" << std::endl;
			c_assert(from_kaguya.value() == 24);
		},
		[](ABC& from_kaguya, int second_arg) {
			std::cout << "calling 2-argument version with kaguya-created ABC { " << from_kaguya.value() << " } and integer argument of " << second_arg << std::endl;
			c_assert(from_kaguya.value() == 24);
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

	std::cout << "=== interop example (kaguya) ===" << std::endl;
	std::cout << "(code lifted from kaguya's README examples: https://github.com/satoren/kaguya)" << std::endl;

	kaguya::State state;

	state["ABC"].setClass(kaguya::UserdataMetatable<ABC>()
						  .setConstructors<ABC(), ABC(int)>()
						  .addFunction("get_value", &ABC::value)
						  .addFunction("set_value", &ABC::setValue)
						  .addOverloadedFunctions("overload", &ABC::overload1, &ABC::overload2)
						  .addStaticFunction("nonmemberfun", [](ABC* self, int) { return 1; }));


	register_sol_stuff(state.state());

	state.dostring(R"(
obj = ABC.new(24)
f(obj) -- call 1 argument version
f(obj, 5) -- call 2 argument version
)");

	check_with_sol(state.state());

	return 0;
}