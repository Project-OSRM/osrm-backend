#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"
#include <iostream>

inline int my_add(int x, int y) {
	return x + y;
}

struct multiplier {
    int operator()(int x) {
        return x * 10;
    }

    static int by_five(int x) {
        return x * 5;
    }
};

int main() {
	std::cout << "=== functions ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// setting a function is simple
	lua.set_function("my_add", my_add);

	// you could even use a lambda
	lua.set_function("my_mul", [](double x, double y) { return x * y; });

	// member function pointers and functors as well
	lua.set_function("mult_by_ten", multiplier{});
	lua.set_function("mult_by_five", &multiplier::by_five);

	// assert that the functions work
	lua.script("assert(my_add(10, 11) == 21)");
	lua.script("assert(my_mul(4.5, 10) == 45)");
	lua.script("assert(mult_by_ten(50) == 500)");
	lua.script("assert(mult_by_five(10) == 50)");

	// using lambdas, functions can have state.
	int x = 0;
	lua.set_function("inc", [&x]() { x += 10; });

	// calling a stateful lambda modifies the value
	lua.script("inc()");
	c_assert(x == 10);
	if (x == 10) {
		// Do something based on this information
		std::cout << "Yahoo! x is " << x << std::endl;
	}

	// this can be done as many times as you want
	lua.script(R"(
inc()
inc()
inc()
)");
	c_assert(x == 40);
	if (x == 40) {
		// Do something based on this information
		std::cout << "Yahoo! x is " << x << std::endl;
	}

	// retrieval of a function is done similarly
	// to other variables, using sol::function
	sol::function add = lua["my_add"];
	int value = add(10, 11);
	// second way to call the function
	int value2 = add.call<int>(10, 11);
	c_assert(value == 21);
	c_assert(value2 == 21);
	if (value == 21 && value2 == 21) {
		std::cout << "Woo, value is 21!" << std::endl;
	}

	std::cout << std::endl;

	return 0;
}