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

#include <algorithm>
#include <numeric>
#include <iostream>

TEST_CASE("operators/default", "test that generic equality operators and all sorts of equality tests can be used") {
	struct T {};
	struct U {
		int a;
		U(int x = 20)
		: a(x) {
		}
		bool operator==(const U& r) {
			return a == r.a;
		}
	};
	struct V {
		int a;
		V(int x = 20)
		: a(x) {
		}
		bool operator==(const V& r) const {
			return a == r.a;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	T t1;
	T& t2 = t1;
	T t3;
	U u1;
	U u2{ 30 };
	U u3;
	U v1;
	U v2{ 30 };
	U v3;
	lua["t1"] = &t1;
	lua["t2"] = &t2;
	lua["t3"] = &t3;
	lua["u1"] = &u1;
	lua["u2"] = &u2;
	lua["u3"] = &u3;
	lua["v1"] = &v1;
	lua["v2"] = &v2;
	lua["v3"] = &v3;

	SECTION("plain") {
		// Can only compare identity here
		{
			auto result1 = lua.safe_script("assert(t1 == t1)"
				"assert(t2 == t2)"
				"assert(t3 == t3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(t1 == t2)"
				"assert(not (t1 == t3))"
				"assert(not (t2 == t3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(u1 == u1)"
				"assert(u2 == u2)"
				"assert(u3 == u3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (u1 == u2))"
				"assert(u1 == u3)"
				"assert(not (u2 == u3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(v1 == v1)"
				"assert(v2 == v2)"
				"assert(v3 == v3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (v1 == v2))"
				"assert(v1 == v3)"
				"assert(not (v2 == v3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
	}
	SECTION("regular") {
		lua.new_usertype<T>("T");
		lua.new_usertype<U>("U");
		lua.new_usertype<V>("V");

		// Can only compare identity here
		{
			auto result1 = lua.safe_script("assert(t1 == t1)"
				"assert(t2 == t2)"
				"assert(t3 == t3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(t1 == t2)"
				"assert(not (t1 == t3))"
				"assert(not (t2 == t3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(u1 == u1)"
				"assert(u2 == u2)"
				"assert(u3 == u3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (u1 == u2))"
				"assert(u1 == u3)"
				"assert(not (u2 == u3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(v1 == v1)"
				"assert(v2 == v2)"
				"assert(v3 == v3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (v1 == v2))"
				"assert(v1 == v3)"
				"assert(not (v2 == v3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<T>("T");
		lua.new_simple_usertype<U>("U");
		lua.new_simple_usertype<V>("V");

		// Can only compare identity here
		{
			auto result1 = lua.safe_script("assert(t1 == t1)"
				"assert(t2 == t2)"
				"assert(t3 == t3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(t1 == t2)"
				"assert(not (t1 == t3))"
				"assert(not (t2 == t3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(u1 == u1)"
				"assert(u2 == u2)"
				"assert(u3 == u3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (u1 == u2))"
				"assert(u1 == u3)"
				"assert(not (u2 == u3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		{
			auto result1 = lua.safe_script("assert(v1 == v1)"
				"assert(v2 == v2)"
				"assert(v3 == v3)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
		{
			auto result1 = lua.safe_script("assert(not (v1 == v2))"
				"assert(v1 == v3)"
				"assert(not (v2 == v3))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
		}
	}
}

TEST_CASE("operators/call", "test call operator generation") {
	struct callable {
		int operator()(int a, std::string b) {
			return a + static_cast<int>(b.length());
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua.set("obj", callable());
			auto result1 = lua.safe_script("v = obj(2, 'bark woof')", sol::script_pass_on_error);
			REQUIRE(result1.valid());
			int v = lua["v"];
			REQUIRE(v == 11);
		}
	}
	SECTION("regular") {
		lua.new_usertype<callable>("callable");
		{
			auto result1 = lua.safe_script("obj = callable.new()\n"
				"v = obj(2, 'bark woof')", sol::script_pass_on_error);
			REQUIRE(result1.valid());
			int v = lua["v"];
			REQUIRE(v == 11);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<callable>("callable");
		{
			lua.safe_script("obj = callable.new()");
			lua.safe_script("v = obj(2, 'bark woof')");
			int v = lua["v"];
			REQUIRE(v == 11);
		}
	}
}

struct stringable {
	static const void* last_print_ptr;
};
const void* stringable::last_print_ptr = nullptr;

std::ostream& operator<<(std::ostream& ostr, const stringable& o) {
	stringable::last_print_ptr = static_cast<const void*>(&o);
	return ostr << "{ stringable, std::ostream! }";
}

struct adl_stringable {
	static const void* last_print_ptr;
};
const void* adl_stringable::last_print_ptr = nullptr;

std::string to_string(const adl_stringable& o) {
	adl_stringable::last_print_ptr = static_cast<const void*>(&o);
	return "{ adl_stringable, to_string! }";
}

namespace inside {
	struct adl_stringable2 {
		static const void* last_print_ptr;
	};
	const void* adl_stringable2::last_print_ptr = nullptr;

	std::string to_string(const adl_stringable2& o) {
		adl_stringable2::last_print_ptr = static_cast<const void*>(&o);
		return "{ inside::adl_stringable2, inside::to_string! }";
	}
} // namespace inside

struct member_stringable {
	static const void* last_print_ptr;

	std::string to_string() const {
		member_stringable::last_print_ptr = static_cast<const void*>(this);
		return "{ member_stringable, to_string! }";
	}
};
const void* member_stringable::last_print_ptr = nullptr;

TEST_CASE("operators/stringable", "test std::ostream stringability") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = stringable();
			auto result1 = lua.safe_script("print(obj)", sol::script_pass_on_error);
			REQUIRE(result1.valid());
			stringable& obj = lua["obj"];
			REQUIRE(stringable::last_print_ptr == &obj);
		}
	}
	SECTION("regular") {
		lua.new_usertype<stringable>("stringable");
		{
			auto result1 = lua.safe_script(R"(obj = stringable.new()
				print(obj) )", sol::script_pass_on_error);
			REQUIRE(result1.valid());
			stringable& obj = lua["obj"];
			REQUIRE(stringable::last_print_ptr == &obj);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<stringable>("stringable");
		{
			auto result1 = lua.safe_script(R"(obj = stringable.new()
				print(obj))", sol::script_pass_on_error);
			REQUIRE(result1.valid());
			stringable& obj = lua["obj"];
			REQUIRE(stringable::last_print_ptr == &obj);
		}
	}
}

TEST_CASE("operators/adl_stringable", "test adl to_string stringability") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = adl_stringable();
			lua.safe_script("print(obj)");
			adl_stringable& obj = lua["obj"];
			REQUIRE(adl_stringable::last_print_ptr == &obj);
		}
	}
	SECTION("regular") {
		lua.new_usertype<adl_stringable>("stringable");
		{
			lua["obj"] = adl_stringable();
			lua.safe_script("print(obj)");
			adl_stringable& obj = lua["obj"];
			REQUIRE(adl_stringable::last_print_ptr == &obj);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<adl_stringable>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			adl_stringable& obj = lua["obj"];
			REQUIRE(adl_stringable::last_print_ptr == &obj);
		}
	}
}

TEST_CASE("operators/inside::adl_stringable2", "test adl to_string stringability from inside a namespace") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = inside::adl_stringable2();
			lua.safe_script("print(obj)");
			inside::adl_stringable2& obj = lua["obj"];
			REQUIRE(inside::adl_stringable2::last_print_ptr == &obj);
		}
	}
	SECTION("regular") {
		lua.new_usertype<inside::adl_stringable2>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			inside::adl_stringable2& obj = lua["obj"];
			REQUIRE(inside::adl_stringable2::last_print_ptr == &obj);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<inside::adl_stringable2>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			inside::adl_stringable2& obj = lua["obj"];
			REQUIRE(inside::adl_stringable2::last_print_ptr == &obj);
		}
	}
}

TEST_CASE("operators/member_stringable", "test member to_string stringability") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = member_stringable();
			lua.safe_script("print(obj)");
			member_stringable& obj = lua["obj"];
			REQUIRE(member_stringable::last_print_ptr == &obj);
		}
	}
	SECTION("regular") {
		lua.new_usertype<member_stringable>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			member_stringable& obj = lua["obj"];
			REQUIRE(member_stringable::last_print_ptr == &obj);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<member_stringable>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			member_stringable& obj = lua["obj"];
			REQUIRE(member_stringable::last_print_ptr == &obj);
		}
	}
}

TEST_CASE("operators/container-like", "test that generic begin/end and iterator are automatically bound") {
#if SOL_LUA_VERSION > 501
	struct container {
		typedef int* iterator;
		typedef int value_type;

		value_type values[10];

		container() {
			std::iota(begin(), end(), 1);
		}

		iterator begin() {
			return &values[0];
		}

		iterator end() {
			return &values[0] + 10;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = container();
			lua.safe_script("i = 0 for k, v in pairs(obj) do i = i + 1 assert(k == v) end");
			std::size_t i = lua["i"];
			REQUIRE(i == 10);
		}
	}
	SECTION("regular") {
		lua.new_usertype<container>("container");
		{
			lua.safe_script("obj = container.new()");
			lua.safe_script("i = 0 for k, v in pairs(obj) do i = i + 1 assert(k == v) end");
			std::size_t i = lua["i"];
			REQUIRE(i == 10);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<container>("container");
		{
			lua.safe_script("obj = container.new()");
			lua.safe_script("i = 0 for k, v in pairs(obj) do i = i + 1 assert(k == v) end");
			std::size_t i = lua["i"];
			REQUIRE(i == 10);
		}
	}
#else
	SUCCEED("");
#endif
}

TEST_CASE("operators/length", "test that size is automatically bound to the length operator") {
	struct sizable {
		std::size_t size() const {
			return 6;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("plain") {
		{
			lua["obj"] = sizable();
			lua.safe_script("s = #obj");
			std::size_t s = lua["s"];
			REQUIRE(s == 6);
		}
	}
	SECTION("regular") {
		lua.new_usertype<sizable>("sizable");
		{
			lua.safe_script("obj = sizable.new()");
			lua.safe_script("s = #obj");
			std::size_t s = lua["s"];
			REQUIRE(s == 6);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<sizable>("sizable");
		{
			lua.safe_script("obj = sizable.new()");
			lua.safe_script("s = #obj");
			std::size_t s = lua["s"];
			REQUIRE(s == 6);
		}
	}
}
