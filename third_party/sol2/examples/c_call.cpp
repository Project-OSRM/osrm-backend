#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

int f1(int) { return 32; }

int f2(int, int) { return 1; }

struct fer {
	double f3(int, int) {
		return 2.5;
	}
};


int main() {

	sol::state lua;
	// overloaded function f
	lua.set("f", sol::c_call<sol::wrap<decltype(&f1), &f1>, sol::wrap<decltype(&f2), &f2>, sol::wrap<decltype(&fer::f3), &fer::f3>>);
	// singly-wrapped function
	lua.set("g", sol::c_call<sol::wrap<decltype(&f1), &f1>>);
	// without the 'sol::wrap' boilerplate
	lua.set("h", sol::c_call<decltype(&f2), &f2>);
	// object used for the 'fer' member function call
	lua.set("obj", fer());

	// call them like any other bound function
	lua.script("r1 = f(1)");
	lua.script("r2 = f(1, 2)");
	lua.script("r3 = f(obj, 1, 2)");
	lua.script("r4 = g(1)");
	lua.script("r5 = h(1, 2)");

	// get the results and see
	// if it worked out
	int r1 = lua["r1"];
	c_assert(r1 == 32);
	int r2 = lua["r2"];
	c_assert(r2 == 1);
	double r3 = lua["r3"];
	c_assert(r3 == 2.5);
	int r4 = lua["r4"];
	c_assert(r4 == 32);
	int r5 = lua["r5"];
	c_assert(r5 == 1);

	return 0;
}
