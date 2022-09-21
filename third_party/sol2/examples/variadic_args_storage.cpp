#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include <functional>

int main() {

	std::cout << "=== variadic_args serialization/storage ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	
	std::function<void()> function_storage;

	auto store_routine = [&function_storage] (sol::function f, sol::variadic_args va) {
		function_storage = [f, args = std::vector<sol::object>(va.begin(), va.end())]() {
			f(sol::as_args(args));
		};
	};

	lua.set_function("store_routine", store_routine);
	
	lua.script(R"(
function a(name)
	print(name)
end
store_routine(a, "some name")
)");
	function_storage();

	lua.script(R"(
function b(number, text)
	print(number, "of", text)
end
store_routine(b, 20, "these apples")
)");
	function_storage();

	std::cout << std::endl;

	return 0;
}
