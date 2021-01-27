#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

class vector {
public:
	double data[3];

	vector() : data{ 0,0,0 } {}

	double& operator[](int i) {
		return data[i];
	}


	static double my_index(vector& v, int i) {
		return v[i];
	}

	static void my_new_index(vector& v, int i, double x) {
		v[i] = x;
	}
};

int main () {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<vector>("vector", sol::constructors<sol::types<>>(),
		sol::meta_function::index, &vector::my_index,
		sol::meta_function::new_index, &vector::my_new_index);
	lua.script("v = vector.new()\n"
		"print(v[1])\n"
		"v[2] = 3\n"
		"print(v[2])\n"
	);

	vector& v = lua["v"];
	c_assert(v[0] == 0.0);
	c_assert(v[1] == 0.0);
	c_assert(v[2] == 3.0);

	return 0;
}
