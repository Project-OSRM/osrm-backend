#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "../../assert.hpp"
#include <iostream>

void some_function() {
	std::cout << "some function!" << std::endl;
}

void some_other_function() {
	std::cout << "some other function!" << std::endl;
}

struct some_class {
	int variable = 30;

	double member_function() {
		return 24.5;
	}
};

int main(int, char*[]) {
	std::cout << "=== functions (all) ===" << std::endl;
	
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// put an instance of "some_class" into lua
	// (we'll go into more detail about this later
	// just know here that it works and is
	// put into lua as a userdata
	lua.set("sc", some_class());

	// binds a plain function
	lua["f1"] = some_function;
	lua.set_function("f2", &some_other_function);

	// binds just the member function
	lua["m1"] = &some_class::member_function;

	// binds the class to the type
	lua.set_function("m2", &some_class::member_function, some_class{});

	// binds just the member variable as a function
	lua["v1"] = &some_class::variable;

	// binds class with member variable as function
	lua.set_function("v2", &some_class::variable, some_class{});

	lua.script(R"(
	f1() -- some function!
	f2() -- some other function!
	
	-- need class instance if you don't bind it with the function
	print(m1(sc)) -- 24.5
	-- does not need class instance: was bound to lua with one 
	print(m2()) -- 24.5
	
	-- need class instance if you 
	-- don't bind it with the function
	print(v1(sc)) -- 30
	-- does not need class instance: 
	-- it was bound with one 
	print(v2()) -- 30

	-- can set, still 
	-- requires instance
	v1(sc, 212)
	-- can set, does not need 
	-- class instance: was bound with one 
	v2(254)

	print(v1(sc)) -- 212
	print(v2()) -- 254
	)");

	std::cout << std::endl;

	return 0;
}
