#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

int main(int, char**) {
	std::cout << "=== script error handling ===" << std::endl;

	sol::state lua;

	std::string code = R"(
bad&$#*$syntax
bad.code = 2
return 24
)";

	/* OPTION 1 */
	// Handling code like this can be robust
	// If you disable exceptions, then obviously you would remove
	// the try-catch branches,
	// and then rely on the `lua_atpanic` function being called 
	// and trapping errors there before exiting the application
	{
		// script_default_on_error throws / panics when the code is bad: trap the error
		try {
			int value = lua.script(code, sol::script_default_on_error);
			// This will never be reached
			std::cout << value << std::endl;
			c_assert(value == 24);
		}
		catch (const sol::error& err) {
			std::cout << "Something went horribly wrong: thrown error" << "\n\t" << err.what() << std::endl;
		}
	}

	/* OPTION 2 */
	// Use the script_pass_on_error handler
	// this simply passes through the protected_function_result,
	// rather than throwing it or calling panic
	// This will check code validity and also whether or not it runs well
	{
		sol::protected_function_result result = lua.script(code, sol::script_pass_on_error);
		c_assert(!result.valid());
		if (!result.valid()) {
			sol::error err = result;
			sol::call_status status = result.status();
			std::cout << "Something went horribly wrong: " << sol::to_string(status) << " error" << "\n\t" << err.what() << std::endl;
		}
	}

	/* OPTION 3 */
	// This is a lower-level, more explicit way to load code
	// This explicitly loads the code, giving you access to any errors
	// plus the load status
	// then, it turns the loaded code into a sol::protected_function
	// which is then called so that the code can run
	// you can then check that too, for any errors
	// The two previous approaches are recommended
	{
		sol::load_result loaded_chunk = lua.load(code);
		c_assert(!loaded_chunk.valid());
		if (!loaded_chunk.valid()) {
			sol::error err = loaded_chunk;
			sol::load_status status = loaded_chunk.status();
			std::cout << "Something went horribly wrong loading the code: " << sol::to_string(status) << " error" << "\n\t" << err.what() << std::endl;
		}
		else {
			// Because the syntax is bad, this will never be reached
			c_assert(false);
			// If there is a runtime error (lua GC memory error, nil access, etc.)
			// it will be caught here
			sol::protected_function script_func = loaded_chunk;
			sol::protected_function_result result = script_func();
			if (!result.valid()) {
				sol::error err = result;
				sol::call_status status = result.status();
				std::cout << "Something went horribly wrong running the code: " << sol::to_string(status) << " error" << "\n\t" << err.what() << std::endl;
			}
		}
	}

	std::cout << std::endl;

	return 0;
}
