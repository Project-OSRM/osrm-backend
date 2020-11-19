#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

struct object { 
	int value = 0; 
};

int main (int, char*[]) {

	std::cout << "==== runtime_extension =====" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<object>( "object" );

	// runtime additions: through the sol API
	lua["object"]["func"] = [](object& o) { return o.value; };
	// runtime additions: through a lua script
	lua.script("function object:print () print(self:func()) end");
	
	// see it work
	lua.script("local obj = object.new() \n obj:print()");

	return 0;
}
