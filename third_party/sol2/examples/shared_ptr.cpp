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

	std::cout << "=== shared_ptr support ===" << std::endl;

	sol::state lua;
	lua.new_usertype<my_type>("my_type",
		"value", &my_type::value
		);
	{
		std::shared_ptr<my_type> shared = std::make_shared<my_type>();
		lua["shared"] = std::move(shared);
	}
	{
		std::cout << "getting reference to shared_ptr..." << std::endl;
		std::shared_ptr<my_type>& ref_to_shared_ptr = lua["shared"];
		std::cout << "\tshared.use_count(): " << ref_to_shared_ptr.use_count() << std::endl;
		my_type& ref_to_my_type = lua["shared"];
		std::cout << "\tafter getting reference to my_type: " << ref_to_shared_ptr.use_count() << std::endl;
		my_type* ptr_to_my_type = lua["shared"];
		std::cout << "\tafter getting pointer to my_type: " << ref_to_shared_ptr.use_count() << std::endl;

		c_assert(ptr_to_my_type == ref_to_shared_ptr.get());
		c_assert(&ref_to_my_type == ref_to_shared_ptr.get());
		c_assert(ref_to_shared_ptr->value == 10);

		// script affects all of them equally
		lua.script("shared.value = 20");

		c_assert(ptr_to_my_type->value == 20);
		c_assert(ref_to_my_type.value == 20);
		c_assert(ref_to_shared_ptr->value == 20);
	}
	{
		std::cout << "getting copy of shared_ptr..." << std::endl;
		std::shared_ptr<my_type> copy_of_shared_ptr = lua["shared"];
		std::cout << "\tshared.use_count(): " << copy_of_shared_ptr.use_count() << std::endl;
		my_type copy_of_value = lua["shared"];
		std::cout << "\tafter getting value copy of my_type: " << copy_of_shared_ptr.use_count() << std::endl;

		c_assert(copy_of_shared_ptr->value == 20);
		c_assert(copy_of_value.value == 20);

		// script still affects pointer, but does not affect copy of `my_type`
		lua.script("shared.value = 30");

		c_assert(copy_of_shared_ptr->value == 30);
		c_assert(copy_of_value.value == 20);
	}
	// set to nil and collect garbage to destroy it
	lua.script("shared = nil");
	lua.collect_garbage();
	lua.collect_garbage();

	std::cout << "garbage has been collected" << std::endl;
	std::cout << std::endl;

	return 0;
}
