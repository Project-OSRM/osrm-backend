// sol2 

// The MIT License (MIT)

// Copyright (c) 2013-2018 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "test_sol.hpp"

#include <catch.hpp>

#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
#include <map>
#include <deque>
#include <array>
#include <unordered_map>
#include <set>
#include <unordered_set>

class int_shim {
public:
	int_shim() = default;

	int_shim(int x)
		: x_(x) {
	}

	int val() const {
		return x_;
	}

private:
	int x_ = -1;
};

class input_it {
public:
	typedef std::input_iterator_tag iterator_category;
	typedef int_shim value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef std::ptrdiff_t difference_type;

	input_it() = default;

	input_it(int n, int m)
		: n_(n), m_(m), value_(n_) {
		assert(n_ >= 0);
		assert(m_ >= 0);
		assert(n_ <= m_);

		if (!n_ && !m_) {
			n_ = -1;
			m_ = -1;
			value_ = -1;
		}
	}

	const int_shim& operator*() const {
		return value_;
	}

	const int_shim* operator->() const {
		return &value_;
	}

	input_it& operator++() {
		assert(n_ >= 0);
		assert(m_ >= 0);
		if (n_ == m_ - 1) {
			n_ = m_ = -1;
		}
		else {
			++n_;
		}
		value_ = n_;
		return *this;
	}

	bool operator==(const input_it& i) const {
		return n_ == i.n_ && m_ == i.m_;
	}

	bool operator!=(const input_it& i) const {
		return !(*this == i);
	}

private:
	int n_ = -1;
	int m_ = -1;
	int_shim value_;
};

class not_really_a_container {
public:
	using value_type = int_shim;
	using iterator = input_it;
	using const_iterator = input_it;

	const_iterator begin() const {
		return iterator(0, 100);
	}

	const_iterator end() const {
		return iterator();
	}

	value_type gcc_warning_block() {
		return int_shim();
	}

	std::size_t size() const {
		return 100;
	}
};

struct my_vec : public std::vector<int> {
	typedef std::vector<int> base_t;
	using base_t::base_t;
};

namespace sol {
	template <>
	struct is_container<my_vec> : std::true_type {};

	template <>
	struct container_traits<my_vec> {
		static auto begin(lua_State*, my_vec& self) {
			return self.begin();
		}
		static auto end(lua_State*, my_vec& self) {
			return self.end();
		}
		static std::ptrdiff_t index_adjustment(lua_State*, my_vec&) {
			return 0;
		}
	};

} // namespace sol

auto test_table_return_one() {
	return sol::as_table(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
}

auto test_table_return_two() {
	return sol::as_table(std::vector<std::pair<std::string, int>>{ { "one", 1 }, { "two", 2 }, { "three", 3 } });
}

auto test_table_return_three() {
	return sol::as_table(std::map<std::string, std::string>{ { "name", "Rapptz" }, { "friend", "ThePhD" }, { "project", "sol" } });
}

auto test_table_return_four() {
	return sol::as_table(std::array<std::pair<std::string, int>, 4>{ { { "one", 1 }, { "two", 2 }, { "three", 3 }, { "four", 4 } } });
}

template <typename S, typename T>
void check_ordered_values(S& src, T& target) {
	std::size_t idx = 0;
	auto b = std::begin(target);
	auto e = std::end(target);
	for (; b != e; ++b, ++idx) {
		const auto& v = src[idx];
		REQUIRE((*b == v));
	}
}

template <typename S, typename T>
void check_unordered_values(S& src, T& target) {
	std::size_t idx = 0;
	auto b = std::begin(target);
	auto e = std::end(target);
	for (; b != e; ++b, ++idx) {
		auto sb = std::begin(src);
		auto se = std::end(src);
		auto it = std::find(sb, se, *b);
		REQUIRE((it != se));
	}
}

TEST_CASE("containers/returns", "make sure that even references to vectors are being serialized as tables") {
	sol::state lua;
	std::vector<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::vector<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::object x = lua["x"];
	sol::type xt = x.get_type();
	REQUIRE(xt == sol::type::userdata);
	sol::table t = x;
	bool matching;
	matching = t[1] == 1;
	REQUIRE(matching);
	matching = t[2] == 2;
	REQUIRE(matching);
	matching = t[3] == 3;
	REQUIRE(matching);
}

TEST_CASE("containers/table conversions (lvalue)", "test table conversions with as_table and nested, when not directly serializing a temporary / new value") {
	sol::state lua;

	auto f = []() {
		std::vector<std::string> response_words;
		response_words.push_back("a");
		response_words.push_back("b");
		response_words.push_back("c");
		return sol::as_table(response_words);
	};
	auto g = []() {
		std::vector<std::string> response_words;
		response_words.push_back("a");
		response_words.push_back("b");
		response_words.push_back("c");
		return sol::as_nested(response_words);
	};

	lua["f"] = std::ref(f);
	lua["g"] = std::ref(g);

	sol::safe_function sff = lua["f"];
	sol::safe_function sfg = lua["g"];
	sol::table tf = sff();
	sol::table tg = sfg();

	std::string af = tf[1];
	std::string bf = tf[2];
	std::string cf = tf[3];
	std::string ag = tf[1];
	std::string bg = tf[2];
	std::string cg = tf[3];
	REQUIRE(tf.size() == 3);
	REQUIRE(af == "a");
	REQUIRE(bf == "b");
	REQUIRE(cf == "c");
	REQUIRE(tg.size() == 3);
	REQUIRE(ag == "a");
	REQUIRE(bg == "b");
	REQUIRE(cg == "c");
}

TEST_CASE("containers/table conversions (std::ref)", "test table conversions with as_table and nested, when not directly serializing a temporary / new value") {
	sol::state lua;

	std::vector<std::string> response_words;
	response_words.push_back("a");
	response_words.push_back("b");
	response_words.push_back("c");
	auto f = [&response_words]() {
		return sol::as_table(std::ref(response_words));
	};
	auto g = [&response_words]() {
		return sol::as_nested(std::ref(response_words));
	};

	lua["f"] = std::ref(f);
	lua["g"] = std::ref(g);

	sol::safe_function sff = lua["f"];
	sol::safe_function sfg = lua["g"];
	sol::table tf = sff();
	sol::table tg = sfg();

	std::string af = tf[1];
	std::string bf = tf[2];
	std::string cf = tf[3];
	std::string ag = tf[1];
	std::string bg = tf[2];
	std::string cg = tf[3];
	REQUIRE(tf.size() == 3);
	REQUIRE(af == "a");
	REQUIRE(bf == "b");
	REQUIRE(cf == "c");
	REQUIRE(tg.size() == 3);
	REQUIRE(ag == "a");
	REQUIRE(bg == "b");
	REQUIRE(cg == "c");
}

TEST_CASE("containers/table conversion", "test table conversions with as_table and nested") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("bark", []() {
		return sol::as_nested(std::vector<std::string>{ "bark", "woof" });
	});

	lua.set_function("woof", []() {
		return sol::as_nested(std::vector<std::string>{ "bark", "woof" });
	});

	auto result1 = lua.safe_script("v1 = bark()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("v2 = woof()", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	sol::as_table_t<std::vector<std::string>> as_table_strings = lua["v1"];
	sol::nested<std::vector<std::string>> nested_strings = lua["v2"];

	std::vector<std::string> expected_values{ "bark", "woof" };
	REQUIRE(as_table_strings.source == expected_values);
	REQUIRE(nested_strings.source == expected_values);
}

TEST_CASE("containers/from table argument conversions", "test table conversions without as_table and nested for function args") {
	const std::vector<std::string> expected_values{ "bark", "woof" };
	
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", [&](std::vector<std::string> t) {
		return t == expected_values;
	});

	auto result0 = lua.safe_script("t = { \"bark\", \"woof\" }");
	REQUIRE(result0.valid());

	auto result1 = lua.safe_script("assert(f(t))", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	sol::function f = lua["f"];
	sol::table t = lua["t"];
	bool passed = f(t);
	REQUIRE(passed);
}

TEST_CASE("containers/deeply nested", "make sure nested works for deeply-nested C++ containers and works as advertised") {
	typedef std::map<const char *, std::string> info_t;
	typedef std::vector<info_t> info_vector;

	class ModList {
	public:
		info_vector list;

		ModList() {
			list.push_back(info_t{
				{"a", "b"}
				});
		}

		sol::nested<info_vector> getList () {
			return sol::nested<info_vector>(list);
		};
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<ModList>("ModList",
		"getList", &ModList::getList	
		);

	sol::string_view code = R"(
mods = ModList.new()
local modlist = mods:getList()
print(modlist[1])
assert(type(modlist) == "table")
assert(type(modlist[1]) == "table")
)";

	auto result1 = lua.safe_script(code, sol::script_pass_on_error);
	REQUIRE(result1.valid());
}

TEST_CASE("containers/vector roundtrip", "make sure vectors can be round-tripped") {
	sol::state lua;
	std::vector<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::vector<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::vector<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/deque roundtrip", "make sure deques can be round-tripped") {
	sol::state lua;
	std::deque<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::deque<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::deque<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/array roundtrip", "make sure arrays can be round-tripped") {
	sol::state lua;
	std::array<int, 3> v{ { 1, 2, 3 } };
	lua.set_function("f", [&]() -> std::array<int, 3>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::array<int, 3> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/list roundtrip", "make sure lists can be round-tripped") {
	sol::state lua;
	std::list<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::list<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::list<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/forward_list roundtrip", "make sure forward_lists can be round-tripped") {
	sol::state lua;
	std::forward_list<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::forward_list<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::forward_list<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/map roundtrip", "make sure maps can be round-tripped") {
	sol::state lua;
	std::map<std::string, int> v{ { "a", 1 }, { "b", 2 }, { "c", 3 } };
	lua.set_function("f", [&]() -> std::map<std::string, int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::map<std::string, int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/unordered_map roundtrip", "make sure unordered_maps can be round-tripped") {
	sol::state lua;
	std::unordered_map<std::string, int> v{ { "a", 1 }, { "b", 2 }, { "c", 3 } };
	lua.set_function("f", [&]() -> std::unordered_map<std::string, int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::unordered_map<std::string, int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/unordered_set roundtrip", "make sure unordered_sets can be round-tripped") {
	sol::state lua;
	std::unordered_set<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::unordered_set<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::unordered_set<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/set roundtrip", "make sure sets can be round-tripped") {
	sol::state lua;
	std::set<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() -> std::set<int>& {
		return v;
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	std::set<int> x = lua["x"];
	bool areequal = x == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/vector table roundtrip", "make sure vectors can be round-tripped") {
	sol::state lua;
	std::vector<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::vector<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/deque table roundtrip", "make sure deques can be round-tripped") {
	sol::state lua;
	std::deque<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::deque<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/array table roundtrip", "make sure arrays can be round-tripped") {
	sol::state lua;
	std::array<int, 3> v{ { 1, 2, 3 } };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::array<int, 3>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/list table roundtrip", "make sure lists can be round-tripped") {
	sol::state lua;
	std::list<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::list<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/forward_list table roundtrip", "make sure forward_lists can be round-tripped") {
	sol::state lua;
	std::forward_list<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::forward_list<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/map table roundtrip", "make sure maps can be round-tripped") {
	sol::state lua;
	std::map<std::string, int> v{ { "a", 1 }, { "b", 2 }, { "c", 3 } };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::map<std::string, int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/unordered_map table roundtrip", "make sure unordered_maps can be round-tripped") {
	sol::state lua;
	std::unordered_map<std::string, int> v{ { "a", 1 }, { "b", 2 }, { "c", 3 } };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::unordered_map<std::string, int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/unordered_set table roundtrip", "make sure unordered_sets can be round-tripped") {
	sol::state lua;
	std::unordered_set<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::unordered_set<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/set table roundtrip", "make sure sets can be round-tripped") {
	sol::state lua;
	std::set<int> v{ 1, 2, 3 };
	lua.set_function("f", [&]() {
		return sol::as_table(v);
	});
	auto result1 = lua.safe_script("x = f()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::as_table_t<std::set<int>> x = lua["x"];
	bool areequal = x.source == v;
	REQUIRE(areequal);
}

TEST_CASE("containers/custom usertype", "make sure container usertype metatables can be overridden") {
	typedef std::unordered_map<int, int> bark;

	sol::state lua;
	lua.open_libraries();
	lua.new_usertype<bark>("bark",
		"something", [](const bark& b) {
			INFO("It works: " << b.at(24));
		},
		"size", &bark::size, "at", sol::resolve<const int&>(&bark::at), "clear", &bark::clear);
	bark obj{ { 24, 50 } };
	lua.set("a", &obj);
	{
		auto result0 = lua.safe_script("assert(a:at(24) == 50)", sol::script_pass_on_error);
		REQUIRE(result0.valid());
		auto result1 = lua.safe_script("a:something()", sol::script_pass_on_error);
		REQUIRE(result1.valid());
	}
	lua.set("a", obj);
	{
		auto result = lua.safe_script("assert(a:at(24) == 50)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("a:something()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("containers/const serialization kvp", "make sure const keys / values are respected") {
	typedef std::map<int, const int> bark;

	sol::state lua;
	lua.open_libraries();
	{
		bark obj{ { 24, 50 } };
		lua.set("a", std::ref(obj));
		auto result0 = lua.safe_script("assert(a[24] == 50)", sol::script_pass_on_error);
		REQUIRE(result0.valid());
		auto result1 = lua.safe_script("a[24] = 51", sol::script_pass_on_error);
		REQUIRE_FALSE(result1.valid());
		auto result2 = lua.safe_script("assert(a[24] == 50)", sol::script_pass_on_error);
		REQUIRE(result2.valid());
	}
}

TEST_CASE("containers/basic serialization", "make sure containers are turned into proper userdata and have basic hooks established") {
	typedef std::vector<int> woof;
	sol::state lua;
	lua.open_libraries();
	lua.set("b", woof{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 });
	{
		auto result = lua.safe_script("for k = 1, #b do assert(k == b[k]) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	woof w{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };
	lua.set("b", w);
	{
		auto result = lua.safe_script("for k = 1, #b do assert(k == b[k]) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	lua.set("b", &w);
	{
		auto result = lua.safe_script("for k = 1, #b do assert(k == b[k]) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	lua.set("b", std::ref(w));
	{
		auto result = lua.safe_script("for k = 1, #b do assert(k == b[k]) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

#if 0 // LUL const int holders
TEST_CASE("containers/const serialization", "make sure containers are turned into proper userdata and the basic hooks respect const-ness") {
	typedef std::vector<const int> woof;
	sol::state lua;
	lua.open_libraries();
	lua.set("b", woof{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 });
	{
		auto result = lua.safe_script("for k, v in pairs(b) do assert(k == v) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("b[1] = 20", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}
#endif

TEST_CASE("containers/table serialization", "ensure types can be serialized as tables still") {
	typedef std::vector<int> woof;
	sol::state lua;
	lua.open_libraries();
	lua.set("b", sol::as_table(woof{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 }));
	{
		auto result = lua.safe_script("for k, v in ipairs(b) do assert(k == v) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	woof w{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };
	lua.set("b", sol::as_table(w));
	{
		auto result = lua.safe_script("for k, v in ipairs(b) do assert(k == v) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	lua.set("b", sol::as_table(&w));
	{
		auto result = lua.safe_script("for k, v in ipairs(b) do assert(k == v) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	lua.set("b", sol::as_table(std::ref(w)));
	{
		auto result = lua.safe_script("for k, v in ipairs(b) do assert(k == v) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("containers/const correctness", "usertype metatable names should reasonably ignore const attributes") {
	struct Vec {
		int x, y, z;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<Vec>("Vec", "x", &Vec::x, "y", &Vec::y, "z", &Vec::z);

	Vec vec;
	vec.x = 1;
	vec.y = 2;
	vec.z = -3;

	std::vector<Vec> foo;
	foo.push_back(vec);

	std::vector<Vec const*> bar;
	bar.push_back(&vec);

	auto result0 = lua.safe_script(R"(
func = function(vecs)
    for i = 1, #vecs do
		vec = vecs[i]
        print(i, ":", vec.x, vec.y, vec.z)
    end
end
)", sol::script_pass_on_error);
	REQUIRE(result0.valid());

	sol::protected_function f(lua["func"]);
	auto pfr1 = f(foo);
	REQUIRE(pfr1.valid());
	auto pfr2 = f(bar);
	REQUIRE(pfr2.valid());
}

TEST_CASE("containers/arbitrary creation", "userdata and tables should be usable from standard containers") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.set_function("test_one", test_table_return_one);
	lua.set_function("test_two", test_table_return_two);
	lua.set_function("test_three", test_table_return_three);
	lua.set_function("test_four", test_table_return_four);

	{
		auto result = lua.safe_script("a = test_one()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("b = test_two()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("c = test_three()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("d = test_four()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	{
		auto result = lua.safe_script("assert(#a == 10, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(a[3] == 3, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(b.one == 1, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(b.three == 3, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(c.name == 'Rapptz', 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(c.project == 'sol', 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(d.one == 1, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(d.three == 3, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(d.four == 4, 'error')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	sol::table a = lua.get<sol::table>("a");
	sol::table b = lua.get<sol::table>("b");
	sol::table c = lua.get<sol::table>("c");
	sol::table d = lua["d"];

	REQUIRE(a.size() == 10ULL);
	REQUIRE(a.get<int>(3) == 3);
	REQUIRE(b.get<int>("one") == 1);
	REQUIRE(b.get<int>("three") == 3);
	REQUIRE(c.get<std::string>("name") == "Rapptz");
	REQUIRE(c.get<std::string>("project") == "sol");
	REQUIRE(d.get<int>("one") == 1);
	REQUIRE(d.get<int>("three") == 3);
	REQUIRE(d.get<int>("four") == 4);
}

TEST_CASE("containers/usertype transparency", "Make sure containers pass their arguments through transparently and push the results as references, not new values") {
	class A {
	public:
		int a;
		A(int b = 2)
		: a(b){};

		void func() {
		}
	};

	struct B {

		B() {
			for (std::size_t i = 0; i < 20; ++i) {
				a_list.emplace_back(static_cast<int>(i));
			}
		}

		std::vector<A> a_list;
	};

	sol::state lua;
	lua.new_usertype<B>("B",
		"a_list", &B::a_list);

	auto result = lua.safe_script(R"(
b = B.new()
a_ref = b.a_list[2]
)", sol::script_pass_on_error);
	REQUIRE(result.valid());

	B& b = lua["b"];
	A& a_ref = lua["a_ref"];
	REQUIRE(&b.a_list[1] == &a_ref);
	REQUIRE(b.a_list[1].a == a_ref.a);
}

struct options {
	static int livingcount;
	static options* last;
	options() {
		++livingcount;
		last = this;
		INFO("constructor: " << this);
	}

	std::string output_help() {
		last = this;
		INFO("func: " << this);
		return "";
	}

	void begin() {
	}
	void end() {
	}

	~options() {
		last = this;
		--livingcount;
	}
};

options* options::last = nullptr;
int options::livingcount = 0;

struct machine {
	options opt;
};

namespace sol {
	template <>
	struct is_container<options> : std::false_type {};
} // namespace sol

TEST_CASE("containers/is container", "make sure the is_container trait behaves properly") {
	sol::state lua;
	lua.open_libraries();

	lua.new_usertype<options>("options_type",
		"output_help", &options::output_help);

	lua.new_usertype<machine>("machine_type",
		"new", sol::no_constructor,
		"opt", [](machine& m) { return &m.opt; },
		"copy_opt", [](machine& m) { return m.opt; });

	{
		machine m;
		lua["machine"] = &m;

		auto result0 = lua.safe_script(R"(
			machine:opt():output_help()
		)", sol::script_pass_on_error);
		REQUIRE(result0.valid());

		REQUIRE(options::last == &m.opt);
		REQUIRE(options::livingcount == 1);
	}
	REQUIRE(options::livingcount == 0);
}

TEST_CASE("containers/readonly", "make sure readonly members are stored appropriately") {
	sol::state lua;
	lua.open_libraries();

	struct bar {
		int x = 24;
	};

	struct foo {
		std::list<bar> seq;
	};

	lua.new_usertype<foo>(
		"foo",
		"seq", &foo::seq,					 // this one works
		"readonly_seq", sol::readonly(&foo::seq) // this one does not work
	);
	lua["value"] = std::list<bar>{ {}, {}, {} };

	auto result0 = lua.safe_script(R"(
a = foo.new()
x = a.seq
a.seq = value
y = a.readonly_seq
)", sol::script_pass_on_error);
	REQUIRE(result0.valid());
	std::list<bar>& seqrefx = lua["x"];
	std::list<bar>& seqrefy = lua["y"];
	REQUIRE(&seqrefx == &seqrefy);
	REQUIRE(seqrefx.size() == 3);
	auto result = lua.safe_script("a.readonly_seq = value", sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
}

TEST_CASE("containers/to_args", "Test that the to_args abstractions works") {
	sol::state lua;
	lua.open_libraries();

	auto result1 = lua.safe_script("function f (a, b, c, d) print(a, b, c, d) return a, b, c, d end", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	sol::function f = lua["f"];
	int a, b, c, d;

	std::vector<int> v2{ 3, 4 };
	sol::tie(a, b, c, d) = f(1, 2, sol::as_args(v2));
	REQUIRE(a == 1);
	REQUIRE(b == 2);
	REQUIRE(c == 3);
	REQUIRE(d == 4);

	std::set<int> v4{ 7, 6, 8, 5 };
	sol::tie(a, b, c, d) = f(sol::as_args(v4));
	REQUIRE(a == 5);
	REQUIRE(b == 6);
	REQUIRE(c == 7);
	REQUIRE(d == 8);

	int v3[] = { 10, 11, 12 };
	sol::tie(a, b, c, d) = f(9, sol::as_args(v3));
	REQUIRE(a == 9);
	REQUIRE(b == 10);
	REQUIRE(c == 11);
	REQUIRE(d == 12);
}

TEST_CASE("containers/ipairs test", "ensure that abstractions roundtrip properly") {
	struct thing {
		int x = 20;
	};
	thing t{};
	sol::state lua;
	lua.open_libraries();

	lua.set_function("f", [&t]() {
		return std::vector<thing*>(5, &t);
	});

	auto result1 = lua.safe_script(R"(
c = f()
)", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	auto result2 = lua.safe_script(R"(
check = {}
local i = 1
while c[i] do
	check[i] = c[i]
	i = i + 1
end
)", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	sol::table c = lua["check"];
	for (std::size_t i = 1; i < 6; ++i) {
		thing& ct = c[i];
		REQUIRE(&t == &ct);
		REQUIRE(ct.x == 20);
	}
}

TEST_CASE("containers/append idiom", "ensure the append-idiom works as intended") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	
	auto result1 = lua.safe_script(
		R"(
function f_fill(vec)
	print("#vec in lua: " .. #vec)
	for k = 1, #vec do
		vec[k] = k
	end
	print("#vec in lua: " .. #vec)
end
function f_append(vec)
	print("#vec in lua: " .. #vec)
	vec[#vec] = -10456407
	vec[#vec + 1] = -54
	print("#vec in lua: " .. #vec)
end
)");
	REQUIRE(result1.valid());

	std::vector<int> fill_cmp{ 1, 2, 3 };
	std::vector<int> append_cmp{ -1, -1, -10456407, -54 };

	std::vector<int> vec1{ -1, -1, -1 };
	std::vector<int> vec2{ -1, -1, -1 };

	REQUIRE(vec1.size() == 3);
	lua["f_fill"](vec1);
	REQUIRE(vec1.size() == 3);
	REQUIRE(vec1 == fill_cmp);

	REQUIRE(vec2.size() == 3);
	lua["f_append"](vec2);
	REQUIRE(vec2.size() == 4);
	REQUIRE(vec2 == append_cmp);
}

TEST_CASE("containers/non_copyable", "make sure non-copyable types in containers behave properly when stored as a member variable in a bound usertype") {
	struct non_copyable {
		non_copyable(non_copyable&& other) noexcept = default;
		non_copyable& operator=(non_copyable&& other) noexcept = default;
		non_copyable(const non_copyable& other) noexcept = delete;
		non_copyable& operator=(const non_copyable& other) noexcept = delete;
	};
	struct test {
		std::vector<non_copyable> b;

		test()
		: b() {
		}
		test(test&&) = default;
		test& operator=(test&&) = default;
		test(const test&) = delete;
		test& operator=(const test&) = delete;
	};

	SECTION("normal") {
		sol::state lua;
		lua.new_usertype<test>("test",
			"b", sol::readonly(&test::b));

		lua["v"] = std::vector<non_copyable>{};

		auto pfr = lua.safe_script("t = test.new() t.b = v", sol::script_pass_on_error);
		REQUIRE_FALSE(pfr.valid());
	}
	SECTION("simple") {
		sol::state lua;
		lua.new_simple_usertype<test>("test",
			"b", sol::readonly(&test::b));

		lua["v"] = std::vector<non_copyable>{};

		auto pfr = lua.safe_script("t = test.new() t.b = v", sol::script_pass_on_error);
		REQUIRE_FALSE(pfr.valid());
	}
}

TEST_CASE("containers/input iterators", "test shitty input iterators that are all kinds of B L E H") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package);
	lua.new_usertype<int_shim>("int_shim",
		"new", sol::no_constructor,
		"val", &int_shim::val);

	not_really_a_container c;
	lua["c"] = &c;
#if SOL_LUA_VERSION > 502
	auto result0 = lua.safe_script(R"lua(
for k, v in pairs(c) do
  assert((k - 1) == v:val())
end
)lua", sol::script_pass_on_error);
	REQUIRE(result0.valid());

#endif
	auto result1 = lua.safe_script(R"lua(
for k=1,#c do
  v = c[k]
  assert((k - 1) == v:val())
end
)lua", sol::script_pass_on_error);
	REQUIRE(result1.valid());
}

TEST_CASE("containers/pairs", "test how well pairs work with the underlying system") {
	typedef std::pair<std::string, int> pair_arr_t[5];
	typedef int arr_t[5];

	sol::state lua;

	lua.open_libraries(sol::lib::base);

	std::vector<std::pair<std::string, int>> a{ { "one", 1 }, { "two", 2 }, { "three", 3 }, { "four", 4 }, { "five", 5 } };
	std::array<std::pair<std::string, int>, 5> b{ { { "one", 1 }, { "two", 2 }, { "three", 3 }, { "four", 4 }, { "five", 5 } } };
	pair_arr_t c{ { "one", 1 }, { "two", 2 }, { "three", 3 }, { "four", 4 }, { "five", 5 } };
	arr_t d = { 1, 2, 3, 4, 5 };

	lua["a"] = std::ref(a);
	lua["b"] = &b;
	lua["c"] = std::ref(c);
	lua["d"] = &d;

	auto result1 = lua.safe_script("av1, av2 = a:get(1)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("bv1, bv2 = b:get(1)", sol::script_pass_on_error);
	REQUIRE(result2.valid());
	auto result3 = lua.safe_script("cv1, cv2 = c:get(1)", sol::script_pass_on_error);
	REQUIRE(result3.valid());
	auto result4 = lua.safe_script("dv1, dv2 = d:get(1)", sol::script_pass_on_error);
	REQUIRE(result4.valid());

	std::vector<std::pair<std::string, int>>& la = lua["a"];
	std::array<std::pair<std::string, int>, 5>& lb = lua["b"];
	pair_arr_t* plc = lua["c"];
	pair_arr_t& lc = *plc;
	arr_t* pld = lua["d"];
	arr_t& ld = *pld;

	std::pair<std::string, int>& va = la[0];
	std::pair<std::string, int>& vb = lb[0];
	std::pair<std::string, int>& vc = lc[0];
	int& vd = ld[0];

	std::string av1 = lua["av1"];
	int av2 = lua["av2"];
	std::string bv1 = lua["bv1"];
	int bv2 = lua["bv2"];
	std::string cv1 = lua["cv1"];
	int cv2 = lua["cv2"];
	int dv1 = lua["dv1"];
	sol::lua_nil_t dv2 = lua["dv2"];

	REQUIRE(va.first == "one");
	REQUIRE(va.second == 1);
	REQUIRE(vb.first == "one");
	REQUIRE(vb.second == 1);
	REQUIRE(vc.first == "one");
	REQUIRE(vc.second == 1);
	REQUIRE(vd == 1);

	REQUIRE(av1 == "one");
	REQUIRE(av2 == 1);
	REQUIRE(bv1 == "one");
	REQUIRE(bv2 == 1);
	REQUIRE(cv1 == "one");
	REQUIRE(cv2 == 1);
	REQUIRE(dv1 == 1);
	REQUIRE(dv2 == sol::lua_nil);
}

TEST_CASE("containers/pointer types", "check that containers with unique usertypes and pointers or something") {
	struct base_t {
		virtual int get() const = 0;
	};

	struct derived_1_t : base_t {
		virtual int get() const override {
			return 250;
		}
	};

	struct derived_2_t : base_t {
		virtual int get() const override {
			return 500;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	derived_1_t d1;
	derived_2_t d2;

	std::vector<std::unique_ptr<base_t>> v1;
	v1.push_back(std::make_unique<derived_1_t>());
	v1.push_back(std::make_unique<derived_2_t>());

	std::vector<base_t*> v2;
	v2.push_back(&d1);
	v2.push_back(&d2);
	lua["c1"] = std::move(v1);
	lua["c2"] = &v2;
	
	auto result1 = lua.safe_script("b1 = c1[1]", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	base_t* b1 = lua["b1"];
	int val1 = b1->get();
	REQUIRE(val1 == 250);

	auto result2 = lua.safe_script("b2 = c2[2]", sol::script_pass_on_error);
	REQUIRE(result2.valid());
	base_t* b2 = lua["b2"];
	int val2 = b2->get();
	REQUIRE(val2 == 500);
}

TEST_CASE("containers/initializer-list", "test initializer lists get pushed as tables directly rather than userdata") {
	SECTION("array-like") {
		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::table);

		lua["c"] = { 1, 2, 3, 4, 5 };
		auto result1 = lua.safe_script(R"lua(
for k, v in pairs(c) do
  assert(k == v)
end
)lua", sol::script_pass_on_error);
		sol::as_table_t<std::vector<int>> t1vector = lua["c"];
		sol::as_table_t<std::deque<int>> t1deque = lua["c"];
		sol::as_table_t<std::list<int>> t1list = lua["c"];
		sol::as_table_t<std::forward_list<int>> t1flist = lua["c"];
		sol::as_table_t<std::set<int>> t1set = lua["c"];
		const int src[5] = { 1, 2, 3, 4, 5 };
		check_ordered_values(src, t1vector.source);
		check_ordered_values(src, t1deque.source);
		check_ordered_values(src, t1list.source);
		check_ordered_values(src, t1flist.source);
		check_ordered_values(src, t1set.source);
	}
	SECTION("map-like") {
		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::table);
		std::pair<const std::string, int> src[5]{
			{ "a", 21 },
			{ "b", 22 },
			{ "c", 23 },
			{ "d", 24 },
			{ "e", 25 }
		};

		lua["c"] = std::initializer_list<std::pair<std::string, int>>{
			{ "a", 21 },
			{ "b", 22 },
			{ "c", 23 },
			{ "d", 24 },
			{ "e", 25 }
		};

		sol::as_table_t<std::unordered_map<std::string, int>> t1umap = lua["c"];
		sol::as_table_t<std::unordered_multimap<std::string, int>> t1ummap = lua["c"];
		check_unordered_values(src, t1umap.source);
		check_unordered_values(src, t1ummap.source);
	}
}

TEST_CASE("containers/custom indexing", "allow containers to set a custom indexing offset") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua["c"] = my_vec{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	auto result1 = lua.safe_script("for i=0,9 do assert(i == c[i]) end", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("assert(c[10] == nil)", sol::script_pass_on_error);
	REQUIRE(result2.valid());
	auto result3 = lua.safe_script("assert(c[-1] == nil)", sol::script_pass_on_error);
	REQUIRE(result3.valid());
}

TEST_CASE("containers/containers of pointers", "containers of pointers shouldn't have their value_type's overly stripped") {
	sol::state lua;

	class MyContainer {
	public:
		typedef int** iterator;
		typedef int* value_type;

		std::vector<value_type> m_vec;

		inline iterator begin() { return m_vec.data(); }
		inline iterator end() { return m_vec.data() + m_vec.size(); }
		inline void push_back(value_type v) { m_vec.push_back(v); }
	};
	int a = 500;
	int b = 600;

	MyContainer ctr;
	ctr.push_back(&a);
	ctr.push_back(&b);
	lua["c"] = ctr;
	{
		auto result1 = lua.safe_script("ap = c[1]", sol::script_pass_on_error);
		REQUIRE(result1.valid());
		auto result2 = lua.safe_script("bp = c[2]", sol::script_pass_on_error);
		REQUIRE(result2.valid());
		int* ap = lua["ap"];
		int* bp = lua["bp"];
		REQUIRE(ap == &a);
		REQUIRE(bp == &b);
		REQUIRE(*ap == 500);
		REQUIRE(*bp == 600);
	}

	std::unordered_map<int, int*> ptrs;
	ptrs[5] = &a;
	ptrs[6] = &b;
	lua["c2"] = ptrs;
	{
		auto result1 = lua.safe_script("ap = c2[5]", sol::script_pass_on_error);
		REQUIRE(result1.valid());
		auto result2 = lua.safe_script("bp = c2[6]", sol::script_pass_on_error);
		REQUIRE(result2.valid());
		int* ap = lua["ap"];
		int* bp = lua["bp"];
		REQUIRE(ap == &a);
		REQUIRE(bp == &b);
		REQUIRE(*ap == 500);
		REQUIRE(*bp == 600);
	}
}
