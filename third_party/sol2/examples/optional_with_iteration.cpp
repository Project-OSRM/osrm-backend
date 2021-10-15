#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <string>
#include <memory>
#include <iostream>

int main(int, char**) {
	std::cout << "=== optional with iteration ===" << std::endl;

	struct thing {
		int a = 20;

		thing() = default;
		thing(int a) : a(a) {}
	};

	struct super_thing : thing {
		int b = 40;
	};

	struct unrelated {

	};

	sol::state lua;

	// Comment out the new_usertype call
	// to prevent derived class "super_thing"
	// from being picked up and cast to its base
	// class
	lua.new_usertype<super_thing>("super_thing",
		sol::base_classes, sol::bases<thing>()
	);

	// Make a few things
	lua["t1"] = thing{};
	lua["t2"] = super_thing{};
	lua["t3"] = unrelated{};
	// And a table
	lua["container"] = lua.create_table_with(
		0, thing{50}, 
		1, unrelated{}, 
		4, super_thing{}
	);


	std::vector<std::reference_wrapper<thing>> things;
	// Our recursive function
	// We use some lambda techniques and pass the function itself itself so we can recurse,
	// but a regular function would work too!
	auto fx = [&things](auto& f, auto& tbl) -> void {
		// You can iterate through a table: it has 
		// begin() and end()
		// like standard containers
		for (auto key_value_pair : tbl) {
			// Note that iterators are extremely frail
			// and should not be used outside of
			// well-constructed for loops
			// that use pre-increment ++,
			// or C++ ranged-for loops
			const sol::object& key = key_value_pair.first;
			const sol::object& value = key_value_pair.second;
			sol::type t = value.get_type();
			switch (t) {
			case sol::type::table: {
				sol::table inner = value.as<sol::table>();
				f(f, inner);
			}
			break;
			case sol::type::userdata: {
				// This allows us to check if a userdata is 
				// a specific class type
				sol::optional<thing&> maybe_thing = value.as<sol::optional<thing&>>();
				if (maybe_thing) {
					thing& the_thing = maybe_thing.value();
					if (key.is<std::string>()) {
						std::cout << "key " << key.as<std::string>() << " is a thing -- ";
					}
					else if (key.is<int>()) {
						std::cout << "key " << key.as<int>() << " is a thing -- ";
					}
					std::cout << "thing.a ==" << the_thing.a << std::endl;
					things.push_back(the_thing);
				}
			}
			break;
			default:
				break;
			}
		}
	};
	fx(fx, lua);

	std::cout << std::endl;

	return 0;
}
