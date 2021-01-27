#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <unordered_set>
#include <iostream>

int main() {
	struct hasher {
		typedef std::pair<std::string, std::string> argument_type;
		typedef std::size_t result_type;

		result_type operator()(const argument_type& p) const {
			return std::hash<std::string>()(p.first);
		}
	};

	using my_set = std::unordered_set<std::pair<std::string, std::string>, hasher>;

	std::cout << "=== containers with std::pair<> ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", []() {
		return my_set{ { "key1", "value1" },{ "key2", "value2" },{ "key3", "value3" } };
	});

	lua.safe_script("v = f()");
	lua.safe_script("print('v:', v)");
	lua.safe_script("print('#v:', #v)");
	// note that using my_obj:pairs() is a
	// way around pairs(my_obj) not working in Lua 5.1/LuaJIT: try it!
	lua.safe_script("for k,v1,v2 in v:pairs() do print(k, v1, v2) end");

	std::cout << std::endl;

	return 0;
}
