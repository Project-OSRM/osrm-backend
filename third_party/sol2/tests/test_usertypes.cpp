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
#include <list>
#include <memory>

struct non_copyable {
	non_copyable() = default;
	non_copyable(const non_copyable&) = delete;
	non_copyable& operator=(const non_copyable&) = delete;
	non_copyable(non_copyable&&) = default;
	non_copyable& operator=(non_copyable&&) = default;
};

struct vars {
	vars() {
	}

	int boop = 0;

	~vars() {
	}
};

struct fuser {
	int x;
	fuser()
	: x(0) {
	}

	fuser(int x)
	: x(x) {
	}

	int add(int y) {
		return x + y;
	}

	int add2(int y) {
		return x + y + 2;
	}
};

namespace crapola {
	struct fuser {
		int x;
		fuser()
		: x(0) {
		}
		fuser(int x)
		: x(x) {
		}
		fuser(int x, int x2)
		: x(x * x2) {
		}

		int add(int y) {
			return x + y;
		}
		int add2(int y) {
			return x + y + 2;
		}
	};
} // namespace crapola

class Base {
public:
	Base(int a_num)
	: m_num(a_num) {
	}

	int get_num() {
		return m_num;
	}

protected:
	int m_num;
};

class Derived : public Base {
public:
	Derived(int a_num)
	: Base(a_num) {
	}

	int get_num_10() {
		return 10 * m_num;
	}
};

class abstract_A {
public:
	virtual void a() = 0;
};

class abstract_B : public abstract_A {
public:
	virtual void a() override {
		INFO("overridden a() in B : public A - BARK");
	}
};

struct Vec {
	float x, y, z;
	Vec(float x, float y, float z)
	: x{ x }, y{ y }, z{ z } {
	}
	float length() {
		return sqrtf(x * x + y * y + z * z);
	}
	Vec normalized() {
		float invS = 1 / length();
		return { x * invS, y * invS, z * invS };
	}
};

struct giver {
	int a = 0;

	giver() {
	}

	void gief() {
		a = 1;
	}

	static void stuff() {
	}

	static void gief_stuff(giver& t, int a) {
		t.a = a;
	}

	~giver() {
	}
};

struct factory_test {
private:
	factory_test() {
		a = true_a;
	}
	~factory_test() {
		a = 0;
	}

public:
	static int num_saved;
	static int num_killed;

	struct deleter {
		void operator()(factory_test* f) {
			f->~factory_test();
		}
	};

	static const int true_a;
	int a;

	static std::unique_ptr<factory_test, deleter> make() {
		return std::unique_ptr<factory_test, deleter>(new factory_test(), deleter());
	}

	static void save(factory_test& f) {
		new (&f) factory_test();
		++num_saved;
	}

	static void kill(factory_test& f) {
		f.~factory_test();
		++num_killed;
	}
};

int factory_test::num_saved = 0;
int factory_test::num_killed = 0;
const int factory_test::true_a = 156;

bool something() {
	return true;
}

struct thing {
	int v = 100;

	thing() {
	}
	thing(int x)
	: v(x) {
	}
};

struct self_test {
	int bark;

	self_test()
	: bark(100) {
	}

	void g(const std::string& str) {
		std::cout << str << '\n';
		bark += 1;
	}

	void f(const self_test& t) {
		std::cout << "got test" << '\n';
		if (t.bark != bark)
			throw sol::error("bark values are not the same for self_test f function");
		if (&t != this)
			throw sol::error("call does not reference self for self_test f function");
	}
};

struct ext_getset {

	int bark = 24;
	const int meow = 56;

	ext_getset() = default;
	ext_getset(int v)
	: bark(v) {
	}
	ext_getset(ext_getset&&) = default;
	ext_getset(const ext_getset&) = delete;
	ext_getset& operator=(ext_getset&&) = default;
	ext_getset& operator=(const ext_getset&) = delete;
	~ext_getset() {
	}

	std::string x() {
		return "bark bark bark";
	}

	int x2(std::string x) {
		return static_cast<int>(x.length());
	}

	void set(sol::variadic_args, sol::this_state, int x) {
		bark = x;
	}

	int get(sol::this_state, sol::variadic_args) {
		return bark;
	}

	static void s_set(int) {
	}

	static int s_get(int x) {
		return x + 20;
	}
};

template <typename T>
void des(T& e) {
	e.~T();
}

struct matrix_xf {
	float a, b;

	static matrix_xf from_lua_table(sol::table t) {
		matrix_xf m;
		m.a = t[1][1];
		m.b = t[1][2];
		return m;
	}
};

struct matrix_xi {
	int a, b;

	static matrix_xi from_lua_table(sol::table t) {
		matrix_xi m;
		m.a = t[1][1];
		m.b = t[1][2];
		return m;
	}
};
template <typename SelfType>
struct alignas(16) weird_aligned_wrapper {
	template <typename F>
	weird_aligned_wrapper(F&& f)
	: lambda(std::forward<F>(f)) {
	}
	void operator()(SelfType& self, sol::object param) const {
		lambda(self, param.as<float>());
	}
	std::function<void(SelfType&, float)> lambda;
};

TEST_CASE("usertype/usertype", "Show that we can create classes from usertype and use them") {
	sol::state lua;

	sol::usertype<fuser> lc{ "add", &fuser::add, "add2", &fuser::add2 };
	lua.set_usertype(lc);

	lua.safe_script(
		"a = fuser:new()\n"
		"b = a:add(1)\n"
		"c = a:add2(1)\n");

	sol::object a = lua.get<sol::object>("a");
	sol::object b = lua.get<sol::object>("b");
	sol::object c = lua.get<sol::object>("c");
	REQUIRE((a.is<sol::userdata_value>()));
	auto atype = a.get_type();
	auto btype = b.get_type();
	auto ctype = c.get_type();
	REQUIRE((atype == sol::type::userdata));
	REQUIRE((btype == sol::type::number));
	REQUIRE((ctype == sol::type::number));
	int bresult = b.as<int>();
	int cresult = c.as<int>();
	REQUIRE(bresult == 1);
	REQUIRE(cresult == 3);
}

TEST_CASE("usertype/usertype-constructors", "Show that we can create classes from usertype and use them with multiple constructors") {

	sol::state lua;

	sol::constructors<sol::types<>, sol::types<int>, sol::types<int, int>> con;
	sol::usertype<crapola::fuser> lc(con, "add", &crapola::fuser::add, "add2", &crapola::fuser::add2);
	lua.set_usertype(lc);

	lua.safe_script(
		"a = fuser.new(2)\n"
		"u = a:add(1)\n"
		"v = a:add2(1)\n"
		"b = fuser:new()\n"
		"w = b:add(1)\n"
		"x = b:add2(1)\n"
		"c = fuser.new(2, 3)\n"
		"y = c:add(1)\n"
		"z = c:add2(1)\n");
	sol::object a = lua.get<sol::object>("a");
	auto atype = a.get_type();
	REQUIRE((atype == sol::type::userdata));
	sol::object u = lua.get<sol::object>("u");
	sol::object v = lua.get<sol::object>("v");
	REQUIRE((u.as<int>() == 3));
	REQUIRE((v.as<int>() == 5));

	sol::object b = lua.get<sol::object>("b");
	auto btype = b.get_type();
	REQUIRE((btype == sol::type::userdata));
	sol::object w = lua.get<sol::object>("w");
	sol::object x = lua.get<sol::object>("x");
	REQUIRE((w.as<int>() == 1));
	REQUIRE((x.as<int>() == 3));

	sol::object c = lua.get<sol::object>("c");
	auto ctype = c.get_type();
	REQUIRE((ctype == sol::type::userdata));
	sol::object y = lua.get<sol::object>("y");
	sol::object z = lua.get<sol::object>("z");
	REQUIRE((y.as<int>() == 7));
	REQUIRE((z.as<int>() == 9));
}

TEST_CASE("usertype/usertype-utility", "Show internal management of classes registered through new_usertype") {
	sol::state lua;

	lua.new_usertype<fuser>("fuser", "add", &fuser::add, "add2", &fuser::add2);

	lua.safe_script(
		"a = fuser.new()\n"
		"b = a:add(1)\n"
		"c = a:add2(1)\n");

	sol::object a = lua.get<sol::object>("a");
	sol::object b = lua.get<sol::object>("b");
	sol::object c = lua.get<sol::object>("c");
	REQUIRE((a.is<sol::userdata_value>()));
	auto atype = a.get_type();
	auto btype = b.get_type();
	auto ctype = c.get_type();
	REQUIRE((atype == sol::type::userdata));
	REQUIRE((btype == sol::type::number));
	REQUIRE((ctype == sol::type::number));
	int bresult = b.as<int>();
	int cresult = c.as<int>();
	REQUIRE(bresult == 1);
	REQUIRE(cresult == 3);
}

TEST_CASE("usertype/usertype-utility-derived", "usertype classes must play nice when a derived class does not overload a publically visible base function") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	sol::constructors<sol::types<int>> basector;
	sol::usertype<Base> baseusertype(basector, "get_num", &Base::get_num);

	lua.set_usertype(baseusertype);

	lua.safe_script("base = Base.new(5)");
	{
		auto result = lua.safe_script("print(base:get_num())", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	sol::constructors<sol::types<int>> derivedctor;
	sol::usertype<Derived> derivedusertype(derivedctor,
		"get_num_10", &Derived::get_num_10,
		"get_num", &Derived::get_num);

	lua.set_usertype(derivedusertype);

	lua.safe_script("derived = Derived.new(7)");
	lua.safe_script(
		"dgn = derived:get_num()\n"
		"print(dgn)");
	lua.safe_script(
		"dgn10 = derived:get_num_10()\n"
		"print(dgn10)");

	REQUIRE((lua.get<int>("dgn10") == 70));
	REQUIRE((lua.get<int>("dgn") == 7));
}

TEST_CASE("usertype/self-referential usertype", "usertype classes must play nice when C++ object types are requested for C++ code") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<self_test>("test", "g", &self_test::g, "f", &self_test::f);

	auto result = lua.safe_script(
		"local a = test.new()\n"
		"a:g(\"woof\")\n"
		"a:f(a)\n",
		sol::script_pass_on_error);
	REQUIRE(result.valid());
}

TEST_CASE("usertype/issue-number-twenty-five", "Using pointers and references from C++ classes in Lua") {
	struct test {
		int x = 0;
		test& set() {
			x = 10;
			return *this;
		}

		int get() {
			return x;
		}

		test* pget() {
			return this;
		}

		test create_get() {
			return *this;
		}

		int fun(int xa) {
			return xa * 10;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<test>("test", "set", &test::set, "get", &test::get, "pointer_get", &test::pget, "fun", &test::fun, "create_get", &test::create_get);
	{
		auto result = lua.safe_script("x = test.new()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(x:set():get() == 10)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("y = x:pointer_get()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("y:set():get()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("y:fun(10)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("x:fun(10)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(y:fun(10) == x:fun(10), '...')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(y:fun(10) == 100, '...')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(y:set():get() == y:set():get(), '...')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("assert(y:set():get() == 10, '...')", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
}

TEST_CASE("usertype/issue-number-thirty-five", "using value types created from lua-called C++ code, fixing user-defined types with constructors") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	sol::constructors<sol::types<float, float, float>> ctor;
	sol::usertype<Vec> udata(ctor, "normalized", &Vec::normalized, "length", &Vec::length);
	lua.set_usertype(udata);

	{
		auto result = lua.safe_script(
			"v = Vec.new(1, 2, 3)\n"
			"print(v:length())");
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script(
			"v = Vec.new(1, 2, 3)\n"
			"print(v:normalized():length())");
		REQUIRE(result.valid());
	}
}

TEST_CASE("usertype/lua-stored-usertype", "ensure usertype values can be stored without keeping usertype object alive") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	{
		sol::constructors<sol::types<float, float, float>> ctor;
		sol::usertype<Vec> udata(ctor,
			"normalized", &Vec::normalized,
			"length", &Vec::length);

		lua.set_usertype(udata);
		// usertype dies, but still usable in lua!
	}

	{
		auto result = lua.safe_script(
			"collectgarbage()\n"
			"v = Vec.new(1, 2, 3)\n"
			"print(v:length())");
		REQUIRE(result.valid());
	}

	{
		auto result = lua.safe_script(
			"v = Vec.new(1, 2, 3)\n"
			"print(v:normalized():length())");
		REQUIRE(result.valid());
	}
}

TEST_CASE("usertype/member-variables", "allow table-like accessors to behave as member variables for usertype") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	sol::constructors<sol::types<float, float, float>> ctor;
	sol::usertype<Vec> udata(ctor,
		"x", &Vec::x,
		"y", &Vec::y,
		"z", &Vec::z,
		"normalized", &Vec::normalized,
		"length", &Vec::length);
	lua.set_usertype(udata);

	REQUIRE_NOTHROW(lua.safe_script(
		"v = Vec.new(1, 2, 3)\n"
		"v2 = Vec.new(0, 1, 0)\n"
		"print(v:length())\n"));
	REQUIRE_NOTHROW(lua.safe_script(
		"v.x = 2\n"
		"v2.y = 2\n"
		"print(v.x, v.y, v.z)\n"
		"print(v2.x, v2.y, v2.z)\n"));
	REQUIRE_NOTHROW(lua.safe_script(
		"assert(v.x == 2)\n"
		"assert(v2.x == 0)\n"
		"assert(v2.y == 2)\n"));
	REQUIRE_NOTHROW(lua.safe_script(
		"v.x = 3\n"
		"local x = v.x\n"
		"assert(x == 3)\n"));

	struct breaks {
		sol::function f;
	};

	lua.open_libraries(sol::lib::base);
	lua.set("b", breaks());
	lua.new_usertype<breaks>("breaks",
		"f", &breaks::f);

	breaks& b = lua["b"];
	{
		auto result = lua.safe_script("b.f = function () print('BARK!') end", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("b.f()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	REQUIRE_NOTHROW(b.f());
}

TEST_CASE("usertype/nonmember-functions", "let users set non-member functions that take unqualified T as first parameter to usertype") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<giver>("giver",
		   "gief_stuff", giver::gief_stuff,
		   "gief", &giver::gief,
		   "__tostring", [](const giver& t) {
			   return std::to_string(t.a) + ": giving value";
		   })
		.get<sol::table>("giver")
		.set_function("stuff", giver::stuff);

	{
		auto result = lua.safe_script("giver.stuff()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script(
			"t = giver.new()\n"
			"print(tostring(t))\n"
			"t:gief()\n"
			"t:gief_stuff(20)\n");
		REQUIRE(result.valid());
	}
	giver& g = lua.get<giver>("t");
	REQUIRE(g.a == 20);
	std::cout << "----- end of 1" << std::endl;
}

TEST_CASE("usertype/unique-shared-ptr", "manage the conversion and use of unique and shared pointers ('unique usertypes')") {
	const int64_t unique_value = 0x7125679355635963;
	auto uniqueint = std::make_unique<int64_t>(unique_value);
	auto sharedint = std::make_shared<int64_t>(unique_value);
	long preusecount = sharedint.use_count();
	{
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		lua.set("uniqueint", std::move(uniqueint));
		lua.set("sharedint", sharedint);
		std::unique_ptr<int64_t>& uniqueintref = lua["uniqueint"];
		std::shared_ptr<int64_t>& sharedintref = lua["sharedint"];
		int64_t* rawuniqueintref = lua["uniqueint"];
		int64_t* rawsharedintref = lua["sharedint"];
		int siusecount = sharedintref.use_count();
		REQUIRE((uniqueintref.get() == rawuniqueintref && sharedintref.get() == rawsharedintref));
		REQUIRE((uniqueintref != nullptr && sharedintref != nullptr && rawuniqueintref != nullptr && rawsharedintref != nullptr));
		REQUIRE((unique_value == *uniqueintref.get() && unique_value == *sharedintref.get()));
		REQUIRE((unique_value == *rawuniqueintref && unique_value == *rawsharedintref));
		REQUIRE(siusecount == sharedint.use_count());
		std::shared_ptr<int64_t> moreref = sharedint;
		REQUIRE(unique_value == *moreref.get());
		REQUIRE(moreref.use_count() == sharedint.use_count());
		REQUIRE(moreref.use_count() == sharedintref.use_count());
	}
	REQUIRE(preusecount == sharedint.use_count());
	std::cout << "----- end of 2" << std::endl;
}

TEST_CASE("regressions/one", "issue number 48") {
	sol::state lua;
	lua.new_usertype<vars>("vars",
		"boop", &vars::boop);
	auto code =
		"beep = vars.new()\n"
		"beep.boop = 1";
	auto result1 = lua.safe_script(code, sol::script_pass_on_error);
	REQUIRE(result1.valid());
	// test for segfault
	auto my_var = lua.get<vars>("beep");
	auto& my_var_ref = lua.get<vars>("beep");
	auto* my_var_ptr = lua.get<vars*>("beep");
	REQUIRE(my_var.boop == 1);
	REQUIRE(my_var_ref.boop == 1);
	REQUIRE(my_var_ptr->boop == 1);
	REQUIRE(std::addressof(my_var_ref) == my_var_ptr);
	std::cout << "----- end of 3" << std::endl;
}

TEST_CASE("usertype/get-set-references", "properly get and set with std::ref semantics. Note that to get, we must not use Unqualified<T> on the type...") {
	std::cout << "----- in 4" << std::endl;
	sol::state lua;

	lua.new_usertype<vars>("vars",
		"boop", &vars::boop);
	vars var{};
	vars rvar{};
	std::cout << "setting beep" << std::endl;
	lua.set("beep", var);
	std::cout << "setting rbeep" << std::endl;
	lua.set("rbeep", std::ref(rvar));
	std::cout << "getting my_var" << std::endl;
	auto& my_var = lua.get<vars>("beep");
	std::cout << "setting rbeep" << std::endl;
	auto& ref_var = lua.get<std::reference_wrapper<vars>>("rbeep");
	vars& proxy_my_var = lua["beep"];
	std::reference_wrapper<vars> proxy_ref_var = lua["rbeep"];
	var.boop = 2;
	rvar.boop = 5;

	// Was return as a value: var must be diferent from "beep"
	REQUIRE_FALSE(std::addressof(var) == std::addressof(my_var));
	REQUIRE_FALSE(std::addressof(proxy_my_var) == std::addressof(var));
	REQUIRE((my_var.boop == 0));
	REQUIRE(var.boop != my_var.boop);

	REQUIRE(std::addressof(ref_var) == std::addressof(rvar));
	REQUIRE(std::addressof(proxy_ref_var.get()) == std::addressof(rvar));
	REQUIRE(rvar.boop == 5);
	REQUIRE(rvar.boop == ref_var.boop);
	std::cout << "----- end of 4" << std::endl;
}

TEST_CASE("usertype/private-constructible", "Check to make sure special snowflake types from Enterprise thingamahjongs work properly.") {
	int numsaved = factory_test::num_saved;
	int numkilled = factory_test::num_killed;
	{
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<factory_test>("factory_test",
			"new", sol::initializers(factory_test::save),
			"__gc", sol::destructor(factory_test::kill),
			"a", &factory_test::a);

		std::unique_ptr<factory_test, factory_test::deleter> f = factory_test::make();
		lua.set("true_a", factory_test::true_a, "f", f.get());
		{
			auto result = lua.safe_script("assert(f.a == true_a)", sol::script_pass_on_error);
			REQUIRE(result.valid());
		}

		auto code1 =
			"local fresh_f = factory_test:new()\n"
			"assert(fresh_f.a == true_a)\n";
		auto result1 = lua.safe_script(code1, sol::script_pass_on_error);
		REQUIRE(result1.valid());
	}
	int expectednumsaved = numsaved + 1;
	int expectednumkilled = numkilled + 1;
	REQUIRE(expectednumsaved == factory_test::num_saved);
	REQUIRE(expectednumkilled == factory_test::num_killed);
	std::cout << "----- end of 5" << std::endl;
}

TEST_CASE("usertype/const-pointer", "Make sure const pointers can be taken") {
	struct A_x {
		int x = 201;
	};
	struct B_foo {
		int foo(const A_x* a) {
			return a->x;
		};
	};

	sol::state lua;
	lua.new_usertype<B_foo>("B",
		"foo", &B_foo::foo);
	lua.set("a", A_x());
	lua.set("b", B_foo());
	lua.safe_script("x = b:foo(a)");
	int x = lua["x"];
	REQUIRE(x == 201);
	std::cout << "----- end of 6" << std::endl;
}

TEST_CASE("usertype/overloading", "Check if overloading works properly for usertypes") {
	struct woof {
		int var;

		int func(int x) {
			return var + x;
		}

		double func2(int x, int y) {
			return var + x + y + 0.5;
		}

		std::string func2s(int x, std::string y) {
			return y + " " + std::to_string(x);
		}
	};
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<woof>("woof",
		"var", &woof::var,
		"func", sol::overload(&woof::func, &woof::func2, &woof::func2s));

	const std::string bark_58 = "bark 58";

	REQUIRE_NOTHROW(lua.safe_script(
		"r = woof:new()\n"
		"a = r:func(1)\n"
		"b = r:func(1, 2)\n"
		"c = r:func(58, 'bark')\n"));
	REQUIRE((lua["a"] == 1));
	REQUIRE((lua["b"] == 3.5));
	REQUIRE((lua["c"] == bark_58));
	auto result = lua.safe_script("r:func(1,2,'meow')", sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
	std::cout << "----- end of 7" << std::endl;
}

TEST_CASE("usertype/overloading_values", "ensure overloads handle properly") {
	struct overloading_test {
		int print(int i) {
			INFO("Integer print: " << i);
			return 500 + i;
		}
		int print() {
			INFO("No param print.");
			return 500;
		}
	};

	sol::state lua;
	lua.new_usertype<overloading_test>("overloading_test", sol::constructors<>(),
		"print", sol::overload(static_cast<int (overloading_test::*)(int)>(&overloading_test::print), static_cast<int (overloading_test::*)()>(&overloading_test::print)),
		"print2", sol::overload(static_cast<int (overloading_test::*)()>(&overloading_test::print), static_cast<int (overloading_test::*)(int)>(&overloading_test::print)));
	lua.set("test", overloading_test());

	sol::function f0_0 = lua.load("return test:print()");
	sol::function f0_1 = lua.load("return test:print2()");
	sol::function f1_0 = lua.load("return test:print(24)");
	sol::function f1_1 = lua.load("return test:print2(24)");
	int res = f0_0();
	int res2 = f0_1();
	int res3 = f1_0();
	int res4 = f1_1();

	REQUIRE(res == 500);
	REQUIRE(res2 == 500);

	REQUIRE(res3 == 524);
	REQUIRE(res4 == 524);
	std::cout << "----- end of 8" << std::endl;
}

TEST_CASE("usertype/reference-and-constness", "Make sure constness compiles properly and errors out at runtime") {
	struct bark {
		int var = 50;
	};
	struct woof {
		bark b;
	};

	struct nested {
		const int f = 25;
	};

	struct outer {
		nested n;
	};

	sol::state lua;
	lua.new_usertype<woof>("woof",
		"b", &woof::b);
	lua.new_usertype<bark>("bark",
		"var", &bark::var);
	lua.new_usertype<outer>("outer",
		"n", &outer::n);
	lua.set("w", woof());
	lua.set("n", nested());
	lua.set("o", outer());
	lua.set("f", sol::c_call<decltype(&nested::f), &nested::f>);
	lua.safe_script(R"(
    x = w.b
    x.var = 20
    val = w.b.var == x.var
    v = f(n);
    )");

	woof& w = lua["w"];
	bark& x = lua["x"];
	nested& n = lua["n"];
	int v = lua["v"];
	bool val = lua["val"];
	// enforce reference semantics
	REQUIRE(std::addressof(w.b) == std::addressof(x));
	REQUIRE(n.f == 25);
	REQUIRE(v == 25);
	REQUIRE(val);

	{
		auto result = lua.safe_script("f(n, 50)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("o.n = 25", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("usertype/readonly-and-static-functions", "Check if static functions can be called on userdata and from their originating (meta)tables") {
	struct bark {
		int var = 50;

		void func() {
		}

		static void oh_boy() {
		}

		static int oh_boy(std::string name) {
			return static_cast<int>(name.length());
		}

		int operator()(int x) {
			return x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<bark>("bark",
		"var", &bark::var,
		"var2", sol::readonly(&bark::var),
		"something", something,
		"something2", [](int x, int y) { return x + y; },
		"func", &bark::func,
		"oh_boy", sol::overload(sol::resolve<void()>(&bark::oh_boy), sol::resolve<int(std::string)>(&bark::oh_boy)),
		sol::meta_function::call_function, &bark::operator());

	{
		auto result = lua.safe_script("assert(bark.oh_boy('woo') == 3)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	{
		auto result = lua.safe_script("bark.oh_boy()", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}

	bark b;
	lua.set("b", &b);

	sol::table b_table = lua["b"];
	sol::function member_func = b_table["func"];
	sol::function s = b_table["something"];
	sol::function s2 = b_table["something2"];

	sol::table b_metatable = b_table[sol::metatable_key];
	bool isvalidmt = b_metatable.valid();
	REQUIRE(isvalidmt);
	sol::function b_call = b_metatable["__call"];
	sol::function b_as_function = lua["b"];

	int x = b_as_function(1);
	int y = b_call(b, 1);
	bool z = s();
	int w = s2(2, 3);
	REQUIRE(x == 1);
	REQUIRE(y == 1);
	REQUIRE(z);
	REQUIRE(w == 5);

	lua.safe_script(R"(
lx = b(1)
ly = getmetatable(b).__call(b, 1)
lz = b.something()
lz2 = bark.something()
lw = b.something2(2, 3)
lw2 = bark.something2(2, 3)
    )");

	int lx = lua["lx"];
	int ly = lua["ly"];
	bool lz = lua["lz"];
	int lw = lua["lw"];
	bool lz2 = lua["lz2"];
	int lw2 = lua["lw2"];
	REQUIRE(lx == 1);
	REQUIRE(ly == 1);
	REQUIRE(lz);
	REQUIRE(lz2);
	REQUIRE(lw == 5);
	REQUIRE(lw2 == 5);
	REQUIRE(lx == ly);
	REQUIRE(lz == lz2);
	REQUIRE(lw == lw2);

	auto result = lua.safe_script("b.var2 = 2", sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
}

TEST_CASE("usertype/properties", "Check if member properties/variables work") {
	struct bark {
		int var = 50;
		int var2 = 25;

		int get_var2() const {
			return var2;
		}

		int get_var3() {
			return var2;
		}

		void set_var2(int x) {
			var2 = x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<bark>("bark",
		"var", &bark::var,
		"var2", sol::readonly(&bark::var2),
		"a", sol::property(&bark::get_var2, &bark::set_var2),
		"b", sol::property(&bark::get_var2),
		"c", sol::property(&bark::get_var3),
		"d", sol::property(&bark::set_var2));

	bark b;
	lua.set("b", &b);

	lua.safe_script("b.a = 59");
	lua.safe_script("var2_0 = b.a");
	lua.safe_script("var2_1 = b.b");
	lua.safe_script("b.d = 1568");
	lua.safe_script("var2_2 = b.c");

	int var2_0 = lua["var2_0"];
	int var2_1 = lua["var2_1"];
	int var2_2 = lua["var2_2"];
	REQUIRE(var2_0 == 59);
	REQUIRE(var2_1 == 59);
	REQUIRE(var2_2 == 1568);

	{
		auto result = lua.safe_script("b.var2 = 24", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("r = b.d", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("r = b.d", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("b.b = 25", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	{
		auto result = lua.safe_script("b.c = 11", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("usertype/safety", "crash with an exception -- not a segfault -- on bad userdata calls") {
	class Test {
	public:
		void sayHello() {
			std::cout << "Hey\n";
		}
	};

	sol::state lua;
	lua.new_usertype<Test>("Test", "sayHello", &Test::sayHello);
	static const std::string code = R"(
        local t = Test.new()
        t:sayHello() --Works fine
        t.sayHello() --Uh oh.
    )";
	auto result = lua.safe_script(code, sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
}

TEST_CASE("usertype/call_constructor", "make sure lua types can be constructed with function call constructors") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<thing>("thing",
		"v", &thing::v, sol::call_constructor, sol::constructors<sol::types<>, sol::types<int>>());

	lua.safe_script(R"(
t = thing(256)
)");

	thing& y = lua["t"];
	INFO(y.v);
	REQUIRE(y.v == 256);
}

TEST_CASE("usertype/call_constructor-factories", "make sure tables can be passed to factory-based call constructors") {
	sol::state lua;
	lua.open_libraries();

	lua.new_usertype<matrix_xf>("mat",
		sol::call_constructor, sol::factories(&matrix_xf::from_lua_table));

	lua.safe_script("m = mat{ {1.1, 2.2} }");

	lua.new_usertype<matrix_xi>("mati",
		sol::call_constructor, sol::factories(&matrix_xi::from_lua_table));

	lua.safe_script("mi = mati{ {1, 2} }");

	matrix_xf& m = lua["m"];
	REQUIRE(m.a == 1.1f);
	REQUIRE(m.b == 2.2f);
	matrix_xi& mi = lua["mi"];
	REQUIRE(mi.a == 1);
	REQUIRE(mi.b == 2);
}

TEST_CASE("usertype/call_constructor_2", "prevent metatable regression") {
	class class01 {
	public:
		int x = 57;
		class01() {
		}
	};

	class class02 {
	public:
		int x = 50;
		class02() {
		}
		class02(const class01& other)
		: x(other.x) {
		}
	};

	sol::state lua;

	lua.new_usertype<class01>("class01",
		sol::call_constructor, sol::constructors<sol::types<>, sol::types<const class01&>>());

	lua.new_usertype<class02>("class02",
		sol::call_constructor, sol::constructors<sol::types<>, sol::types<const class02&>, sol::types<const class01&>>());

	REQUIRE_NOTHROW(lua.safe_script(R"(
x = class01()
y = class02(x)
)"));
	class02& y = lua["y"];
	REQUIRE(y.x == 57);
}

TEST_CASE("usertype/blank_constructor", "make sure lua types cannot be constructed with arguments if a blank / empty constructor is provided") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<thing>("thing",
		"v", &thing::v, sol::call_constructor, sol::constructors<>());

	auto result = lua.safe_script("t = thing(256)", sol::script_pass_on_error);
	REQUIRE_FALSE(result.valid());
}

TEST_CASE("usertype/no_constructor", "make sure lua types cannot be constructed if a blank / empty constructor is provided") {

	SECTION("order1") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		lua.new_usertype<thing>("thing",
			"v", &thing::v,
			sol::call_constructor, sol::no_constructor);
		auto result = lua.safe_script("t = thing()", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}

	SECTION("order2") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<thing>("thing",
			sol::call_constructor, sol::no_constructor,
			"v", &thing::v);
		auto result = lua.safe_script("t = thing.new()", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}

	SECTION("new no_constructor") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<thing>("thing",
			sol::meta_function::construct, sol::no_constructor);
		auto result = lua.safe_script("t = thing.new()", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}

	SECTION("call no_constructor") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<thing>("thing",
			sol::call_constructor, sol::no_constructor);
		auto result = lua.safe_script("t = thing()", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
}

TEST_CASE("usertype/coverage", "try all the things") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<ext_getset>("ext_getset",
		sol::call_constructor, sol::constructors<sol::types<>, sol::types<int>>(),
		sol::meta_function::garbage_collect, sol::destructor(des<ext_getset>),
		"x", sol::overload(&ext_getset::x, &ext_getset::x2, [](ext_getset& m, std::string x, int y) {
			return m.meow + 50 + y + x.length();
		}),
		"bark", &ext_getset::bark, "meow", &ext_getset::meow, "readonlybark", sol::readonly(&ext_getset::bark), "set", &ext_getset::set, "get", &ext_getset::get, "sset", &ext_getset::s_set, "sget", &ext_getset::s_get, "propbark", sol::property(&ext_getset::set, &ext_getset::get), "readonlypropbark", sol::property(&ext_getset::get), "writeonlypropbark", sol::property(&ext_getset::set));

	INFO("usertype created");

	lua.safe_script(R"(
e = ext_getset()
w = e:x(e:x(), e:x(e:x()))
print(w)
)");

	int w = lua["w"];
	REQUIRE(w == (56 + 50 + 14 + 14));

	INFO("REQUIRE(w) successful");

	lua.safe_script(R"(
e:set(500)
e.sset(24)
x = e:get()
y = e.sget(20)
)");

	int x = lua["x"];
	int y = lua["y"];
	REQUIRE(x == 500);
	REQUIRE(y == 40);

	INFO("REQUIRE(x, y) successful");

	lua.safe_script(R"(
e.bark = 5001
a = e:get()
print(e.bark)
print(a)

e.propbark = 9700
b = e:get()
print(e.propbark)
print(b)
)");
	int a = lua["a"];
	int b = lua["b"];

	REQUIRE(a == 5001);
	REQUIRE(b == 9700);

	INFO("REQUIRE(a, b) successful");

	lua.safe_script(R"(
c = e.readonlybark
d = e.meow
print(e.readonlybark)
print(c)
print(e.meow)
print(d)
)");

	int c = lua["c"];
	int d = lua["d"];
	REQUIRE(c == 9700);
	REQUIRE(d == 56);

	INFO("REQUIRE(c, d) successful");

	lua.safe_script(R"(
e.writeonlypropbark = 500
z = e.readonlypropbark
print(e.readonlybark)
print(e.bark)
)");

	int z = lua["z"];
	REQUIRE(z == 500);

	INFO("REQUIRE(z) successful");
	{
		auto result = lua.safe_script("e.readonlybark = 24", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
		INFO("REQUIRE_FALSE 1 successful");
	}
	{
		auto result = lua.safe_script("e.readonlypropbark = 500", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
		INFO("REQUIRE_FALSE 2 successful");
	}
	{
		auto result = lua.safe_script("y = e.writeonlypropbark", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
		INFO("REQUIRE_FALSE 3 successful");
	}
}

TEST_CASE("usertype/copyability", "make sure user can write to a class variable even if the class itself isn't copy-safe") {
	struct NoCopy {
		int get() const {
			return _you_can_copy_me;
		}
		void set(int val) {
			_you_can_copy_me = val;
		}

		int _you_can_copy_me;
		non_copyable _haha_you_cant_copy_me;
	};

	sol::state lua;
	lua.new_usertype<NoCopy>("NoCopy", "val", sol::property(&NoCopy::get, &NoCopy::set));

	REQUIRE_NOTHROW(
		lua.safe_script(R"__(
nocopy = NoCopy.new()
nocopy.val = 5
               )__"));
}

TEST_CASE("usertype/protect", "users should be allowed to manually protect a function") {
	struct protect_me {
		int gen(int x) {
			return x;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<protect_me>("protect_me",
		"gen", sol::protect(&protect_me::gen));

	REQUIRE_NOTHROW(
		lua.safe_script(R"__(
pm = protect_me.new()
value = pcall(pm.gen,pm)
)__"));
	bool value = lua["value"];
	REQUIRE_FALSE(value);
}

TEST_CASE("usertype/vars", "usertype vars can bind various class items") {
	static int muh_variable = 25;
	static int through_variable = 10;

	sol::state lua;
	lua.open_libraries();
	struct test {};
	lua.new_usertype<test>("test",
		"straight", sol::var(2),
		"global", sol::var(muh_variable),
		"ref_global", sol::var(std::ref(muh_variable)),
		"global2", sol::var(through_variable),
		"ref_global2", sol::var(std::ref(through_variable)));

	int prets = lua["test"]["straight"];
	int pretg = lua["test"]["global"];
	int pretrg = lua["test"]["ref_global"];
	int pretg2 = lua["test"]["global2"];
	int pretrg2 = lua["test"]["ref_global2"];

	REQUIRE(prets == 2);
	REQUIRE(pretg == 25);
	REQUIRE(pretrg == 25);
	REQUIRE(pretg2 == 10);
	REQUIRE(pretrg2 == 10);

	lua.safe_script(R"(
print(test.straight)
test.straight = 50
print(test.straight)
)");
	int s = lua["test"]["straight"];
	REQUIRE(s == 50);

	lua.safe_script(R"(
t = test.new()
print(t.global)
t.global = 50
print(t.global)
)");
	int mv = lua["test"]["global"];
	REQUIRE(mv == 50);
	REQUIRE(muh_variable == 25);

	lua.safe_script(R"(
print(t.ref_global)
t.ref_global = 50
print(t.ref_global)
)");
	int rmv = lua["test"]["ref_global"];
	REQUIRE(rmv == 50);
	REQUIRE(muh_variable == 50);

	REQUIRE(through_variable == 10);
	lua.safe_script(R"(
print(test.global2)
test.global2 = 35
print(test.global2)
)");
	int tv = lua["test"]["global2"];
	REQUIRE(through_variable == 10);
	REQUIRE(tv == 35);

	lua.safe_script(R"(
print(test.ref_global2)
test.ref_global2 = 35
print(test.ref_global2)
)");
	int rtv = lua["test"]["ref_global2"];
	REQUIRE(rtv == 35);
	REQUIRE(through_variable == 35);
}

TEST_CASE("usertype/static-properties", "allow for static functions to get and set things as a property") {
	static int b = 50;
	struct test_t {
		static double s_func() {
			return b + 0.5;
		}

		static void g_func(int v) {
			b = v;
		}

		std::size_t func() {
			return 24;
		}
	};
	test_t manager;

	sol::state lua;

	lua.new_usertype<test_t>("test",
		"f", std::function<std::size_t()>(std::bind(std::mem_fn(&test_t::func), &manager)),
		"g", sol::property(&test_t::s_func, &test_t::g_func));

	lua.safe_script("v1 = test.f()");
	lua.safe_script("v2 = test.g");
	lua.safe_script("test.g = 60");
	lua.safe_script("v2a = test.g");
	int v1 = lua["v1"];
	REQUIRE(v1 == 24);
	double v2 = lua["v2"];
	REQUIRE(v2 == 50.5);
	double v2a = lua["v2a"];
	REQUIRE(v2a == 60.5);
}

TEST_CASE("usertype/var-and-property", "make sure const vars are readonly and properties can handle lambdas") {
	const static int arf = 20;

	struct test {
		int value = 10;
	};

	sol::state lua;
	lua.open_libraries();

	lua.new_usertype<test>("test",
		"prop", sol::property([](test& t) { return t.value; }, [](test& t, int x) { t.value = x; }),
		"global", sol::var(std::ref(arf)));

	lua.safe_script(R"(
t = test.new()
print(t.prop)
t.prop = 50
print(t.prop)
	)");

	test& t = lua["t"];
	REQUIRE(t.value == 50);

	lua.safe_script(R"(
t = test.new()
print(t.global)
	)");
	{
		auto result = lua.safe_script("t.global = 20", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
	lua.safe_script("print(t.global)");
}

TEST_CASE("usertype/unique_usertype-check", "make sure unique usertypes don't get pushed as references with function calls and the like") {
	class Entity {
	public:
		std::string GetName() {
			return "Charmander";
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::io);

	lua.new_usertype<Entity>("Entity",
		"new", sol::no_constructor,
		"get_name", &Entity::GetName);

	lua.safe_script(R"(
		function my_func(entity)
		print("INSIDE LUA")
		print(entity:get_name())
		end
)");

	sol::function my_func = lua["my_func"];
	REQUIRE_NOTHROW([&] {
		auto ent = std::make_shared<Entity>();
		my_func(ent);
		Entity ent2;
		my_func(ent2);
		my_func(std::make_shared<Entity>());
	}());
}

TEST_CASE("usertype/abstract-base-class", "Ensure that abstract base classes and such can be registered") {
	sol::state lua;
	lua.new_usertype<abstract_A>("A", "a", &abstract_A::a);
	lua.new_usertype<abstract_B>("B", sol::base_classes, sol::bases<abstract_A>());
	REQUIRE_NOTHROW([&]() {
		lua.safe_script(R"(
local b = B.new()
b:a()
		)");
	});
}

TEST_CASE("usertype/as_function", "Ensure that variables can be turned into functions by as_function") {
	class B {
	public:
		int bvar = 24;
	};

	sol::state lua;
	lua.open_libraries();
	lua.new_usertype<B>("B", "b", &B::bvar, "f", sol::as_function(&B::bvar));

	B b;
	lua.set("b", &b);
	lua.safe_script("x = b:f()");
	lua.safe_script("y = b.b");
	int x = lua["x"];
	int y = lua["y"];
	REQUIRE(x == 24);
	REQUIRE(y == 24);
}

TEST_CASE("usertype/call-initializers", "Ensure call constructors with initializers work well") {
	struct A {
		double f = 25.5;

		static void init(A& x, double f) {
			x.f = f;
		}
	};

	sol::state lua;
	lua.open_libraries();

	lua.new_usertype<A>("A",
		sol::call_constructor, sol::initializers(&A::init));

	lua.safe_script(R"(
a = A(24.3)
)");
	A& a = lua["a"];
	REQUIRE(a.f == 24.3);
}

TEST_CASE("usertype/missing-key", "make sure a missing key returns nil") {
	struct thing {};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<thing>("thing");
	{
		auto result = lua.safe_script("v = thing.missingKey\nprint(v)", sol::script_pass_on_error);
		REQUIRE(result.valid());
	}
	sol::object o = lua["v"];
	bool isnil = o.is<sol::lua_nil_t>();
	REQUIRE(isnil);
}

TEST_CASE("usertype/runtime-extensibility", "Check if usertypes are runtime extensible") {
	struct thing {
		int v = 20;
		int func(int a) {
			return a;
		}
	};
	int val = 0;

	class base_a {
	public:
		int x;
	};

	class derived_b : public base_a {
	};

	SECTION("just functions") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<thing>("thing",
			"func", &thing::func);

		lua.safe_script(R"(
t = thing.new()
		)");

		{
			auto result = lua.safe_script(R"(
t.runtime_func = function (a)
	return a + 50
end
		)",
				sol::script_pass_on_error);
			REQUIRE_FALSE(result.valid());
		};

		{
			auto result = lua.safe_script(R"(
function t:runtime_func(a)
	return a + 52
end
		)",
				sol::script_pass_on_error);
			REQUIRE_FALSE(result.valid());
		};

		lua.safe_script("val = t:func(2)");
		val = lua["val"];
		REQUIRE(val == 2);

		REQUIRE_NOTHROW([&lua]() {
			lua.safe_script(R"(
function thing:runtime_func(a)
	return a + 1
end
		)");
		}());

		lua.safe_script("val = t:runtime_func(2)");
		val = lua["val"];
		REQUIRE(val == 3);
	}
	SECTION("with variable") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<thing>("thing",
			"func", &thing::func,
			"v", &thing::v);

		lua.safe_script(R"(
t = thing.new()
		)");

		{
			auto result = lua.safe_script(R"(
t.runtime_func = function (a)
	return a + 50
end
		)",
				sol::script_pass_on_error);
			REQUIRE_FALSE(result.valid());
		};

		{
			auto result = lua.safe_script(R"(
function t:runtime_func(a)
	return a + 52
end
		)",
				sol::script_pass_on_error);
			REQUIRE_FALSE(result.valid());
		};

		lua.safe_script("val = t:func(2)");
		val = lua["val"];
		REQUIRE(val == 2);

		REQUIRE_NOTHROW([&lua]() {
			lua.safe_script(R"(
function thing:runtime_func(a)
	return a + 1
end
		)");
		}());

		lua.safe_script("val = t:runtime_func(2)");
		val = lua["val"];
		REQUIRE(val == 3);
	}
	SECTION("with bases") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<base_a>("A",
			"x", &base_a::x //no crash without this
		);

		lua.new_usertype<derived_b>("B",
			sol::base_classes, sol::bases<base_a>());

		auto pfr0 = lua.safe_script("function A:c() print('A') return 1 end", sol::script_pass_on_error);
		REQUIRE(pfr0.valid());
		auto pfr1 = lua.safe_script("function B:c() print('B') return 2 end", sol::script_pass_on_error);
		REQUIRE(pfr1.valid());
		auto pfr2 = lua.safe_script("obja = A.new() objb = B.new()", sol::script_default_on_error);
		REQUIRE(pfr2.valid());
		auto pfr3 = lua.safe_script("assert(obja:c() == 1)", sol::script_default_on_error);
		REQUIRE(pfr3.valid());
		auto pfr4 = lua.safe_script("assert(objb:c() == 2)", sol::script_default_on_error);
		REQUIRE(pfr4.valid());
	}
}

TEST_CASE("usertype/runtime-replacement", "ensure that functions can be properly replaced at runtime for non-indexed things") {
	struct heart_base_t {};
	struct heart_t : heart_base_t {
		void func() {
		}
	};

	SECTION("plain") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<heart_t>("a");
		REQUIRE_NOTHROW([&lua]() {
			lua.safe_script("obj = a.new()");
			lua.safe_script("function a:heartbeat () print('arf') return 1 end");
			lua.safe_script("v1 = obj:heartbeat()");
			lua.safe_script("function a:heartbeat () print('bark') return 2 end");
			lua.safe_script("v2 = obj:heartbeat()");
			lua.safe_script("a.heartbeat = function(self) print('woof') return 3 end");
			lua.safe_script("v3 = obj:heartbeat()");
		}());
		int v1 = lua["v1"];
		int v2 = lua["v2"];
		int v3 = lua["v3"];
		REQUIRE(v1 == 1);
		REQUIRE(v2 == 2);
		REQUIRE(v3 == 3);
	}
	SECTION("variables") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<heart_t>("a",
			sol::base_classes, sol::bases<heart_base_t>());

		REQUIRE_NOTHROW([&lua]() {
			lua.safe_script("obj = a.new()");
			lua.safe_script("function a:heartbeat () print('arf') return 1 end");
			lua.safe_script("v1 = obj:heartbeat()");
			lua.safe_script("function a:heartbeat () print('bark') return 2 end");
			lua.safe_script("v2 = obj:heartbeat()");
			lua.safe_script("a.heartbeat = function(self) print('woof') return 3 end");
			lua.safe_script("v3 = obj:heartbeat()");
		}());
		int v1 = lua["v1"];
		int v2 = lua["v2"];
		int v3 = lua["v3"];
		REQUIRE(v1 == 1);
		REQUIRE(v2 == 2);
		REQUIRE(v3 == 3);
	}
	SECTION("methods") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		lua.new_usertype<heart_t>("a",
			"func", &heart_t::func,
			sol::base_classes, sol::bases<heart_base_t>());

		REQUIRE_NOTHROW([&lua]() {
			lua.safe_script("obj = a.new()");
			lua.safe_script("function a:heartbeat () print('arf') return 1 end");
			lua.safe_script("v1 = obj:heartbeat()");
			lua.safe_script("function a:heartbeat () print('bark') return 2 end");
			lua.safe_script("v2 = obj:heartbeat()");
			lua.safe_script("a.heartbeat = function(self) print('woof') return 3 end");
			lua.safe_script("v3 = obj:heartbeat()");
		}());
		int v1 = lua["v1"];
		int v2 = lua["v2"];
		int v3 = lua["v3"];
		REQUIRE(v1 == 1);
		REQUIRE(v2 == 2);
		REQUIRE(v3 == 3);
	}
}

TEST_CASE("usertype/runtime additions with newindex", "ensure that additions when new_index is overriden don't hit the specified new_index function") {
	class newindex_object {};
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.new_usertype<newindex_object>("object",
		sol::meta_function::new_index, [](newindex_object& o, sol::object key, sol::object value) {
			return;
		});

	lua["object"]["test"] = [](newindex_object& o) {
		std::cout << "test" << std::endl;
		return 446;
	};

	auto result1 = lua.safe_script("o = object.new()", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("assert(o:test() == 446)", sol::script_pass_on_error);
	REQUIRE(result2.valid());
}

TEST_CASE("usertype/alignment", "ensure that alignment does not trigger weird aliasing issues") {
	struct aligned_base {};
	struct aligned_derived : aligned_base {};

	sol::state lua;
	auto f = [](aligned_base&, float d) {
		REQUIRE(d == 5.0f);
	};
	lua.new_usertype<aligned_base>("Base",
		"x", sol::writeonly_property(weird_aligned_wrapper<aligned_base>(std::ref(f))));
	lua.new_usertype<aligned_derived>("Derived",
		sol::base_classes, sol::bases<aligned_base>());

	aligned_derived d;
	lua["d"] = d;

	auto result = lua.safe_script("d.x = 5");
	REQUIRE(result.valid());
}

TEST_CASE("usertype/meta-key-retrievals", "allow for special meta keys (__index, __newindex) to trigger methods even if overwritten directly") {
	SECTION("dynamically") {
		static int writes = 0;
		static std::string keys[4] = {};
		static int values[4] = {};
		struct d_sample {
			void foo(std::string k, int v) {
				keys[writes] = k;
				values[writes] = v;
				++writes;
			}
		};

		sol::state lua;
		lua.new_usertype<d_sample>("sample");
		sol::table s = lua["sample"]["new"]();
		s[sol::metatable_key][sol::meta_function::new_index] = &d_sample::foo;
		lua["var"] = s;

		lua.safe_script("var = sample.new()");
		lua.safe_script("var.key = 2");
		lua.safe_script("var.__newindex = 4");
		lua.safe_script("var.__index = 3");
		lua.safe_script("var.__call = 1");
		REQUIRE(values[0] == 2);
		REQUIRE(values[1] == 4);
		REQUIRE(values[2] == 3);
		REQUIRE(values[3] == 1);
		REQUIRE(keys[0] == "key");
		REQUIRE(keys[1] == "__newindex");
		REQUIRE(keys[2] == "__index");
		REQUIRE(keys[3] == "__call");
	}

	SECTION("statically") {
		static int writes = 0;
		static std::string keys[4] = {};
		static int values[4] = {};
		struct sample {
			void foo(std::string k, int v) {
				keys[writes] = k;
				values[writes] = v;
				++writes;
			}
		};

		sol::state lua;
		lua.new_usertype<sample>("sample", sol::meta_function::new_index, &sample::foo);

		lua.safe_script("var = sample.new()");
		lua.safe_script("var.key = 2");
		lua.safe_script("var.__newindex = 4");
		lua.safe_script("var.__index = 3");
		lua.safe_script("var.__call = 1");
		REQUIRE(values[0] == 2);
		REQUIRE(values[1] == 4);
		REQUIRE(values[2] == 3);
		REQUIRE(values[3] == 1);
		REQUIRE(keys[0] == "key");
		REQUIRE(keys[1] == "__newindex");
		REQUIRE(keys[2] == "__index");
		REQUIRE(keys[3] == "__call");
	}
}

TEST_CASE("usertype/basic type information", "check that we can query some basic type information") {
	struct my_thing {};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<my_thing>("my_thing");

	lua.safe_script("obj = my_thing.new()");

	lua.safe_script("assert(my_thing.__type.is(obj))");
	lua.safe_script("assert(not my_thing.__type.is(1))");
	lua.safe_script("assert(not my_thing.__type.is(\"not a thing\"))");
	lua.safe_script("print(my_thing.__type.name)");

	lua.safe_script("assert(obj.__type.is(obj))");
	lua.safe_script("assert(not obj.__type.is(1))");
	lua.safe_script("assert(not obj.__type.is(\"not a thing\"))");
	lua.safe_script("print(obj.__type.name)");

	lua.safe_script("assert(getmetatable(my_thing).__type.is(obj))");
	lua.safe_script("assert(not getmetatable(my_thing).__type.is(1))");
	lua.safe_script("assert(not getmetatable(my_thing).__type.is(\"not a thing\"))");
	lua.safe_script("print(getmetatable(my_thing).__type.name)");

	lua.safe_script("assert(getmetatable(obj).__type.is(obj))");
	lua.safe_script("assert(not getmetatable(obj).__type.is(1))");
	lua.safe_script("assert(not getmetatable(obj).__type.is(\"not a thing\"))");
	lua.safe_script("print(getmetatable(obj).__type.name)");
}

#if !defined(_MSC_VER) || !(defined(_WIN32) && !defined(_WIN64))

TEST_CASE("usertype/noexcept-methods", "make sure noexcept functions and methods can be bound to usertypes without issues") {
	struct T {
		static int noexcept_function() noexcept {
			return 0x61;
		}

		int noexcept_method() noexcept {
			return 0x62;
		}
	};

	sol::state lua;
	lua.new_usertype<T>("T",
		"nf", &T::noexcept_function,
		"nm", &T::noexcept_method);

	lua.safe_script("t = T.new()");
	lua.safe_script("v1 = t.nf()");
	lua.safe_script("v2 = t:nm()");
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	REQUIRE(v1 == 0x61);
	REQUIRE(v2 == 0x62);
}

#endif // VC++ or my path/compiler settings doing strange bullshit (but it happens on Appveyor too???)
