#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main () {

	std::cout << "=== safe_script usage ===" << std::endl;

	sol::state lua;
	// uses sol::script_default_on_error, which either panics or throws, 
	// depending on your configuration and compiler settings
	try {
		auto result1 = lua.safe_script("bad.code");
	}
	catch( const sol::error& e ) {
		std::cout << "an expected error has occurred: " << e.what() << std::endl;
	}
	
	// a custom handler that you write yourself
	// is only called when an error happens with loading or running the script
	auto result2 = lua.safe_script("123 bad.code", [](lua_State*, sol::protected_function_result pfr) {
		// pfr will contain things that went wrong, for either loading or executing the script
		// the user can do whatever they like here, including throw. Otherwise...
		sol::error err = pfr;
		std::cout << "An error (an expected one) occurred: " << err.what() << std::endl;

		// ... they need to return the protected_function_result
		return pfr;
	});

	std::cout << std::endl;

	return 0;
}
