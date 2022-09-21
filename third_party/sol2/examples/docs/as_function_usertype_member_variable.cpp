#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

int main () {
	class B {
	public:
		int bvar = 24;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<B>("B", 
		// bind as variable
		"b", &B::bvar,
		// bind as function
		"f", sol::as_function(&B::bvar)
	);

	B b;
	lua.set("b", &b);
	lua.script(R"(x = b:f()
		y = b.b
		assert(x == 24)
		assert(y == 24)
	)");

	return 0;
}
