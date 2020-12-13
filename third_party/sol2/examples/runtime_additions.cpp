#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

struct object {
	int value = 0;
};

int main(int, char*[]) {
	std::cout << "=== runtime_additions ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<object>("object");

	// runtime additions: through the sol API
	lua["object"]["func"] = [](object& o) { 
		++o.value; 
		return o.value; 
	};
	// runtime additions: through a lua script
	lua.script(R"(
function object:print () 
	print(self:func())
end
	)");

	// see it work
	lua.script(R"(
obj = object.new()
obj:print()
	)");

	object& obj = lua["obj"];
	c_assert(obj.value == 1);

	return 0;
}
