#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

int func_1(int value) {
	return 20 + value;
}

std::string func_2(std::string text) {
	return "received: " + text;
}

sol::variadic_results fallback(sol::this_state ts, sol::variadic_args args) {
	sol::variadic_results r;
	if (args.size() == 2) {
		r.push_back({ ts, sol::in_place, args.get<int>(0) + args.get<int>(1) });
	}
	else {
		r.push_back({ ts, sol::in_place, 52 });
	}
	return r;
}

int main(int, char*[]) {
	std::cout << "=== calling lua functions ===" << std::endl;

	sol::state lua;
	lua.open_libraries();

	sol::table mLuaPackets = lua.create_named_table("mLuaPackets");
	mLuaPackets[1] = lua.create_table_with("timestamp", 0LL);
	mLuaPackets[2] = lua.create_table_with("timestamp", 3LL);
	mLuaPackets[3] = lua.create_table_with("timestamp", 1LL);

	lua.script("print('--- pre sort ---')");
	lua.script("for i=1,#mLuaPackets do print(i, mLuaPackets[i].timestamp) end");

	lua["table"]["sort"](mLuaPackets, sol::as_function([](sol::table l, sol::table r) {
		std::uint64_t tl = l["timestamp"];
		std::uint64_t tr = r["timestamp"];
		return tl < tr;
	}));

	lua.script("print('--- post sort ---')");
	lua.script("for i=1,#mLuaPackets do print(i, mLuaPackets[i].timestamp) end");

	return 0;
}