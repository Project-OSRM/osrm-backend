#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <iostream>

struct something {
	int arr[4];

	something() : arr{ 5, 6, 7, 8 } {}
};

int main() {

	std::cout << "=== c arrays (works with Visual C++ too) ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<something>("something",
		"arr", sol::property([](something& s) {
			return std::ref(s.arr);
		})
	);
	lua.script(R"(s = something.new() 
		print(s.arr[3])
		s.arr[3] = 40
		print(s.arr[3])
	)");

	something& s = lua["s"];
	c_assert(s.arr[0] == 5);
	c_assert(s.arr[1] == 6);
	c_assert(s.arr[2] == 40);
	c_assert(s.arr[3] == 8);

	std::string string_array[] = {
		"first string",
		"second string",
		"third string",
	};
	lua["str_arr"] = std::ref(string_array);
	// or:
	// lua["str_array"] = &string_array;
	lua.script(R"(
		print(str_arr[3])
		str_arr[3] = str_arr[3] .. ': modified'
		print(str_arr[3])
	)");

	c_assert(string_array[2] == "third string: modified");

	return 0;
}
