#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main () {

	const auto& code = R"(
	bark_power = 11;

	function got_problems( error_msg )
		return "got_problems handler: " .. error_msg
	end

	function woof ( bark_energy )
		if bark_energy < 20 then
			error("*whine*")
		end
		return (bark_energy * (bark_power / 4))
	end

	function woofers ( bark_energy )
		if bark_energy < 10 then
			error("*whine*")
		end
		return (bark_energy * (bark_power / 4))
	end
	)";

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	
	lua.script(code);

	sol::protected_function problematic_woof = lua["woof"];
	problematic_woof.error_handler = lua["got_problems"];

	auto firstwoof = problematic_woof(20);
	if ( firstwoof.valid() ) {
		// Can work with contents
		double numwoof = firstwoof;
		std::cout << "Got value: " << numwoof << std::endl;
	}
	else{
		// An error has occured
		sol::error err = firstwoof;
		std::string what = err.what();
		std::cout << what << std::endl;
	}

	// errors, calls handler and then returns a string error from Lua at the top of the stack
	auto secondwoof = problematic_woof(19);
	if (secondwoof.valid()) {
		// Call succeeded
		double numwoof = secondwoof;
		std::cout << "Got value: " << numwoof << std::endl;
	}
	else {
		// Call failed
		// Note that if the handler was successfully called, this will include
		// the additional appended error message information of
		// "got_problems handler: " ...
		sol::error err = secondwoof;
		std::string what = err.what();
		std::cout << what << std::endl;
	}

	// can also use optional to tell things
	sol::optional<double> maybevalue = problematic_woof(19);
	if (maybevalue) {
		// Have a value, use it
		double numwoof = maybevalue.value();
		std::cout << "Got value: " << numwoof << std::endl;
	}
	else {
		std::cout << "No value!" << std::endl;
	}

	std::cout << std::endl;

	return 0;
}
