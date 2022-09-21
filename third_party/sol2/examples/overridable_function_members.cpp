#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main() {
	std::cout << "=== override-able member functions ===" << std::endl;

	struct thingy {
		sol::function paint;

		thingy(sol::this_state L) : paint(sol::make_reference<sol::function>(L.lua_state(), &thingy::default_paint)) {
		}

		void default_paint() {
			std::cout << "p" << std::endl;
		}

	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<thingy>("thingy",
		sol::constructors<thingy(sol::this_state)>(),
		"paint", &thingy::paint);

	sol::string_view code = R"(
obj = thingy.new()
obj:paint()
obj.paint = function (self) print("g") end
obj:paint()
function obj:paint () print("s") end
obj:paint()
)";

	lua.safe_script(code);

	std::cout << std::endl;

	return 0;
}