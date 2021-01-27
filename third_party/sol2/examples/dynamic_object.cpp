#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>
#include "assert.hpp"

// use as-is,
// add as a member of your class,
// or derive from it and bind it appropriately
struct dynamic_object {
	std::unordered_map<std::string, sol::object> entries;

	void dynamic_set(std::string key, sol::stack_object value) {
		auto it = entries.find(key);
		if (it == entries.cend()) {
			entries.insert(it, { std::move(key), std::move(value) });
		}
		else {
			std::pair<const std::string, sol::object>& kvp = *it;
			sol::object& entry = kvp.second;
			entry = sol::object(std::move(value));
		}
	}

	sol::object dynamic_get(std::string key) {
		auto it = entries.find(key);
		if (it == entries.cend()) {
			return sol::lua_nil;
		}
		return it->second;
	}
};


int main() {
	std::cout << "=== dynamic_object ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<dynamic_object>("dynamic_object",
		sol::meta_function::index, &dynamic_object::dynamic_get,
		sol::meta_function::new_index, &dynamic_object::dynamic_set,
		sol::meta_function::length, [](dynamic_object& d) {
		return d.entries.size();
	}
	);

	lua.safe_script(R"(
d1 = dynamic_object.new()
d2 = dynamic_object.new()

print(#d1) -- length operator
print(#d2)

function d2:run(lim)
	local r = 0
	for i=0,lim do
		r = r + i
	end
	if (r % 2) == 1 then
		print("odd")
	end
	return r
end

-- only added an entry to d2
print(#d1) 
print(#d2)

-- only works on d2
local value = d2:run(5)
assert(value == 15)
)");

	// does not work on d1: 'run' wasn't added to d1, only d2
	auto script_result = lua.safe_script("local value = d1:run(5)", sol::script_pass_on_error);
	c_assert(!script_result.valid());
	sol::error err = script_result;
	std::cout << "received expected error: " << err.what() << std::endl;
	std::cout << std::endl;

	return 0;
}