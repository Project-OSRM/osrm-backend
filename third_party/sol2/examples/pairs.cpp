#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <map>
#include <iostream>

struct my_thing {
	std::map<std::string, int> m{
		{ "bark", 20 },
		{ "woof", 60 },
		{ "borf", 30 },
		{ "awoo", 5 },
	};

	my_thing() {

	}
};

struct lua_iterator_state {
	typedef std::map<std::string, int>::iterator it_t;
	it_t it;
	it_t last;

	lua_iterator_state(my_thing& mt) : it(mt.m.begin()), last(mt.m.end()) {}
};

std::tuple<sol::object, sol::object> my_next(sol::user<lua_iterator_state&> user_it_state, sol::this_state l) {
	// this gets called 
	// to start the first iteration, and every
	// iteration there after

	// the state you passed in my_pairs is argument 1
	// the key value is argument 2, but we do not 
	// care about the key value here
	lua_iterator_state& it_state = user_it_state;
	auto& it = it_state.it;
	if (it == it_state.last) {
		// return nil to signify that 
		// there's nothing more to work with.
		return std::make_tuple(
			sol::object(sol::lua_nil),
			sol::object(sol::lua_nil)
		);
	}
	auto itderef = *it;
	// 2 values are returned (pushed onto the stack):
	// the key and the value
	// the state is left alone
	auto r = std::make_tuple(sol::object(l, sol::in_place, it->first), sol::object(l, sol::in_place, it->second));
	// the iterator must be moved forward one before we return
	std::advance(it, 1);
	return r;
}

auto my_pairs(my_thing& mt) {
	// pairs expects 3 returns:
	// the "next" function on how to advance,
	// the "table" itself or some state,
	// and an initial key value (can be nil)

	// prepare our state
	lua_iterator_state it_state(mt);
	// sol::user is a space/time optimization over regular usertypes,
	// it's incompatible with regular usertypes and stores the type T directly in lua without any pretty setup
	// saves space allocation and a single dereference
	return std::make_tuple(&my_next, sol::user<lua_iterator_state>(std::move(it_state)), sol::lua_nil);
}

int main(int, char*[]) {
	std::cout << "===== pairs (advanced) =====" << std::endl;

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<my_thing>("my_thing",
		sol::meta_function::pairs, my_pairs
	);

#if SOL_LUA_VERSION > 501
	lua.safe_script(R"(
local mt = my_thing.new()
for k, v in pairs(mt) do 
	print(k, v) 
end
)");
#endif // Does not work on Lua 5.1
	std::cout << std::endl;

	return 0;
}
