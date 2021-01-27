#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

void test_environment(std::string key, const sol::environment& env, const sol::state_view& lua) {
	sol::optional<int> maybe_env_a = env[key];
	sol::optional<int> maybe_global_a = lua[key];
	if (maybe_global_a) {
		std::cout << "\t'" << key << "' is " << maybe_global_a.value() << " in the global environment" << std::endl;
	}
	else {
		std::cout << "\t'" << key << "' does not exist in the global environment" << std::endl;
	}
	if (maybe_env_a) {
		std::cout << "\t'" << key << "' is " << maybe_env_a.value() << " in target environment" << std::endl;
	}
	else {
		std::cout << "\t'" << key << "' does not exist in target environment" << std::endl;
	}
}

int main(int, char**) {
	std::cout << "=== environments ===" << std::endl;

	sol::state lua;
	// A global variable to see if we can "fallback" into it
	lua["b"] = 2142;

	// Create a new environment
	sol::environment plain_env(lua, sol::create);
	// Use it
	lua.script("a = 24", plain_env);
	std::cout << "-- target: plain_env" << std::endl;
	test_environment("a", plain_env, lua);
	test_environment("b", plain_env, lua);
	std::cout << std::endl;

	// Create an environment with a fallback
	sol::environment env_with_fallback(lua, sol::create, lua.globals());
	// Use it
	lua.script("a = 56", env_with_fallback);
	std::cout << "-- target: env_with_fallback (fallback is global table)" << std::endl;
	test_environment("a", env_with_fallback, lua);
	test_environment("b", env_with_fallback, lua);
	std::cout << std::endl;


	return 0;
}
