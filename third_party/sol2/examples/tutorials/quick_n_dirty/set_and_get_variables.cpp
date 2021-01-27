#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "../../assert.hpp"

int main(int, char*[]) {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// integer types
	lua.set("number", 24);
	// floating point numbers
	lua["number2"] = 24.5;
	// string types
	lua["important_string"] = "woof woof";
	// is callable, therefore gets stored as a function that can be called
	lua["a_function"] = []() { return 100; };
	// make a table
	lua["some_table"] = lua.create_table_with("value", 24);


	// equivalent to this code
	std::string equivalent_code = R"(
		t = {
			number = 24,
			number2 = 24.5,
			important_string = "woof woof",
			a_function = function () return 100 end,
			some_table = { value = 24 }
		}
	)";

	// check in Lua
	lua.script(equivalent_code);

	lua.script(R"(
		assert(t.number == number)
		assert(t.number2 == number2)
		assert(t.important_string == important_string)
		assert(t.a_function() == a_function())
		assert(t.some_table.value == some_table.value)
	)");


	// implicit conversion
	int number = lua["number"];
	c_assert(number == 24);
	// explicit get
	auto number2 = lua.get<double>("number2");
	c_assert(number2 == 24.5);
	// strings too
	std::string important_string = lua["important_string"];
	c_assert(important_string == "woof woof");
	// dig into a table
	int value = lua["some_table"]["value"];
	c_assert(value == 24);
	// get a function
	sol::function a_function = lua["a_function"];
	int value_is_100 = a_function();
	// convertible to std::function
	std::function<int()> a_std_function = a_function;
	int value_is_still_100 = a_std_function();
	c_assert(value_is_100 == 100);
	c_assert(value_is_still_100 == 100);

	sol::object number_obj = lua.get<sol::object>("number");
	// sol::type::number
	sol::type t1 = number_obj.get_type();
	c_assert(t1 == sol::type::number);

	sol::object function_obj = lua["a_function"];
	// sol::type::function
	sol::type t2 = function_obj.get_type();
	c_assert(t2 == sol::type::function);
	bool is_it_really = function_obj.is<std::function<int()>>();
	c_assert(is_it_really);

	// will not contain data
	sol::optional<int> check_for_me = lua["a_function"];
	c_assert(check_for_me == sol::nullopt);

	return 0;
}
