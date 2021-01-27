#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

int main(int, char*[]) {
	std::cout << "=== functions empty args ===" << std::endl;

	// sol::reference, sol::Stack_reference,
	// sol::object (and main_* types) can all be
	// used to capture "nil", or "none" when a function
	// leaves it off
	auto my_defaulting_function = [](sol::object maybe_defaulted) -> int {
		// if it's nil, it's "unused" or "inactive"
		bool inactive = maybe_defaulted == sol::lua_nil;
		if (inactive) {
			return 0;
		}
		if (maybe_defaulted.is<int>()) {
			int value = maybe_defaulted.as<int>();
			return value;
		}
		return 1;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// copy function in (use std::ref to change this behavior)
	lua.set_function("defaulting_function", my_defaulting_function);

	sol::string_view code = R"(
		result = defaulting_function(24)
		result_nothing = defaulting_function()
		result_nil = defaulting_function(nil)
		result_string = defaulting_function('meow')
		print('defaulting_function(24), returned:', result)
		print('defaulting_function(), returned:', result_nothing)
		print('defaulting_function(nil), returned:', result_nil)
		print('defaulting_function(\'meow\'), returned:', result_string)
		assert(result == 24)
		assert(result_nothing == 0)
		assert(result_nil == 0)
		assert(result_string == 1)
	)";

	lua.safe_script(code);

	std::cout << std::endl;

	return 0;
}