#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include "assert.hpp"
#include <cmath>

// Note that this is a bunch of if/switch statements
// for the sake of brevity and clarity
// A much more robust implementation could use a std::unordered_map
// to link between keys and desired actions for those keys on
// setting/getting
// The sky becomes the limit when you have this much control,
// so apply it wisely!

struct vec {
	double x;
	double y;

	vec() : x(0), y(0) {}
	vec(double x) : vec(x, x) {}
	vec(double x, double y) : x(x), y(y) {}

	sol::object getter(sol::stack_object key, sol::this_state L) {
		// we use stack_object for the arguments because we know
		// the values from Lua will remain on Lua's stack,
		// so long we we don't mess with it
		auto maybe_string_key = key.as<sol::optional<std::string>>();
		if (maybe_string_key) {
			const std::string& k = *maybe_string_key;
			if (k == "x") {
				// Return x
				return sol::object(L, sol::in_place, this->x);
			}
			else if (k == "y") {
				// Return y
				return sol::object(L, sol::in_place, this->y);
			}
		}

		// String keys failed, check for numbers
		auto maybe_numeric_key = key.as<sol::optional<int>>();
		if (maybe_numeric_key) {
			int n = *maybe_numeric_key;
			switch (n) {
			case 0:
				return sol::object(L, sol::in_place, this->x);
			case 1:
				return sol::object(L, sol::in_place, this->y);
			default:
				break;
			}
		}

		// No valid key: push nil
		// Note that the example above is a bit unnecessary:
		// binding the variables x and y to the usertype
		// would work just as well and we would not
		// need to look it up here,
		// but this is to show it works regardless
		return sol::object(L, sol::in_place, sol::lua_nil);
	}

	void setter(sol::stack_object key, sol::stack_object value, sol::this_state) {
		// we use stack_object for the arguments because we know
		// the values from Lua will remain on Lua's stack,
		// so long we we don't mess with it
		auto maybe_string_key = key.as<sol::optional<std::string>>();
		if (maybe_string_key) {
			const std::string& k = *maybe_string_key;
			if (k == "x") {
				// set x
				this->x = value.as<double>();
			}
			else if (k == "y") {
				// set y
				this->y = value.as<double>();
			}
		}

		// String keys failed, check for numbers
		auto maybe_numeric_key = key.as<sol::optional<int>>();
		if (maybe_numeric_key) {
			int n = *maybe_numeric_key;
			switch (n) {
			case 0:
				this->x = value.as<double>();
				break;
			case 1:
				this->y = value.as<double>();
				break;
			default:
				break;
			}
		}
	}
};

int main() {
	std::cout << "=== usertype dynamic getter/setter ===" << std::endl;

	sol::state lua;
	lua.open_libraries();

	lua.new_usertype<vec>("vec",
		sol::constructors<vec(), vec(double), vec(double, double)>(),
		// index and newindex control what happens when a "key"
		// is looked up that is not baked into the class itself
		// it is called with the object and the key for index (get)s
		// or it is called with the object, the key, and the index (set)
		// we can use a member function to assume the "object" is of the `vec`
		// type, and then just have a function that takes 
		// the key (get) or the key + the value (set)
		sol::meta_function::index, &vec::getter,
		sol::meta_function::new_index, &vec::setter
		);

	lua.script(R"(
		v1 = vec.new(1, 0)
		v2 = vec.new(0, 1)
		
		-- observe usage of getter/setter
		print("v1:", v1.x, v1.y)
		print("v2:", v2.x, v2.y)
		-- gets 0, 1 by doing lookup into getter function
		print("changing v2...")
		v2.x = 3
		v2[1] = 5
		-- can use [0] [1] like popular
		-- C++-style math vector classes
		print("v1:", v1.x, v1.y)
		print("v2:", v2.x, v2.y)
		-- both obj.name and obj["name"]
		-- are equivalent lookup methods
		-- and both will trigger the getter
		-- if it can't find 'name' on the object
		assert(v1["x"] == v1.x)
		assert(v1[0] == v1.x)
		assert(v1["x"] == v1[0])

		assert(v1["y"] == v1.y)
		assert(v1[1] == v1.y)
		assert(v1["y"] == v1[1])
)");


	// Can also be manipulated from C++,
	// and will call getter/setter methods:
	sol::userdata v1 = lua["v1"];
	double v1x = v1["x"];
	double v1y = v1["y"];
	c_assert(v1x == 1.000);
	c_assert(v1y == 0.000);
	v1[0] = 2.000;

	lua.script(R"(
		assert(v1.x == 2.000)
		assert(v1["x"] == 2.000)
		assert(v1[0] == 2.000)
	)");

	std::cout << std::endl;

	return 0;
}