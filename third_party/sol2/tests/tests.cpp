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

#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

bool func_opt_ret_bool(sol::optional<int> i) {
	if (i) {
		INFO(i.value());
	}
	else {
		INFO("optional isn't set");
	}
	return true;
}

TEST_CASE("table/traversal", "ensure that we can chain requests and tunnel down into a value if we desire") {

	sol::state lua;
	int begintop = 0, endtop = 0;

	sol::function scriptload = lua.load("t1 = {t2 = {t3 = 24}};");
	scriptload();
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		int traversex24 = lua.traverse_get<int>("t1", "t2", "t3");
		REQUIRE(traversex24 == 24);
	}
	REQUIRE(begintop == endtop);

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		int x24 = lua["t1"]["t2"]["t3"];
		REQUIRE(x24 == 24);
	}
	REQUIRE(begintop == endtop);

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		lua["t1"]["t2"]["t3"] = 64;
		int traversex64 = lua.traverse_get<int>("t1", "t2", "t3");
		REQUIRE(traversex64 == 64);
	}
	REQUIRE(begintop == endtop);

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		int x64 = lua["t1"]["t2"]["t3"];
		REQUIRE(x64 == 64);
	}
	REQUIRE(begintop == endtop);

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		lua.traverse_set("t1", "t2", "t3", 13);
		int traversex13 = lua.traverse_get<int>("t1", "t2", "t3");
		REQUIRE(traversex13 == 13);
	}
	REQUIRE(begintop == endtop);

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		int x13 = lua["t1"]["t2"]["t3"];
		REQUIRE(x13 == 13);
	}
	REQUIRE(begintop == endtop);
}

TEST_CASE("simple/set", "Check if the set works properly.") {
	sol::state lua;
	int begintop = 0, endtop = 0;
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		lua.set("a", 9);
	}
	REQUIRE(begintop == endtop);
	{
		auto result = lua.safe_script("if a ~= 9 then error('wrong value') end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		lua.set("d", "hello");
	}
	REQUIRE(begintop == endtop);
	{
		auto result = lua.safe_script("if d ~= 'hello' then error('expected \\'hello\\', got '.. tostring(d)) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		lua.set("e", std::string("hello"), "f", true);
	}
	REQUIRE(begintop == endtop);
	{
		auto result = lua.safe_script("if d ~= 'hello' then error('expected \\'hello\\', got '.. tostring(d)) end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("if f ~= true then error('wrong value') end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("simple/get", "Tests if the get function works properly.") {
	sol::state lua;
	int begintop = 0, endtop = 0;

	lua.safe_script("a = 9");
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		auto a = lua.get<int>("a");
		REQUIRE(a == 9.0);
	}
	REQUIRE(begintop == endtop);

	lua.safe_script("b = nil");
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		REQUIRE_NOTHROW(lua.get<sol::lua_nil_t>("b"));
	}
	REQUIRE(begintop == endtop);

	lua.safe_script("d = 'hello'");
	lua.safe_script("e = true");
	{
		test_stack_guard g(lua.lua_state(), begintop, endtop);
		std::string d;
		bool e;
		std::tie(d, e) = lua.get<std::string, bool>("d", "e");
		REQUIRE(d == "hello");
		REQUIRE(e == true);
	}
	REQUIRE(begintop == endtop);
}

TEST_CASE("simple/set and get global integer", "Tests if the get function works properly with global integers") {
	sol::state lua;
	lua[1] = 25.4;
	lua.safe_script("b = 1");
	double a = lua.get<double>(1);
	double b = lua.get<double>("b");
	REQUIRE(a == 25.4);
	REQUIRE(b == 1);
}

TEST_CASE("simple/get_or", "check if table.get_or works correctly") {
	sol::state lua;

	auto bob_table = lua.create_table("bob");
	bob_table.set("is_set", 42);

	int is_set = bob_table.get_or("is_set", 3);
	int is_not_set = bob_table.get_or("is_not_set", 22);

	REQUIRE(is_set == 42);
	REQUIRE(is_not_set == 22);

	lua["joe"] = 55.6;
	double bark = lua.get_or<double>("joe", 60);
	REQUIRE(bark == 55.6);
}

TEST_CASE("simple/proxy get_or", "check if proxy.get_or works correctly") {
	sol::state lua;

	auto bob_table = lua.create_table("bob");
	bob_table.set("is_set", 42);

	int is_set = bob_table["is_set"].get_or(3);
	int is_not_set = bob_table["is_not_set"].get_or(22);

	REQUIRE(is_set == 42);
	REQUIRE(is_not_set == 22);

	lua["joe"] = 55.6;
	double bark = lua["joe"].get_or<double>(60);
	REQUIRE(bark == 55.6);
}

TEST_CASE("simple/addition", "check if addition works and can be gotten through lua.get and lua.set") {
	sol::state lua;

	lua.set("b", 0.2);
	lua.safe_script("c = 9 + b");
	auto c = lua.get<double>("c");

	REQUIRE(c == 9.2);
}

TEST_CASE("simple/if", "check if if statements work through lua") {
	sol::state lua;

	std::string program = "if true then f = 0.1 else f = 'test' end";
	lua.safe_script(program);
	auto f = lua.get<double>("f");

	REQUIRE(f == 0.1);
	REQUIRE((f == lua["f"]));
}

TEST_CASE("negative/basic errors", "Check if error handling works correctly") {
	sol::state lua;

	auto result = lua.safe_script("nil[5]", sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
}

TEST_CASE("libraries", "Check if we can open libraries") {
	sol::state lua;
	REQUIRE_NOTHROW(lua.open_libraries(sol::lib::base, sol::lib::os));
}

TEST_CASE("libraries2", "Check if we can open ALL the libraries") {
	sol::state lua;
	REQUIRE_NOTHROW(lua.open_libraries(sol::lib::base,
		sol::lib::bit32,
		sol::lib::coroutine,
		sol::lib::debug,
		sol::lib::ffi,
		sol::lib::jit,
		sol::lib::math,
		sol::lib::os,
		sol::lib::package,
		sol::lib::string,
		sol::lib::table));
}

TEST_CASE("interop/null-to-nil-and-back", "nil should be the given type when a pointer from C++ is returned as nullptr, and nil should result in nullptr in connected C++ code") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("lol", []() -> int* {
		return nullptr;
	});
	lua.set_function("rofl", [](int* x) {
		INFO(x);
	});
	REQUIRE_NOTHROW(lua.safe_script(
		"x = lol()\n"
		"rofl(x)\n"
		"assert(x == nil)"));
}

TEST_CASE("object/conversions", "make sure all basic reference types can be made into objects") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	struct d {};

	lua.safe_script("function f () print('bark') end");
	lua["d"] = d{};
	lua["l"] = static_cast<void*>(nullptr);

	sol::table t = lua.create_table();
	sol::table t2(lua, sol::create);
	sol::thread th = sol::thread::create(lua);
	sol::function f = lua["f"];
	sol::protected_function pf = lua["f"];
	sol::userdata ud = lua["d"];
	sol::lightuserdata lud = lua["l"];
	sol::environment env(lua, sol::create);

	sol::object ot(t);
	sol::object ot2(t2);
	sol::object oteq = ot;
	sol::object oth(th);
	sol::object of(f);
	sol::object opf(pf);
	sol::object od(ud);
	sol::object ol(lud);
	sol::object oenv(env);

	auto oni = sol::make_object(lua, 50);
	auto ond = sol::make_object(lua, 50.0);

	std::string somestring = "look at this text isn't it nice";
	auto osl = sol::make_object(lua, "Bark bark bark");
	auto os = sol::make_object(lua, somestring);

	auto omn = sol::make_object(lua, sol::lua_nil);

	REQUIRE(ot.get_type() == sol::type::table);
	REQUIRE(ot2.get_type() == sol::type::table);
	REQUIRE(oteq.get_type() == sol::type::table);
	REQUIRE(oth.get_type() == sol::type::thread);
	REQUIRE(of.get_type() == sol::type::function);
	REQUIRE(opf.get_type() == sol::type::function);
	REQUIRE(od.get_type() == sol::type::userdata);
	REQUIRE(ol.get_type() == sol::type::lightuserdata);
	REQUIRE(oni.get_type() == sol::type::number);
	REQUIRE(ond.get_type() == sol::type::number);
	REQUIRE(osl.get_type() == sol::type::string);
	REQUIRE(os.get_type() == sol::type::string);
	REQUIRE(omn.get_type() == sol::type::lua_nil);
	REQUIRE(oenv.get_type() == sol::type::table);
}

TEST_CASE("object/main_* conversions", "make sure all basic reference types can be made into objects") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	struct d {};

	lua.safe_script("function f () print('bark') end");
	lua["d"] = d{};
	lua["l"] = static_cast<void*>(nullptr);

	sol::main_table t = lua.create_table();
	sol::main_table t2(lua, sol::create);
	sol::thread th = sol::thread::create(lua);
	sol::main_function f = lua["f"];
	sol::main_protected_function pf = lua["f"];
	sol::main_userdata ud = lua["d"];
	sol::main_lightuserdata lud = lua["l"];
	sol::main_environment env(lua, sol::create);

	sol::main_object ot(t);
	sol::main_object ot2(t2);
	sol::main_object oteq = ot;
	sol::main_object oth(th);
	sol::main_object of(f);
	sol::main_object opf(pf);
	sol::main_object od(ud);
	sol::main_object ol(lud);
	sol::main_object oenv(env);

	auto oni = sol::make_object(lua, 50);
	auto ond = sol::make_object(lua, 50.0);

	std::string somestring = "look at this text isn't it nice";
	auto osl = sol::make_object(lua, "Bark bark bark");
	auto os = sol::make_object(lua, somestring);

	auto omn = sol::make_object(lua, sol::lua_nil);

	REQUIRE(ot.get_type() == sol::type::table);
	REQUIRE(ot2.get_type() == sol::type::table);
	REQUIRE(oteq.get_type() == sol::type::table);
	REQUIRE(oth.get_type() == sol::type::thread);
	REQUIRE(of.get_type() == sol::type::function);
	REQUIRE(opf.get_type() == sol::type::function);
	REQUIRE(od.get_type() == sol::type::userdata);
	REQUIRE(ol.get_type() == sol::type::lightuserdata);
	REQUIRE(oni.get_type() == sol::type::number);
	REQUIRE(ond.get_type() == sol::type::number);
	REQUIRE(osl.get_type() == sol::type::string);
	REQUIRE(os.get_type() == sol::type::string);
	REQUIRE(omn.get_type() == sol::type::lua_nil);
	REQUIRE(oenv.get_type() == sol::type::table);
}

TEST_CASE("feature/indexing overrides", "make sure index functions can be overridden on types") {
	struct PropertySet {
		sol::object get_property_lua(const char* name, sol::this_state s) {
			auto& var = props[name];
			return sol::make_object(s, var);
		}

		void set_property_lua(const char* name, sol::stack_object object) {
			props[name] = object.as<std::string>();
		}

		std::unordered_map<std::string, std::string> props;
	};

	struct DynamicObject {
		PropertySet& get_dynamic_props() {
			return dynamic_props;
		}

		PropertySet dynamic_props;
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<PropertySet>("PropertySet", sol::meta_function::new_index, &PropertySet::set_property_lua, sol::meta_function::index, &PropertySet::get_property_lua);
	lua.new_usertype<DynamicObject>("DynamicObject", "props", sol::property(&DynamicObject::get_dynamic_props));

	lua.safe_script(R"__(
obj = DynamicObject:new()
obj.props.name = 'test name'
print('name = ' .. obj.props.name)
)__");

	std::string name = lua["obj"]["props"]["name"];
	REQUIRE(name == "test name");
}

TEST_CASE("features/indexing numbers", "make sure indexing functions can be override on usertypes") {
	class vector {
	public:
		double data[3];

		vector()
		: data{ 0, 0, 0 } {
		}

		double& operator[](int i) {
			return data[i];
		}

		static double my_index(vector& v, int i) {
			return v[i];
		}

		static void my_new_index(vector& v, int i, double x) {
			v[i] = x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<vector>("vector", sol::constructors<sol::types<>>(),
		sol::meta_function::index, &vector::my_index,
		sol::meta_function::new_index, &vector::my_new_index);
	lua.safe_script(
		"v = vector.new()\n"
		"print(v[1])\n"
		"v[2] = 3\n"
		"print(v[2])\n");

	vector& v = lua["v"];
	REQUIRE(v[0] == 0.0);
	REQUIRE(v[1] == 0.0);
	REQUIRE(v[2] == 3.0);
}

TEST_CASE("features/multiple inheritance", "Ensure that multiple inheritance works as advertised") {
	struct base1 {
		int a1 = 250;
	};

	struct base2 {
		int a2 = 500;
	};

	struct simple : base1 {
	};

	struct complex : base1, base2 {
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<base1>("base1",
		"a1", &base1::a1);
	lua.new_usertype<base2>("base2",
		"a2", &base2::a2);
	lua.new_usertype<simple>("simple",
		"a1", &simple::a1,
		sol::base_classes, sol::bases<base1>());
	lua.new_usertype<complex>("complex",
		"a1", &complex::a1,
		"a2", &complex::a2,
		sol::base_classes, sol::bases<base1, base2>());
	lua.safe_script(
		"c = complex.new()\n"
		"s = simple.new()\n"
		"b1 = base1.new()\n"
		"b2 = base1.new()\n");

	base1* sb1 = lua["s"];
	REQUIRE(sb1 != nullptr);
	REQUIRE(sb1->a1 == 250);

	base1* cb1 = lua["c"];
	base2* cb2 = lua["c"];

	REQUIRE(cb1 != nullptr);
	REQUIRE(cb2 != nullptr);
	REQUIRE(cb1->a1 == 250);
	REQUIRE(cb2->a2 == 500);
}

TEST_CASE("regressions/std::ref", "Ensure that std::reference_wrapper<> isn't considered as a function by using unwrap_unqualified_t trait") {
	struct base1 {
		int a1 = 250;
	};

	sol::state lua;
	base1 v;
	lua["vp"] = &v;
	lua["vr"] = std::ref(v);

	base1* vp = lua["vp"];
	base1& vr = lua["vr"];
	REQUIRE(vp != nullptr);
	REQUIRE(vp == &v);

	REQUIRE(vp->a1 == 250);
	REQUIRE(vr.a1 == 250);

	v.a1 = 568;

	REQUIRE(vp->a1 == 568);
	REQUIRE(vr.a1 == 568);
}

TEST_CASE("optional/left out args", "Make sure arguments can be left out of optional without tanking miserably") {

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	// sol::optional needs an argument no matter what?
	lua.set_function("func_opt_ret_bool", func_opt_ret_bool);
	REQUIRE_NOTHROW([&] {
		lua.safe_script(R"(
        func_opt_ret_bool(42)
        func_opt_ret_bool()
        print('ok')
        )");
	}());
}

TEST_CASE("optional/engaged versus unengaged", "solidify semantics for an engaged and unengaged optional") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", [](sol::optional<sol::object> optional_arg) {
		if (optional_arg) {
			return true;
		}
		return false;
	});

	auto valid0 = lua.safe_script("assert(not f())", sol::script_pass_on_error);
	REQUIRE(valid0.valid());
	auto valid1 = lua.safe_script("assert(not f(nil))", sol::script_pass_on_error);
	REQUIRE(valid1.valid());
	auto valid2 = lua.safe_script("assert(f(1))", sol::script_pass_on_error);
	REQUIRE(valid2.valid());
	auto valid3 = lua.safe_script("assert(f('hi'))", sol::script_pass_on_error);
	REQUIRE(valid3.valid());
}

TEST_CASE("pusher/constness", "Make sure more types can handle being const and junk") {
	struct Foo {
		Foo(const sol::function& f)
		: _f(f) {
		}
		const sol::function& _f;

		const sol::function& f() const {
			return _f;
		}
	};

	sol::state lua;

	lua.new_usertype<Foo>("Foo",
		sol::call_constructor, sol::no_constructor,
		"f", &Foo::f);

	lua["func"] = []() { return 20; };
	sol::function f = lua["func"];
	lua["foo"] = Foo(f);
	Foo& foo = lua["foo"];
	int x = foo.f()();
	REQUIRE(x == 20);
}

TEST_CASE("compilation/const regression", "make sure constness in tables is respected all the way down") {
	struct State {
	public:
		State() {
			this->state_.registry()["state"] = this;
		}

		sol::state state_;
	};

	State state;
	State* s = state.state_.registry()["state"];
	REQUIRE(s == &state);
}

TEST_CASE("numbers/integers", "make sure integers are detectable on most platforms") {
	sol::state lua;

	lua["a"] = 50;   // int
	lua["b"] = 50.5; // double

	sol::object a = lua["a"];
	sol::object b = lua["b"];

	bool a_is_int = a.is<int>();
	bool a_is_double = a.is<double>();

	bool b_is_int = b.is<int>();
	bool b_is_double = b.is<double>();

	REQUIRE(a_is_int);
	REQUIRE(a_is_double);

	// TODO: will this fail on certain lower Lua versions?
	REQUIRE_FALSE(b_is_int);
	REQUIRE(b_is_double);
}

TEST_CASE("object/is", "test whether or not the is abstraction works properly for a user-defined type") {
	struct thing {};

	SECTION("stack_object") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		lua.set_function("is_thing", [](sol::stack_object obj) { return obj.is<thing>(); });
		lua["a"] = thing{};
		{
			auto result = lua.safe_script("assert(is_thing(a))", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}

	SECTION("object") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		lua.set_function("is_thing", [](sol::object obj) { return obj.is<thing>(); });
		lua["a"] = thing{};
		{
			auto result = lua.safe_script("assert(is_thing(a))", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}
	}
}
