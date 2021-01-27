#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

struct test {
	static int number;
};
int test::number = 25;


int main() {
	sol::state lua;
	lua.open_libraries();
	lua.new_usertype<test>("test",
		"direct", sol::var(2),
		"number", sol::var(test::number),
		"ref_number", sol::var(std::ref(test::number))
	);

	int direct_value = lua["test"]["direct"];
	c_assert(direct_value == 2);

	int number = lua["test"]["number"];
	c_assert(number == 25);
	int ref_number = lua["test"]["ref_number"];
	c_assert(ref_number == 25);
	
	test::number = 542;

	// number is its own memory: was passed by value
	// So does not change
	int number_again = lua["test"]["number"];
	c_assert(number_again == 25);

	// ref_number is just test::number 
	// passed through std::ref
	// so, it holds a reference
	// which can be updated
	int ref_number_again = lua["test"]["ref_number"];
	c_assert(ref_number_again == 542);
	// be careful about referencing local variables,
	// if they go out of scope but are still reference
	// you'll suffer dangling reference bugs!

	return 0;
}
