#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "../../assert.hpp"

int main(int, char* []) {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua["abc_sol2"] = lua.create_table_with(
		0, 24
	);

	sol::table inner_table = lua.create_table_with("bark", 50,
		// can reference other existing stuff too
		"woof", lua["abc_sol2"]
	);
	lua.create_named_table("def_sol2",
		"ghi", inner_table
	);

	std::string code = R"(
		abc = { [0] = 24 }
		def = {
			ghi = {
				bark = 50,
				woof = abc
			}
		}
	)";

	lua.script(code);
	lua.script(R"(
		assert(abc_sol2[0] == abc[0])
		assert(def_sol2.ghi.bark == def.ghi.bark)
	)");

	return 0;
}
