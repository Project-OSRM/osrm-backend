#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int main(int, char*[]) {
	std::cout << "=== usertype call from C++ ===" << std::endl;
	
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	struct cpp_object {
		int value = 5;
	};

	struct test {
		int value = 0;

		int func(const cpp_object& obj) {
			std::cout << "func\t" << obj.value << std::endl;
			value += obj.value;
			return value;
		}
	};

	lua.new_usertype<cpp_object>("test",
		"value", &cpp_object::value
		);
	lua.new_usertype<test>("test",
		"func", &test::func
		);
	lua.script("function test:lua_func(obj) print('lua_func', obj.value) end");

	lua["obj"] = test{};
	cpp_object cppobj;

	lua["obj"]["func"](lua["obj"], cppobj);
	lua["obj"]["lua_func"](lua["obj"], cppobj);

	lua["test"]["func"](lua["obj"], cppobj);
	lua["test"]["lua_func"](lua["obj"], cppobj);

	// crashes
	//lua["obj"]["func"](cppobj);
	//lua["obj"]["lua_func"](cppobj);

	// crashes
	//lua["test"]["func"](cppobj);
	//lua["test"]["lua_func"](cppobj);

	return 0;
}