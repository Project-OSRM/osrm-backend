#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <vector>
#include <numeric>

#include "assert.hpp"
#include <iostream>

class number_storage {
private:
	std::vector<int> data;

public:
	number_storage(int i) { data.push_back(i); }

	int accumulate() const { 
		return std::accumulate(data.begin(), data.end(), 0);
	}

public:
	using value_type = decltype(data)::value_type;
	using iterator = decltype(data)::iterator;
	using size_type = decltype(data)::size_type;
	iterator begin() { return iterator(data.begin()); }
	iterator end() { return iterator(data.end()); }
	size_type size() const noexcept { return data.size(); }
	size_type max_size() const noexcept { return data.max_size(); }
	void push_back(int value) { data.push_back(value); }
	bool empty() const noexcept { return data.empty(); }
};

namespace sol {
	template <>
	struct is_container<number_storage> : std::false_type {};
}

int main(int, char*[]) {
	std::cout << "=== container as container ===" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<number_storage>("number_storage",
		sol::constructors<number_storage(int)>(),
		"accumulate", &number_storage::accumulate,
		"iterable", [](number_storage& ns) { 
			return sol::as_container(ns); // treat like a container, despite is_container specialization
		}
	);

	lua.script(R"(
ns = number_storage.new(23)
print("accumulate before:", ns:accumulate())

-- reference original usertype like a container
ns_container = ns:iterable()
ns_container:add(24)
ns_container:add(25)

-- now print to show effect
print("accumulate after :", ns:accumulate())
	)");

	number_storage& ns = lua["ns"];
	number_storage& ns_container = lua["ns_container"];
	c_assert(&ns == &ns_container);
	c_assert(ns.size() == 3);

	std::cout << std::endl;

	return 0;
}
