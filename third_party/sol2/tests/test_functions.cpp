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

#include <iostream>

template <typename T>
T va_func(sol::variadic_args va, T first) {
	T s = 0;
	for (auto arg : va) {
		T v = arg;
		s += v;
	}
	std::cout << first << std::endl;
	std::cout << s << std::endl;

	return s;
}

std::function<int()> makefn() {
	auto fx = []() -> int {
		return 0x1456789;
	};
	return fx;
}

void takefn(std::function<int()> purr) {
	if (purr() != 0x1456789)
		throw 0;
}

struct A {
	int a = 0xA;
	int bark() {
		return 1;
	}
};

std::tuple<int, int> bark(int num_value, A* a) {
	return std::tuple<int, int>(num_value * 2, a->bark());
}

void test_free_func(std::function<void()> f) {
	f();
}

void test_free_func2(std::function<int(int)> f, int arg1) {
	int val = f(arg1);
	REQUIRE(val == arg1);
}

int overloaded(int x) {
	INFO(x);
	return 3;
}

int overloaded(int x, int y) {
	INFO(x << " " << y);
	return 7;
}

int overloaded(int x, int y, int z) {
	INFO(x << " " << y << " " << z);
	return 11;
}

int non_overloaded(int x, int y, int z) {
	INFO(x << " " << y << " " << z);
	return 13;
}

namespace sep {
	int plop_xyz(int x, int y, std::string z) {
		INFO(x << " " << y << " " << z);
		return 11;
	}
} // namespace sep

int func_1(int) {
	return 1;
}

std::string func_1s(std::string a) {
	return "string: " + a;
}

int func_2(int, int) {
	return 2;
}

void func_3(int, int, int) {
}

int f1(int) {
	return 32;
}
int f2(int, int) {
	return 1;
}
struct fer {
	double f3(int, int) {
		return 2.5;
	}
};

static int raw_noexcept_function(lua_State* L) noexcept {
	return sol::stack::push(L, 0x63);
}

TEST_CASE("functions/tuple returns", "Make sure tuple returns are ordered properly") {
	sol::state lua;
	auto result1 = lua.safe_script("function f() return '3', 4 end", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	std::tuple<std::string, int> result = lua["f"]();
	auto s = std::get<0>(result);
	auto v = std::get<1>(result);
	REQUIRE(s == "3");
	REQUIRE(v == 4);
}

TEST_CASE("functions/overload resolution", "Check if overloaded function resolution templates compile/work") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("non_overloaded", non_overloaded);
	{
		auto result = lua.safe_script("x = non_overloaded(1, 2, 3)\nprint(x)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	/*
	// Cannot reasonably support: clang++ refuses to try enough
	// deductions to make this work
	lua.set_function<int>("overloaded", overloaded);
	{ auto result = lua.safe_script("print(overloaded(1))", sol::script_pass_on_error); REQUIRE(result.valid()); }

	lua.set_function<int, int>("overloaded", overloaded);
	{ auto result = lua.safe_script("print(overloaded(1, 2))", sol::script_pass_on_error); REQUIRE(result.valid()); }

	lua.set_function<int, int, int>("overloaded", overloaded);
	{ auto result = lua.safe_script("print(overloaded(1, 2, 3))", sol::script_pass_on_error); REQUIRE(result.valid()); }
	*/
	lua.set_function("overloaded", sol::resolve<int(int)>(overloaded));
	{
		auto result = lua.safe_script("print(overloaded(1))", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	lua.set_function("overloaded", sol::resolve<int(int, int)>(overloaded));
	{
		auto result = lua.safe_script("print(overloaded(1, 2))", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	lua.set_function("overloaded", sol::resolve<int(int, int, int)>(overloaded));
	{
		auto result = lua.safe_script("print(overloaded(1, 2, 3))", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("functions/return order and multi get", "Check if return order is in the same reading order specified in Lua") {
	const static std::tuple<int, int, int> triple = std::make_tuple(10, 11, 12);
	const static std::tuple<int, float> paired = std::make_tuple(10, 10.f);
	sol::state lua;
	lua.set_function("f", [] {
		return std::make_tuple(10, 11, 12);
	});
	lua.set_function("h", []() {
		return std::make_tuple(10, 10.0f);
	});
	auto result1 = lua.safe_script("function g() return 10, 11, 12 end\nx,y,z = g()", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	auto tcpp = lua.get<sol::function>("f").call<int, int, int>();
	auto tlua = lua.get<sol::function>("g").call<int, int, int>();
	auto tcpp2 = lua.get<sol::function>("h").call<int, float>();
	auto tluaget = lua.get<int, int, int>("x", "y", "z");
	REQUIRE(tcpp == triple);
	REQUIRE(tlua == triple);
	REQUIRE(tluaget == triple);
	REQUIRE(tcpp2 == paired);
}

TEST_CASE("functions/deducing return order and multi get", "Check if return order is in the same reading order specified in Lua, with regular deducing calls") {
	const static std::tuple<int, int, int> triple = std::make_tuple(10, 11, 12);
	sol::state lua;
	lua.set_function("f_string", []() { return "this is a string!"; });
	sol::function f_string = lua["f_string"];

	// Make sure there are no overload collisions / compiler errors for automatic string conversions
	std::string f_string_result = f_string();
	REQUIRE(f_string_result == "this is a string!");
	f_string_result = f_string();
	REQUIRE(f_string_result == "this is a string!");

	lua.set_function("f", [] {
		return std::make_tuple(10, 11, 12);
	});
	auto result1 = lua.safe_script("function g() return 10, 11, 12 end\nx,y,z = g()", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	std::tuple<int, int, int> tcpp = lua.get<sol::function>("f")();
	std::tuple<int, int, int> tlua = lua.get<sol::function>("g")();
	std::tuple<int, int, int> tluaget = lua.get<int, int, int>("x", "y", "z");
	INFO("cpp: " << std::get<0>(tcpp) << ',' << std::get<1>(tcpp) << ',' << std::get<2>(tcpp));
	INFO("lua: " << std::get<0>(tlua) << ',' << std::get<1>(tlua) << ',' << std::get<2>(tlua));
	INFO("lua xyz: " << lua.get<int>("x") << ',' << lua.get<int>("y") << ',' << lua.get<int>("z"));
	REQUIRE(tcpp == triple);
	REQUIRE(tlua == triple);
	REQUIRE(tluaget == triple);
}

TEST_CASE("functions/optional values", "check if optionals can be passed in to be nil or otherwise") {
	struct thing {
		int v;
	};
	sol::state lua;
	auto result1 = lua.safe_script(R"( function f (a)
    return a
end )", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	sol::function lua_bark = lua["f"];

	sol::optional<int> testv = lua_bark(sol::optional<int>(29));
	sol::optional<int> testn = lua_bark(sol::nullopt);
	REQUIRE((bool)testv);
	REQUIRE_FALSE((bool)testn);
	REQUIRE(testv.value() == 29);
	sol::optional<thing> v = lua_bark(sol::optional<thing>(thing{ 29 }));
	REQUIRE_NOTHROW([&] {sol::lua_nil_t n = lua_bark(sol::nullopt); return n; }());
	REQUIRE(v->v == 29);
}

TEST_CASE("functions/pair and tuple and proxy tests", "Check if sol::reference and sol::proxy can be passed to functions as arguments") {
	sol::state lua;
	lua.new_usertype<A>("A",
		"bark", &A::bark);
	auto result1 = lua.safe_script(R"( function f (num_value, a)
    return num_value * 2, a:bark()
end 
function h (num_value, a, b)
    return num_value * 2, a:bark(), b * 3
end 
nested = { variables = { no = { problem = 10 } } } )", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	lua.set_function("g", bark);

	sol::function cpp_bark = lua["g"];
	sol::function lua_bark = lua["f"];
	sol::function lua_bark2 = lua["h"];

	sol::reference lua_variable_x = lua["nested"]["variables"]["no"]["problem"];
	A cpp_variable_y;

	static const std::tuple<int, int> abdesired(20, 1);
	static const std::pair<int, int> cddesired = { 20, 1 };
	static const std::tuple<int, int, int> abcdesired(20, 1, 3);

	std::tuple<int, int> ab = cpp_bark(lua_variable_x, cpp_variable_y);
	std::pair<int, int> cd = lua_bark(lua["nested"]["variables"]["no"]["problem"], cpp_variable_y);

	REQUIRE(ab == abdesired);
	REQUIRE(cd == cddesired);

	ab = cpp_bark(std::make_pair(lua_variable_x, cpp_variable_y));
	cd = static_cast<std::pair<int, int>>(lua_bark(std::make_pair(lua["nested"]["variables"]["no"]["problem"], cpp_variable_y)));

	REQUIRE(ab == abdesired);
	REQUIRE(cd == cddesired);

	std::tuple<int, int, int> abc = lua_bark2(std::make_tuple(10, cpp_variable_y), sol::optional<int>(1));
	REQUIRE(abc == abcdesired);
}

TEST_CASE("functions/sol::function to std::function", "check if conversion to std::function works properly and calls with correct arguments") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("testFunc", test_free_func);
	lua.set_function("testFunc2", test_free_func2);
	auto result1 = lua.safe_script("testFunc(function() print(\"hello std::function\") end)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	{
		auto result = lua.safe_script(
			"function m(a)\n"
			"     print(\"hello std::function with arg \", a)\n"
			"     return a\n"
			"end\n"
			"\n"
			"testFunc2(m, 1)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("functions/returning functions from C++", "check to see if returning a functor and getting a functor from lua is possible") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("makefn", makefn);
	lua.set_function("takefn", takefn);
	{
		auto result = lua.safe_script(
			"afx = makefn()\n"
			"print(afx())\n"
			"takefn(afx)\n", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("functions/function_result and protected_function_result", "Function result should be the beefy return type for sol::function that allows for error checking and error handlers") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::debug);
	static const char unhandlederrormessage[] = "true error message";
	static const char handlederrormessage[] = "doodle";
	static const std::string handlederrormessage_s = handlederrormessage;

	// Some function; just using a lambda to be cheap
	auto doomfx = []() {
		throw std::runtime_error(unhandlederrormessage);
	};
	lua.set_function("doom", doomfx);

	auto cpphandlerfx = [](std::string x) {
		INFO("c++ handler called with: " << x);
		return handlederrormessage;
	};
	lua.set_function("cpphandler", cpphandlerfx);
	
	auto result1 = lua.safe_script(
		std::string("function luahandler ( message )")
		+ "    print('lua handler called with: ' .. message)"
		+ "    return '" + handlederrormessage + "'"
		+ "end", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	
	auto nontrampolinefx = [](lua_State* L) -> int {
		return luaL_error(L, "x");
	};
	lua_CFunction c_nontrampolinefx = nontrampolinefx;
	lua.set("nontrampoline", c_nontrampolinefx);
	
	lua.set_function("bark", []() -> int { return 100; });

	sol::function luahandler = lua["luahandler"];
	sol::function cpphandler = lua["cpphandler"];
	sol::protected_function doom(lua["doom"], luahandler);
	sol::protected_function nontrampoline(lua["nontrampoline"], cpphandler);
	sol::protected_function justfine = lua["bark"];
	sol::protected_function justfinewithhandler = lua["bark"];

	justfinewithhandler.error_handler = luahandler;
	bool present = true;
	{
		sol::protected_function_result result = doom();
		REQUIRE_FALSE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE(present);
		present = (bool)opvalue;
		REQUIRE_FALSE(present);
		sol::error err = result;
		REQUIRE(err.what() == handlederrormessage_s);
	}
	{
		sol::protected_function_result result = nontrampoline();
		REQUIRE_FALSE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE(present);
		present = (bool)opvalue;
		REQUIRE_FALSE(present);
		sol::error err = result;
		REQUIRE(err.what() == handlederrormessage_s);
	}
	{
		sol::protected_function_result result = justfine();
		REQUIRE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE_FALSE(present);
		present = (bool)opvalue;
		REQUIRE(present);
		int value = result;
		REQUIRE(value == 100);
	}
	{
		sol::protected_function_result result = justfinewithhandler();
		REQUIRE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE_FALSE(present);
		present = (bool)opvalue;
		REQUIRE(present);
		int value = result;
		REQUIRE(value == 100);
	}
}

#if !defined(SOL2_CI) && ((!defined(_M_IX86) || defined(_M_IA64)) || (defined(_WIN64)) || (defined(__LLP64__) || defined(__LP64__)) )
TEST_CASE("functions/unsafe protected_function_result handlers", "This test will thrash the stack and allocations on weaker compilers (e.g., non 64-bit ones). Run with caution.") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::debug);
	static const char unhandlederrormessage[] = "true error message";
	static const char handlederrormessage[] = "doodle";
	static const std::string handlederrormessage_s = handlederrormessage;

	auto luadoomfx = [&lua]() {
		// Does not bypass error function, will call it
		// however, this bypasses `catch` state
		// in trampoline entirely...
		luaL_error(lua.lua_state(), unhandlederrormessage);
	};
	lua.set_function("luadoom", luadoomfx);

	auto cpphandlerfx = [](std::string x) {
		INFO("c++ handler called with: " << x);
		return handlederrormessage;
	};
	lua.set_function("cpphandler", cpphandlerfx);
	auto nontrampolinefx = [](lua_State*) -> int { 
		// this code shoots an exception
		// through the C API, without the trampoline
		// present.
		// it is probably guaranteed to kill our code.
		throw "x"; 
	};
	lua_CFunction c_nontrampolinefx = nontrampolinefx;
	lua.set("nontrampoline", c_nontrampolinefx);
	lua.set_function("bark", []() -> int { return 100; });

	sol::function cpphandler = lua["cpphandler"];
	sol::protected_function luadoom(lua["luadoom"]);
	sol::protected_function nontrampoline = lua["nontrampoline"];
	luadoom.error_handler = cpphandler;
	nontrampoline.error_handler = cpphandler;
	
	bool present = true;
	{
		sol::protected_function_result result = luadoom();
		REQUIRE_FALSE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE(present);
		present = (bool)opvalue;
		REQUIRE_FALSE(present);
		sol::error err = result;
		REQUIRE(err.what() == handlederrormessage_s);
	}
	{
		sol::protected_function_result result = nontrampoline();
		REQUIRE_FALSE(result.valid());
		sol::optional<sol::error> operr = result;
		sol::optional<int> opvalue = result;
		present = (bool)operr;
		REQUIRE(present);
		present = (bool)opvalue;
		REQUIRE_FALSE(present);
		sol::error err = result;
		REQUIRE(err.what() == handlederrormessage_s);
	}
}
#endif // This test will thrash the stack and allocations on weaker compilers

TEST_CASE("functions/all kinds", "Register all kinds of functions, make sure they all compile and work") {
	sol::state lua;

	struct test_1 {
		int a = 0xA;
		virtual int bark() {
			return a;
		}

		int bark_mem() {
			return a;
		}

		static std::tuple<int, int> x_bark(int num_value, test_1* a) {
			return std::tuple<int, int>(num_value * 2, a->a);
		}
	};

	struct test_2 {
		int a = 0xC;
		int bark() {
			return 20;
		}
	};

	struct inner {
		const int z = 5653;
	};

	struct nested {
		inner i;
	};

	auto a = []() { return 500; };
	auto b = [&]() { return 501; };
	auto c = [&]() { return 502; };
	auto d = []() { return 503; };

	lua.new_usertype<test_1>("test_1",
		"bark", sol::c_call<decltype(&test_1::bark_mem), &test_1::bark_mem>);
	lua.new_usertype<test_2>("test_2",
		"bark", sol::c_call<decltype(&test_2::bark), &test_2::bark>);
	test_2 t2;

	lua.set_function("a", a);
	lua.set_function("b", b);
	lua.set_function("c", std::ref(c));
	lua.set_function("d", std::ref(d));
	lua.set_function("f", &test_1::bark);
	lua.set_function("g", test_1::x_bark);
	lua.set_function("h", sol::c_call<decltype(&test_1::bark_mem), &test_1::bark_mem>);
	lua.set_function("i", &test_2::bark, test_2());
	lua.set_function("j", &test_2::a, test_2());
	lua.set_function("k", &test_2::a);
	lua.set_function("l", sol::c_call<decltype(&test_1::a), &test_1::a>);
	lua.set_function("m", &test_2::a, &t2);
	lua.set_function("n", sol::c_call<decltype(&non_overloaded), &non_overloaded>);

	auto result1 = lua.safe_script(R"(
o1 = test_1.new()
o2 = test_2.new()
)", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	auto result2 = lua.safe_script(R"(
ob = o1:bark()

A = a()
B = b()
C = c()
D = d()
F = f(o1)
G0, G1 = g(2, o1)
H = h(o1)
I = i(o1)
I = i(o1)
)", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	auto result3 = lua.safe_script(R"(
J0 = j()
j(24)
J1 = j()
    )", sol::script_pass_on_error);
	REQUIRE(result3.valid());

	auto result4 = lua.safe_script(R"(
K0 = k(o2)
k(o2, 1024)
K1 = k(o2)
    )", sol::script_pass_on_error);
	REQUIRE(result4.valid());

	auto result5 = lua.safe_script(R"(
L0 = l(o1)
l(o1, 678)
L1 = l(o1)
    )", sol::script_pass_on_error);
	REQUIRE(result5.valid());

	auto result6 = lua.safe_script(R"(
M0 = m()
m(256)
M1 = m()
    )", sol::script_pass_on_error);
	REQUIRE(result6.valid());

	auto result7 = lua.safe_script(R"(
N = n(1, 2, 3)
    )", sol::script_pass_on_error);
	REQUIRE(result7.valid());

	int ob, A, B, C, D, F, G0, G1, H, I, J0, J1, K0, K1, L0, L1, M0, M1, N;
	std::tie(ob, A, B, C, D, F, G0, G1, H, I, J0, J1, K0, K1, L0, L1, M0, M1, N)
		= lua.get<int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int>(
			"ob", "A", "B", "C", "D", "F", "G0", "G1", "H", "I", "J0", "J1", "K0", "K1", "L0", "L1", "M0", "M1", "N");

	REQUIRE(ob == 0xA);

	REQUIRE(A == 500);
	REQUIRE(B == 501);
	REQUIRE(C == 502);
	REQUIRE(D == 503);

	REQUIRE(F == 0xA);
	REQUIRE(G0 == 4);
	REQUIRE(G1 == 0xA);
	REQUIRE(H == 0xA);
	REQUIRE(I == 20);

	REQUIRE(J0 == 0xC);
	REQUIRE(J1 == 24);

	REQUIRE(K0 == 0xC);
	REQUIRE(K1 == 1024);

	REQUIRE(L0 == 0xA);
	REQUIRE(L1 == 678);

	REQUIRE(M0 == 0xC);
	REQUIRE(M1 == 256);

	REQUIRE(N == 13);

	sol::tie(ob, A, B, C, D, F, G0, G1, H, I, J0, J1, K0, K1, L0, L1, M0, M1, N)
		= lua.get<int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int>(
			"ob", "A", "B", "C", "D", "F", "G0", "G1", "H", "I", "J0", "J1", "K0", "K1", "L0", "L1", "M0", "M1", "N");

	REQUIRE(ob == 0xA);

	REQUIRE(A == 500);
	REQUIRE(B == 501);
	REQUIRE(C == 502);
	REQUIRE(D == 503);

	REQUIRE(F == 0xA);
	REQUIRE(G0 == 4);
	REQUIRE(G1 == 0xA);
	REQUIRE(H == 0xA);
	REQUIRE(I == 20);

	REQUIRE(J0 == 0xC);
	REQUIRE(J1 == 24);

	REQUIRE(K0 == 0xC);
	REQUIRE(K1 == 1024);

	REQUIRE(L0 == 0xA);
	REQUIRE(L1 == 678);

	REQUIRE(M0 == 0xC);
	REQUIRE(M1 == 256);

	REQUIRE(N == 13);

	// Work that compiler, WORK IT!
	lua.set("o", &test_1::bark);
	lua.set("p", test_1::x_bark);
	lua.set("q", sol::c_call<decltype(&test_1::bark_mem), &test_1::bark_mem>);
	lua.set("r", &test_2::a);
	lua.set("s", sol::readonly(&test_2::a));
	lua.set_function("t", sol::readonly(&test_2::a), test_2());
	lua.set_function("u", &nested::i, nested());
	lua.set("v", &nested::i);
	lua.set("nested", nested());
	lua.set("inner", inner());
	{
		auto result = lua.safe_script("s(o2, 2)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("t(2)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("u(inner)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("v(nested, inner)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("simple/call with parameters", "Lua function is called with a few parameters from C++") {
	sol::state lua;

	{
		auto result = lua.safe_script("function my_add(i, j, k) return i + j + k end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	auto f = lua.get<sol::function>("my_add");
	{
		auto result = lua.safe_script("function my_nothing(i, j, k) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	auto fvoid = lua.get<sol::function>("my_nothing");
	REQUIRE_NOTHROW([&]() {
		fvoid(1, 2, 3);
	}());
	REQUIRE_NOTHROW([&]() {
		int a = f.call<int>(1, 2, 3);
		REQUIRE(a == 6);
	}());
	sol::protected_function pf = f;
	REQUIRE_NOTHROW([&]() {
		sol::protected_function_result pfr = pf(1, 2, "arf");
		REQUIRE_FALSE(pfr.valid());
	}());
}

TEST_CASE("simple/call c++ function", "C++ function is called from lua") {
	sol::state lua;

	lua.set_function("plop_xyz", sep::plop_xyz);
	auto result1 = lua.safe_script("x = plop_xyz(2, 6, 'hello')", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	REQUIRE(lua.get<int>("x") == 11);
}

TEST_CASE("simple/call lambda", "A C++ lambda is exposed to lua and called") {
	sol::state lua;

	int a = 0;

	lua.set_function("foo", [&a] { a = 1; });

	auto result1 = lua.safe_script("foo()", sol::script_pass_on_error);
	REQUIRE(result1.valid());

	REQUIRE(a == 1);
}

TEST_CASE("advanced/get and call", "Checks for lambdas returning values after a get operation") {
	const static std::string lol = "lol", str = "str";
	const static std::tuple<int, float, double, std::string> heh_tuple = std::make_tuple(1, 6.28f, 3.14, std::string("heh"));
	sol::state lua;

	REQUIRE_NOTHROW(lua.set_function("a", [] { return 42; }));
	REQUIRE(lua.get<sol::function>("a").call<int>() == 42);

	REQUIRE_NOTHROW(lua.set_function("b", [] { return 42u; }));
	REQUIRE(lua.get<sol::function>("b").call<unsigned int>() == 42u);

	REQUIRE_NOTHROW(lua.set_function("c", [] { return 3.14; }));
	REQUIRE(lua.get<sol::function>("c").call<double>() == 3.14);

	REQUIRE_NOTHROW(lua.set_function("d", [] { return 6.28f; }));
	REQUIRE(lua.get<sol::function>("d").call<float>() == 6.28f);

	REQUIRE_NOTHROW(lua.set_function("e", [] { return "lol"; }));
	REQUIRE(lua.get<sol::function>("e").call<std::string>() == lol);

	REQUIRE_NOTHROW(lua.set_function("f", [] { return true; }));
	REQUIRE(lua.get<sol::function>("f").call<bool>());

	REQUIRE_NOTHROW(lua.set_function("g", [] { return std::string("str"); }));
	REQUIRE(lua.get<sol::function>("g").call<std::string>() == str);

	REQUIRE_NOTHROW(lua.set_function("h", [] {}));
	REQUIRE_NOTHROW(lua.get<sol::function>("h").call());

	REQUIRE_NOTHROW(lua.set_function("i", [] { return sol::lua_nil; }));
	REQUIRE(lua.get<sol::function>("i").call<sol::lua_nil_t>() == sol::lua_nil);
	REQUIRE_NOTHROW(lua.set_function("j", [] { return std::make_tuple(1, 6.28f, 3.14, std::string("heh")); }));
	REQUIRE((lua.get<sol::function>("j").call<int, float, double, std::string>() == heh_tuple));
}

TEST_CASE("advanced/operator[] call", "Checks for lambdas returning values using operator[]") {
	const static std::string lol = "lol", str = "str";
	const static std::tuple<int, float, double, std::string> heh_tuple = std::make_tuple(1, 6.28f, 3.14, std::string("heh"));
	sol::state lua;

	REQUIRE_NOTHROW(lua.set_function("a", [] { return 42; }));
	REQUIRE(lua["a"].call<int>() == 42);

	REQUIRE_NOTHROW(lua.set_function("b", [] { return 42u; }));
	REQUIRE(lua["b"].call<unsigned int>() == 42u);

	REQUIRE_NOTHROW(lua.set_function("c", [] { return 3.14; }));
	REQUIRE(lua["c"].call<double>() == 3.14);

	REQUIRE_NOTHROW(lua.set_function("d", [] { return 6.28f; }));
	REQUIRE(lua["d"].call<float>() == 6.28f);

	REQUIRE_NOTHROW(lua.set_function("e", [] { return "lol"; }));
	REQUIRE(lua["e"].call<std::string>() == lol);

	REQUIRE_NOTHROW(lua.set_function("f", [] { return true; }));
	REQUIRE(lua["f"].call<bool>());

	REQUIRE_NOTHROW(lua.set_function("g", [] { return std::string("str"); }));
	REQUIRE(lua["g"].call<std::string>() == str);

	REQUIRE_NOTHROW(lua.set_function("h", [] {}));
	REQUIRE_NOTHROW(lua["h"].call());

	REQUIRE_NOTHROW(lua.set_function("i", [] { return sol::lua_nil; }));
	REQUIRE(lua["i"].call<sol::lua_nil_t>() == sol::lua_nil);
	REQUIRE_NOTHROW(lua.set_function("j", [] { return std::make_tuple(1, 6.28f, 3.14, std::string("heh")); }));
	REQUIRE((lua["j"].call<int, float, double, std::string>() == heh_tuple));
}

TEST_CASE("advanced/call lambdas", "A C++ lambda is exposed to lua and called") {
	sol::state lua;

	int x = 0;
	lua.set_function("set_x", [&](int new_x) {
		x = new_x;
		return 0;
	});

	auto result1 = lua.safe_script("set_x(9)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	REQUIRE(x == 9);
}

TEST_CASE("advanced/call referenced obj", "A C++ object is passed by pointer/reference_wrapper to lua and invoked") {
	sol::state lua;

	int x = 0;
	auto objx = [&](int new_x) {
		x = new_x;
		return 0;
	};
	lua.set_function("set_x", std::ref(objx));

	int y = 0;
	auto objy = [&](int new_y) {
		y = new_y;
		return std::tuple<int, int>(0, 0);
	};
	lua.set_function("set_y", &decltype(objy)::operator(), std::ref(objy));

	auto result1 = lua.safe_script("set_x(9)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("set_y(9)", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	REQUIRE(x == 9);
	REQUIRE(y == 9);
}

TEST_CASE("functions/tie", "make sure advanced syntax with 'tie' works") {
	sol::state lua;

	auto result1 = lua.safe_script(R"(function f () 
    return 1, 2, 3 
end)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	sol::function f = lua["f"];

	int a, b, c;
	sol::tie(a, b, c) = f();
	REQUIRE(a == 1);
	REQUIRE(b == 2);
	REQUIRE(c == 3);
}

TEST_CASE("functions/overloading", "Check if overloading works properly for regular set function syntax") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("func_1", func_1);
	lua.set_function("func", sol::overload(func_2, func_3, func_1, func_1s));

	const std::string string_bark = "string: bark";

	{
		auto result = lua.safe_script(
			"a = func(1)\n"
			"b = func('bark')\n"
			"c = func(1,2)\n"
			"func(1,2,3)\n", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	REQUIRE((lua["a"] == 1));
	REQUIRE((lua["b"] == string_bark));
	REQUIRE((lua["c"] == 2));

	{
		auto result = lua.safe_script("func(1,2,'meow')", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("overloading/c_call", "Make sure that overloading works with c_call functionality") {
	sol::state lua;
	lua.set("f", sol::c_call<sol::wrap<decltype(&f1), &f1>, sol::wrap<decltype(&f2), &f2>, sol::wrap<decltype(&fer::f3), &fer::f3>>);
	lua.set("g", sol::c_call<sol::wrap<decltype(&f1), &f1>>);
	lua.set("h", sol::c_call<decltype(&f2), &f2>);
	lua.set("obj", fer());

	lua.safe_script("r1 = f(1)");
	lua.safe_script("r2 = f(1, 2)");
	lua.safe_script("r3 = f(obj, 1, 2)");
	lua.safe_script("r4 = g(1)");
	lua.safe_script("r5 = h(1, 2)");

	int r1 = lua["r1"];
	int r2 = lua["r2"];
	double r3 = lua["r3"];
	int r4 = lua["r4"];
	int r5 = lua["r5"];

	REQUIRE(r1 == 32);
	REQUIRE(r2 == 1);
	REQUIRE(r3 == 2.5);
	REQUIRE(r4 == 32);
	REQUIRE(r5 == 1);
}

TEST_CASE("functions/stack atomic", "make sure functions don't impede on the stack") {
	//setup sol/lua
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::string);

	lua.safe_script("function ErrorHandler(msg) print('Lua created error msg : ' ..  msg) return msg end");
	lua.safe_script("function stringtest(a) if a == nil then error('fuck') end print('Lua recieved content : ' .. a) return a end");

	// test normal function
	{
		sol::stack_guard normalsg(lua);
		std::string str = lua["stringtest"]("normal test");
		INFO("Back in C++, direct call result is : " << str);
	}

	//test protected_function
	sol::protected_function Stringtest(lua["stringtest"]);
	Stringtest.error_handler = lua["ErrorHandler"];
	sol::stack_guard sg(lua);
	{
		sol::protected_function_result stringresult = Stringtest("protected test");
		REQUIRE(stringresult.valid());
		std::string s = stringresult;
		INFO("Back in C++, protected result is : " << s);
	}
	REQUIRE(sg.check_stack());

	//test optional
	{
		sol::stack_guard opsg(lua);
		sol::optional<std::string> opt_result = Stringtest("optional test");
		REQUIRE(opsg.check_stack());
		if (opt_result) {
			std::string s = opt_result.value();
			INFO("Back in C++, opt_result is : " << s);
		}
		else {
			INFO("opt_result failed");
		}
	}
	REQUIRE(sg.check_stack());

	{
		sol::protected_function_result errresult = Stringtest(sol::lua_nil);
		REQUIRE_FALSE(errresult.valid());
		sol::error err = errresult;
		std::string msg = err.what();
		INFO("error :" << msg);
	}
	REQUIRE(sg.check_stack());
}

TEST_CASE("functions/stack multi-return", "Make sure the stack is protected after multi-returns") {
	sol::state lua;
	lua.safe_script("function f () return 1, 2, 3, 4, 5 end");

	{
		sol::stack_guard sg(lua);
		sol::stack::push(lua, double(256.78));
		{
			int a, b, c, d, e;
			sol::stack_guard sg2(lua);
			sol::function f = lua["f"];
			sol::tie(a, b, c, d, e) = f();
			REQUIRE(a == 1);
			REQUIRE(b == 2);
			REQUIRE(c == 3);
			REQUIRE(d == 4);
			REQUIRE(e == 5);
		}
		double f = sol::stack::pop<double>(lua);
		REQUIRE(f == 256.78);
	}
}

TEST_CASE("functions/protected stack multi-return", "Make sure the stack is protected after multi-returns") {
	sol::state lua;
	lua.safe_script("function f () return 1, 2, 3, 4, 5 end");

	{
		sol::stack_guard sg(lua);
		sol::stack::push(lua, double(256.78));
		{
			int a, b, c, d, e;
			sol::stack_guard sg2(lua);
			sol::protected_function pf = lua["f"];
			sol::tie(a, b, c, d, e) = pf();
			REQUIRE(a == 1);
			REQUIRE(b == 2);
			REQUIRE(c == 3);
			REQUIRE(d == 4);
			REQUIRE(e == 5);
		}
		double f = sol::stack::pop<double>(lua);
		REQUIRE(f == 256.78);
	}
}

TEST_CASE("functions/function_result as arguments", "ensure that function_result can be pushed as its results and not a userdata") {
	sol::state lua;
	lua.open_libraries();

	lua.safe_script("function f () return 1, 2, 3, 4, 5 end");
	lua.safe_script("function g (a, b, c, d, e) assert(a == 1) assert(b == 2) assert(c == 3) assert(d == 4) assert(e == 5) end");

	{
		sol::stack_guard sg(lua);
		sol::stack::push(lua, double(256.78));
		{
			int a, b, c, d, e;
			sol::stack_guard sg2(lua);
			sol::function pf = lua["f"];
			sol::tie(a, b, c, d, e) = pf();
			REQUIRE(a == 1);
			REQUIRE(b == 2);
			REQUIRE(c == 3);
			REQUIRE(d == 4);
			REQUIRE(e == 5);
			REQUIRE_NOTHROW([&]() {
				lua["g"](pf());
			}());
		}
		double f = sol::stack::pop<double>(lua);
		REQUIRE(f == 256.78);
	}
}

TEST_CASE("functions/protected_function_result as arguments", "ensure that protected_function_result can be pushed as its results and not a userdata") {
	sol::state lua;
	lua.open_libraries();

	lua.safe_script("function f () return 1, 2, 3, 4, 5 end");
	lua.safe_script("function g (a, b, c, d, e) assert(a == 1) assert(b == 2) assert(c == 3) assert(d == 4) assert(e == 5) end");

	{
		sol::stack_guard sg(lua);
		sol::stack::push(lua, double(256.78));
		{
			int a, b, c, d, e;
			sol::stack_guard sg2(lua);
			sol::protected_function pf = lua["f"];
			sol::tie(a, b, c, d, e) = pf();
			REQUIRE(a == 1);
			REQUIRE(b == 2);
			REQUIRE(c == 3);
			REQUIRE(d == 4);
			REQUIRE(e == 5);
			REQUIRE_NOTHROW([&]() {
				lua["g"](pf());
			}());
		}
		double f = sol::stack::pop<double>(lua);
		REQUIRE(f == 256.78);
	}
}

TEST_CASE("functions/overloaded variadic", "make sure variadics work to some degree with overloading") {
	sol::state lua;
	lua.open_libraries();

	sol::table ssl = lua.create_named_table("ssl");
	ssl.set_function("test", sol::overload(&va_func<int>, &va_func<double>));

	lua.safe_script("a = ssl.test(1, 2, 3)");
	lua.safe_script("b = ssl.test(1, 2)");
	lua.safe_script("c = ssl.test(2.2)");

	int a = lua["a"];
	int b = lua["b"];
	double c = lua["c"];
	REQUIRE(a == 6);
	REQUIRE(b == 3);
	REQUIRE(c == 2.2);
}

TEST_CASE("functions/sectioning variadic", "make sure variadics can bite off chunks of data") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", [](sol::variadic_args va) {
		int r = 0;
		sol::variadic_args shifted_va(va.lua_state(), 3);
		for (auto v : shifted_va) {
			int value = v;
			r += value;
		}
		return r;
	});

	lua.safe_script("x = f(1, 2, 3, 4)");
	lua.safe_script("x2 = f(8, 200, 3, 4)");
	lua.safe_script("x3 = f(1, 2, 3, 4, 5, 6)");

	lua.safe_script("print(x) assert(x == 7)");
	lua.safe_script("print(x2) assert(x2 == 7)");
	lua.safe_script("print(x3) assert(x3 == 18)");
}

TEST_CASE("functions/set_function already wrapped", "setting a function returned from Lua code that is already wrapped into a sol::function or similar") {
	SECTION("test different types") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		sol::function fn = lua.safe_script("return function() return 5 end");
		sol::protected_function pfn = fn;
		std::function<int()> sfn = fn;

		lua.set_function("test", fn);
		lua.set_function("test2", pfn);
		lua.set_function("test3", sfn);

		{
			auto result = lua.safe_script("assert(type(test) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() ~= nil)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() == 5)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}

		{
			auto result = lua.safe_script("assert(type(test2) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test2() ~= nil)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test2() == 5)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}

		{
			auto result = lua.safe_script("assert(type(test3) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test3() ~= nil)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test3() == 5)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}

	SECTION("getting the value from C++") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		sol::function fn = lua.safe_script("return function() return 5 end");

		int result = fn();
		REQUIRE(result == 5);
	}

	SECTION("setting the function directly") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		sol::function fn = lua.safe_script("return function() return 5 end");

		lua.set_function("test", fn);

		{
			auto result = lua.safe_script("assert(type(test) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() ~= nil)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() == 5)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}

	SECTION("does the function actually get executed?") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		sol::function fn2 = lua.safe_script("return function() print('this was executed') end");
		lua.set_function("test", fn2);

		{
			auto result = lua.safe_script("assert(type(test) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("test()", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}

	SECTION("setting the function indirectly, with the return value cast explicitly") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		sol::function fn = lua.safe_script("return function() return 5 end");

		lua.set_function("test", [&fn]() { return fn.call<int>(); });

		{
			auto result = lua.safe_script("assert(type(test) == 'function')", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() ~= nil)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("assert(test() == 5)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}
}

TEST_CASE("functions/pointer nullptr + nil", "ensure specific semantics for handling pointer-nils passed through sol") {
	struct nil_test {

		static void f(nil_test* p) {
			REQUIRE(p == nullptr);
		}
		static void g(std::unique_ptr<nil_test>& p) {
			REQUIRE(p == nullptr);
		}
		static void h(std::shared_ptr<nil_test>& p) {
			REQUIRE(p == nullptr);
		}
	};

	std::shared_ptr<nil_test> sptr;
	std::unique_ptr<nil_test> uptr;
	std::unique_ptr<nil_test> ruptr;
	nil_test* rptr = ruptr.get();
	nil_test* vptr = nullptr;

	SECTION("ptr") {
		sol::state lua;
		lua["v1"] = sptr;
		lua["v2"] = std::unique_ptr<nil_test>();
		lua["v3"] = rptr;
		lua["v4"] = vptr;

		REQUIRE_NOTHROW([&]() {
			nil_test* v1 = lua["v1"];
			nil_test* v2 = lua["v2"];
			nil_test* v3 = lua["v3"];
			nil_test* v4 = lua["v4"];
			REQUIRE(v1 == sptr.get());
			REQUIRE(v1 == nullptr);
			REQUIRE(v2 == uptr.get());
			REQUIRE(v2 == nullptr);
			REQUIRE(v3 == rptr);
			REQUIRE(v3 == nullptr);
			REQUIRE(v4 == vptr);
			REQUIRE(v4 == nullptr);
		}());
	}
	SECTION("ptr") {
		sol::state lua;
		lua.open_libraries();

		lua["v1"] = sptr;
		lua["v2"] = std::unique_ptr<nil_test>();
		lua["v3"] = rptr;
		lua["v4"] = vptr;
		lua["f"] = &nil_test::f;
		lua["g"] = &nil_test::g;
		lua["h"] = &nil_test::h;

		REQUIRE_NOTHROW([&]() {
			lua.safe_script("f(v1)");
			lua.safe_script("f(v2)");
			lua.safe_script("f(v3)");
			lua.safe_script("f(v4)");

			lua.safe_script("assert(v1 == nil)");
			lua.safe_script("assert(v2 == nil)");
			lua.safe_script("assert(v3 == nil)");
			lua.safe_script("assert(v4 == nil)");
		}());
	}
	SECTION("throw unique argument") {
		sol::state lua;
		lua["v2"] = std::unique_ptr<nil_test>();
		lua["g"] = &nil_test::g;

		auto result = lua.safe_script("g(v2)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	SECTION("throw shared argument") {
		sol::state lua;
		lua["v1"] = sptr;
		lua["h"] = &nil_test::h;

		auto result = lua.safe_script("h(v1)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	SECTION("throw ref") {
		{
			sol::state lua;
			lua["v1"] = sptr;
			sol::object o = lua["v1"];
			bool isp = o.is<nil_test&>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v2"] = std::unique_ptr<nil_test>();
			sol::object o = lua["v2"];
			bool isp = o.is<nil_test&>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v3"] = rptr;
			sol::object o = lua["v3"];
			bool isp = o.is<nil_test&>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v4"] = vptr;
			sol::object o = lua["v4"];
			bool isp = o.is<nil_test&>();
			REQUIRE_FALSE(isp);
		}
	}
	SECTION("throw unique") {
		{
			sol::state lua;
			lua["v1"] = sptr;
			sol::object o = lua["v1"];
			bool isp = o.is<std::unique_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v2"] = std::unique_ptr<nil_test>();
			sol::object o = lua["v2"];
			bool isp = o.is<std::unique_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v3"] = rptr;
			sol::object o = lua["v3"];
			bool isp = o.is<std::unique_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		};
		{
			sol::state lua;
			lua["v4"] = vptr;
			sol::object o = lua["v4"];
			bool isp = o.is<std::unique_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		};
	}
	SECTION("throw shared") {
		{
			sol::state lua;
			lua["v1"] = sptr;
			sol::object o = lua["v1"];
			bool isp = o.is<std::shared_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v2"] = std::unique_ptr<nil_test>();
			sol::object o = lua["v2"];
			bool isp = o.is<std::shared_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v3"] = rptr;
			sol::object o = lua["v3"];
			bool isp = o.is<std::shared_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
		{
			sol::state lua;
			lua["v4"] = vptr;
			sol::object o = lua["v4"];
			bool isp = o.is<std::shared_ptr<nil_test>>();
			REQUIRE_FALSE(isp);
		}
	}
}

TEST_CASE("functions/unique_usertype overloading", "make sure overloading can work with ptr vs. specifically asking for a unique_usertype") {
	struct test {
		int special_value = 17;
		test()
		: special_value(17) {
		}
		test(int special_value)
		: special_value(special_value) {
		}
	};
	auto print_up_test = [](std::unique_ptr<test>& x) {
		REQUIRE(x->special_value == 21);
	};
	auto print_up_2_test = [](int, std::unique_ptr<test>& x) {
		REQUIRE(x->special_value == 21);
	};
	auto print_sp_test = [](std::shared_ptr<test>& x) {
		REQUIRE(x->special_value == 44);
	};
	auto print_ptr_test = [](test* x) {
		REQUIRE(x->special_value == 17);
	};
	auto print_ref_test = [](test& x) {
		bool is_any = x.special_value == 17
			|| x.special_value == 21
			|| x.special_value == 44;
		REQUIRE(is_any);
	};
	using f_t = void(test&);
	f_t* fptr = print_ref_test;

	std::unique_ptr<test> ut = std::make_unique<test>(17);
	SECTION("working") {
		sol::state lua;

		lua.set_function("f", print_up_test);
		lua.set_function("g", sol::overload(std::move(print_sp_test), print_up_test, std::ref(print_ptr_test)));
		lua.set_function("h", std::ref(fptr));
		lua.set_function("i", std::move(print_up_2_test));

		lua["v1"] = std::make_unique<test>(21);
		lua["v2"] = std::make_shared<test>(44);
		lua["v3"] = test(17);
		lua["v4"] = ut.get();

		{
			auto result = lua.safe_script("f(v1)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("g(v1)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("g(v2)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("g(v3)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("g(v4)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("h(v1)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("h(v2)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("h(v3)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("h(v4)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
		{
			auto result = lua.safe_script("i(20, v1)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	};
	// LuaJIT segfaults hard on some Linux machines
	// and it breaks all the tests...
	SECTION("throws-value") {
		sol::state lua;

		lua.set_function("f", print_up_test);
		lua["v3"] = test(17);

		auto result = lua.safe_script("f(v3)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	};
	SECTION("throws-shared_ptr") {
		sol::state lua;

		lua.set_function("f", print_up_test);
		lua["v2"] = std::make_shared<test>(44);

		auto result = lua.safe_script("f(v2)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	};
	SECTION("throws-ptr") {
		sol::state lua;

		lua.set_function("f", print_up_test);
		lua["v4"] = ut.get();

		auto result = lua.safe_script("f(v4)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	};
}

TEST_CASE("functions/lua style default arguments", "allow default arguments using sol::reference and sol::object") {
	auto def_f1 = [](sol::object defaulted) -> int {
		bool inactive = defaulted == sol::lua_nil; // inactive by default
		if (inactive) {
			return 20;
		}
		return 10;
	};
	auto def_f2 = [](sol::reference defaulted) -> int {
		bool inactive = defaulted == sol::lua_nil; // inactive by default
		if (inactive) {
			return 20;
		}
		return 10;
	};
	auto def_f3 = [](sol::stack_reference defaulted) -> int {
		bool inactive = defaulted == sol::lua_nil; // inactive by default
		if (inactive) {
			return 20;
		}
		return 10;
	};

	sol::state lua;
	lua.set_function("f1", def_f1);
	lua.set_function("f2", def_f2);
	lua.set_function("f3", def_f3);

	auto result = lua.safe_script(R"(
		v1d, v1nd = f1(), f1(1)
		v2d, v2nd = f2(), f2(1)
		v3d, v3nd = f3(), f3(1)
	)", sol::script_pass_on_error);
	REQUIRE(result.valid());
	int v1d = lua["v1d"];
	int v1nd = lua["v1nd"];
	int v2d = lua["v2d"];
	int v2nd = lua["v2nd"];
	int v3d = lua["v3d"];
	int v3nd = lua["v3nd"];
	REQUIRE(20 == v1d);
	REQUIRE(20 == v2d);
	REQUIRE(20 == v3d);
	REQUIRE(10 == v1nd);
	REQUIRE(10 == v2nd);
	REQUIRE(10 == v3nd);
}

#if !defined(_MSC_VER) || !(defined(_WIN32) && !defined(_WIN64))

TEST_CASE("functions/noexcept", "allow noexcept functions to be serialized properly into Lua using sol2") {
	struct T {
		static int noexcept_function() noexcept {
			return 0x61;
		}

		int noexcept_method() noexcept {
			return 0x62;
		}
	} t;

	lua_CFunction ccall = sol::c_call<decltype(&raw_noexcept_function), &raw_noexcept_function>;

	sol::state lua;

	lua.set_function("f", &T::noexcept_function);
	lua.set_function("g", &T::noexcept_method);
	lua.set_function("h", &T::noexcept_method, T());
	lua.set_function("i", &T::noexcept_method, std::ref(t));
	lua.set_function("j", &T::noexcept_method, &t);
	lua.set_function("k", &T::noexcept_method, t);
	lua.set_function("l", &raw_noexcept_function);
	lua.set_function("m", ccall);

	lua["t"] = T();
	lua.safe_script("v1 = f()");
	lua.safe_script("v2 = g(t)");
	lua.safe_script("v3 = h()");
	lua.safe_script("v4 = i()");
	lua.safe_script("v5 = j()");
	lua.safe_script("v6 = k()");
	lua.safe_script("v7 = l()");
	lua.safe_script("v8 = m()");
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	int v4 = lua["v4"];
	int v5 = lua["v5"];
	int v6 = lua["v6"];
	int v7 = lua["v7"];
	int v8 = lua["v8"];
	REQUIRE(v1 == 0x61);
	REQUIRE(v2 == 0x62);
	REQUIRE(v3 == 0x62);
	REQUIRE(v4 == 0x62);
	REQUIRE(v5 == 0x62);
	REQUIRE(v6 == 0x62);
	REQUIRE(v7 == 0x63);
	REQUIRE(v8 == 0x63);
}

#endif // Strange VC++ stuff
