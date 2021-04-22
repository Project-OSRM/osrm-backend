#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

struct ree {
	int value = 1;
	ree() {}
	ree(int v) : value(v) {}
};

int main() {

	std::cout << "=== special pointers -- modify in place ===" << std::endl;

	sol::state lua;

	auto new_shared_ptr = [](sol::stack_reference obj) {
		// works just fine
		sol::stack::modify_unique_usertype(obj, [](std::shared_ptr<ree>& sptr) {
			sptr = std::make_shared<ree>(sptr->value + 1);
		});
	};

	auto reset_shared_ptr = [](sol::stack_reference obj) {
		sol::stack::modify_unique_usertype(obj, [](std::shared_ptr<ree>& sptr) {
			// THIS IS SUCH A BAD IDEA AAAGH
			sptr.reset();
			// DO NOT reset to nullptr:
			// change it to an actual NEW value...
			// otherwise you will inject a nullptr into the userdata representation...
			// which will NOT compare == to Lua's nil
		});
	};

	lua.set_function("f", new_shared_ptr);
	lua.set_function("f2", reset_shared_ptr);
	lua.set_function("g", [](ree* r) {
		std::cout << r->value << std::endl;
	});

	lua["p"] = std::make_shared<ree>();
	lua.script("g(p) -- okay");
	lua.script("f(p)");
	lua.script("g(p) -- okay");
	// uncomment the below for
	// segfault/read-access violation
	lua.script("f2(p)");
	//lua.script("g(p) -- kaboom");

	return 0;
}
