#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include "assert.hpp"

#include <iostream>

struct my_type {
	int value = 10;

	my_type() {
		std::cout << "my_type at " << static_cast<void*>(this) << " being default constructed!" << std::endl;
	}

	my_type(const my_type& other) : value(other.value) {
		std::cout << "my_type at " << static_cast<void*>(this) << " being copy constructed!" << std::endl;
	}

	my_type(my_type&& other) : value(other.value) {
		std::cout << "my_type at " << static_cast<void*>(this) << " being move-constructed!" << std::endl;
	}

	my_type& operator=(const my_type& other) {
		value = other.value;
		std::cout << "my_type at " << static_cast<void*>(this) << " being copy-assigned to!" << std::endl;
		return *this;
	}

	my_type& operator=(my_type&& other) {
		value = other.value;
		std::cout << "my_type at " << static_cast<void*>(this) << " being move-assigned to!" << std::endl;
		return *this;
	}

	~my_type() {
		std::cout << "my_type at " << static_cast<void*>(this) << " being destructed!" << std::endl;
	}
};

int main() {

	std::cout << "=== unique_ptr support ===" << std::endl;

	sol::state lua;
	lua.new_usertype<my_type>("my_type",
		"value", &my_type::value
		);
	{
		std::unique_ptr<my_type> unique = std::make_unique<my_type>();
		lua["unique"] = std::move(unique);
	}
	{
		std::cout << "getting reference to unique_ptr..." << std::endl;
		std::unique_ptr<my_type>& ref_to_unique_ptr = lua["unique"];
		my_type& ref_to_my_type = lua["unique"];
		my_type* ptr_to_my_type = lua["unique"];

		c_assert(ptr_to_my_type == ref_to_unique_ptr.get());
		c_assert(&ref_to_my_type == ref_to_unique_ptr.get());
		c_assert(ref_to_unique_ptr->value == 10);

		// script affects all of them equally
		lua.script("unique.value = 20");

		c_assert(ptr_to_my_type->value == 20);
		c_assert(ref_to_my_type.value == 20);
		c_assert(ref_to_unique_ptr->value == 20);
	}
	{
		std::cout << "getting copy of unique_ptr..." << std::endl;
		my_type copy_of_value = lua["unique"];
		
		c_assert(copy_of_value.value == 20);

		// script still affects pointer, but does not affect copy of `my_type`
		lua.script("unique.value = 30");

		c_assert(copy_of_value.value == 20);
	}
	// set to nil and collect garbage to destroy it
	lua.script("unique = nil");
	lua.collect_garbage();
	lua.collect_garbage();

	std::cout << "garbage has been collected" << std::endl;
	std::cout << std::endl;

	return 0;
}
