#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <vector>

int main (int, char*[]) {
	
	sol::state lua;
	lua.open_libraries();
	lua.set("my_table", sol::as_table(std::vector<int>{ 1, 2, 3, 4, 5 }));
	lua.script("for k, v in ipairs(my_table) do print(k, v) assert(k == v) end");

	return 0;
}
