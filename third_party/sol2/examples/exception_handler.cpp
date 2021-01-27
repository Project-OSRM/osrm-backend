#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <iostream>

int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
	// L is the lua state, which you can wrap in a state_view if necessary
	// maybe_exception will contain exception, if it exists
	// description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
	std::cout << "An exception occurred in a function, here's what it says ";
	if (maybe_exception) {
		std::cout << "(straight from the exception): ";
		const std::exception& ex = *maybe_exception;
		std::cout << ex.what() << std::endl;
	}
	else {
		std::cout << "(from the description parameter): ";
		std::cout.write(description.data(), description.size());
		std::cout << std::endl;
	}

	// you must push 1 element onto the stack to be 
	// transported through as the error object in Lua
	// note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
	// so we push a single string (in our case, the description of the error)
	return sol::stack::push(L, description);
}

void will_throw() {
	throw std::runtime_error("oh no not an exception!!!");
}

int main() {
	std::cout << "=== exception_handler ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.set_exception_handler(&my_exception_handler);

	lua.set_function("will_throw", &will_throw);

	sol::protected_function_result pfr = lua.safe_script("will_throw()", &sol::script_pass_on_error);

	c_assert(!pfr.valid());
	
	sol::error err = pfr;
	std::cout << err.what() << std::endl;

	std::cout << std::endl;

	return 0;
}
