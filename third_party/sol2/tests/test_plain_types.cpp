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

#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1

#include <sol.hpp>
#include <catch.hpp>

TEST_CASE("plain/alignment", "test that aligned classes in certain compilers don't trigger compiler errors") {
#ifdef _MSC_VER
	__declspec(align(16)) struct aligned_class {
		int var;
	};

	struct A {
		aligned_class a;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math);

	lua.new_usertype<A>("A");
	A a;
	lua["a"] = &a;
	A& la = lua["a"];
	REQUIRE(&a == &la);
#else
	REQUIRE(true);
#endif // VC++ stuff
}

TEST_CASE("plain/indestructible", "test that we error for types that are innately indestructible") {
	struct indestructible {
	public:
		int v = 20;

		struct insider {
			void operator()(indestructible* i) const {
				i->~indestructible();
			}
		};

	private:
		~indestructible() {
		}
	};

	SECTION("doomed") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unique_ptr<indestructible, indestructible::insider> i = sol::detail::make_unique_deleter<indestructible, indestructible::insider>();
		lua["i"] = *i;
		lua.safe_script("i = nil");
		auto result = lua.safe_script("collectgarbage()", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	SECTION("saved") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unique_ptr<indestructible, indestructible::insider> i = sol::detail::make_unique_deleter<indestructible, indestructible::insider>();
		lua["i"] = *i;
		lua.new_usertype<indestructible>("indestructible",
			sol::default_constructor,
			sol::meta_function::garbage_collect, sol::destructor([](indestructible& i) {
				indestructible::insider del;
				del(&i);
			}));
		lua.safe_script("i = nil");
		auto result = lua.safe_script("collectgarbage()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("plain/constructors and destructors", "Make sure that constructors, destructors, deallocators and others work properly with the desired type") {
	static int constructed = 0;
	static int destructed = 0;
	static int copied = 0;
	static int moved = 0;

	struct st {
		int value = 10;

		st() : value(10) {
			++constructed;
		}

		st(const st& o) : value(o.value) {
			++copied;
			++constructed;
		}

		st(st&& o) : value(o.value) {
			++moved;
			++constructed;
		}

		~st() {
			value = 0;
			++destructed;
		}
	};

	struct deallocate_only {
		void operator()(st* p) const {
			std::allocator<st> alloc;
			alloc.deallocate(p, 1);
		}
	};

	{
		sol::state lua;

		lua["f"] = sol::constructors<st()>();
		lua["g"] = sol::initializers([](st* mem) { std::allocator<st> alloc; std::allocator_traits<std::allocator<st>>::construct(alloc, mem); });
		lua["h"] = sol::factories([]() { return st(); });
		lua["d"] = sol::destructor([](st& m) { m.~st(); });

		sol::protected_function_result result1 = lua.safe_script("v = f()", &sol::script_pass_on_error);
		REQUIRE(result1.valid());
		st& v = lua["v"];
		REQUIRE(v.value == 10);
		REQUIRE(constructed == 1);
		REQUIRE(destructed == 0);
		{
			std::unique_ptr<st, deallocate_only> unsafe(new st());
			lua["unsafe"] = unsafe.get();
			sol::protected_function_result result2 = lua.safe_script("d(unsafe)", &sol::script_pass_on_error);
			REQUIRE(result2.valid());
			REQUIRE(constructed == 2);
			REQUIRE(destructed == 1);
		}
		REQUIRE(constructed == 2);
		REQUIRE(destructed == 1);

		{
			sol::protected_function_result result3 = lua.safe_script("vg = g()", &sol::script_pass_on_error);
			REQUIRE(result3.valid());
			st& vg = lua["vg"];
			REQUIRE(vg.value == 10);
			REQUIRE(constructed == 3);
			REQUIRE(destructed == 1);
		}

		{
			sol::protected_function_result result4 = lua.safe_script("vh = h()", &sol::script_pass_on_error);
			REQUIRE(result4.valid());
			st& vh = lua["vh"];
			REQUIRE(vh.value == 10);
		}
	}
	int purely_constructed = constructed - moved - copied;
	int purely_destructed = destructed - moved - copied;
	REQUIRE(constructed == destructed);
	REQUIRE(purely_constructed == purely_destructed);
	REQUIRE(purely_constructed == 4);
	REQUIRE(purely_destructed == 4);
}
