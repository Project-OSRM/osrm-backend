#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

sol::variadic_results call_it(sol::object function_name, sol::variadic_args args, sol::this_environment env, sol::this_state L) {
	sol::state_view lua = L;
	// default to global table as environment
	sol::environment function_environment = lua.globals();
	if (env) {
		// if we have an environment, use that instead
		function_environment = env;
	}

	// get and call the function
	sol::protected_function pf = function_environment[function_name];
	sol::protected_function_result res = pf(args);

	//
	sol::variadic_results results;
	if (!res.valid()) {
		// something went wrong: log/crash/whatever
		return results;
	}
	int returncount = res.return_count();
	for (int i = 0; i < returncount; i++) {
		// pass offset to get the object that was returned
		sol::object obj = res.get<sol::object>(i);
		results.push_back(obj);
	}
	// return the results
	return results;
}

int main() {
	std::cout << "=== indirect function calls ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua["call_it"] = call_it;

	// some functions to call
	lua.script(R"(
function add (a, b)
	return a + b;
end

function subtract (a, b)
	return a - b;
end

function log (x)
	print(x)
end
)");

	// call the functions indirectly, using a name
	lua.script(R"(
		call_it("log", "hiyo")
		call_it("log", 24)
		subtract_result = call_it("subtract", 5, 1)
		add_result = call_it("add", 5, 1)
	)");

	int subtract_result = lua["subtract_result"];
	int add_result = lua["add_result"];

	c_assert(add_result == 6);
	c_assert(subtract_result == 4);

	std::cout << std::endl;
	return 0;
}
