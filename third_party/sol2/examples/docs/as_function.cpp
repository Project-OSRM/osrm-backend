#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

int main () {
	struct callable {
		int operator()( int a, bool b ) {
			return a + b ? 10 : 20;
		}
	};


	sol::state lua;
	// Binds struct as userdata
	// can still be callable, but beware
	// caveats
	lua.set( "not_func", callable() );
	// Binds struct as function
	lua.set( "func", sol::as_function( callable() ) );
	// equivalent: lua.set_function( "func", callable() );
	// equivalent: lua["func"] = callable();
}
