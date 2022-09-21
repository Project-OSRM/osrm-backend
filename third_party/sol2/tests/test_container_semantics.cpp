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
#include <deque>
#include <set>
#include <map>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <numeric> // std::iota

struct my_object {
private:
	std::vector<int> mdata;

public:
	static const void* last_printed;

	my_object(int sz)
	: mdata() {
		mdata.resize(sz);
		std::iota(mdata.begin(), mdata.end(), 1);
	}

	void operator()(std::size_t count, int value) {
		for (; count > 0; --count) {
			mdata.push_back(value);
		}
	}

public: // Container requirements, as per the C++ standard
	using value_type = int;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = decltype(mdata)::iterator;
	using const_iterator = decltype(mdata)::const_iterator;
	using difference_type = decltype(mdata)::difference_type;
	using size_type = decltype(mdata)::size_type;

	iterator begin() {
		return iterator(mdata.begin());
	}
	iterator end() {
		return iterator(mdata.end());
	}
	const_iterator begin() const {
		return const_iterator(mdata.begin());
	}
	const_iterator end() const {
		return const_iterator(mdata.end());
	}
	const_iterator cbegin() const {
		return begin();
	}
	const_iterator cend() const {
		return end();
	}
	size_type size() const noexcept {
		return mdata.size();
	}
	size_type max_size() const noexcept {
		return mdata.max_size();
	}
	void push_back(const value_type& v) {
		mdata.push_back(v);
	}
	void insert(const_iterator where, const value_type& v) {
		mdata.insert(where, v);
	}
	bool empty() const noexcept {
		return mdata.empty();
	}
	bool operator==(const my_object& right) const {
		return mdata == right.mdata;
	}
	bool operator!=(const my_object& right) const noexcept {
		return mdata != right.mdata;
	}

	std::vector<int>& data() {
		return mdata;
	}

	const std::vector<int>& data() const {
		return mdata;
	}
};

const void* my_object::last_printed = nullptr;

std::ostream& operator<<(std::ostream& ostr, const my_object& mo) {
	my_object::last_printed = static_cast<const void*>(&mo);
	ostr << "{ ";
	const auto& v = mo.data();
	if (v.empty()) {
		ostr << "empty";
	}
	else {
		ostr << v[0];
		for (std::size_t i = 1; i < v.size(); ++i) {
			ostr << ", " << v[i];
		}
	}
	ostr << " }";

	return ostr;
}

namespace sol {
	template <>
	struct is_container<my_object> : std::false_type {};
} // namespace sol

template <typename T>
void sequence_container_check(sol::state& lua, T& items) {
	{
		auto r1 = lua.safe_script(R"(
for i=1,#c do 
	v = c[i] 
	assert(v == (i + 10)) 
end
		)",
			sol::script_pass_on_error);
		REQUIRE(r1.valid());
	}
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(1)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(3)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(2, 20)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(6, 16)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(i1)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(i2)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17)", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[#c + 1] = 18", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[#c]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	auto backit = items.begin();
	std::size_t len = 0;
	{
		auto e = items.end();
		auto last = backit;
		for (; backit != e; ++backit, ++len) {
			if (backit == e) {
				break;
			}
			last = backit;
		}
		backit = last;
	}
	const int& first = *items.begin();
	const int& last = *backit;
	std::size_t i1 = lua["i1"];
	std::size_t i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	int values[6] = {
		20, 13, 14, 16, 17, 18
	};
	{
		std::size_t idx = 0;
		for (const auto& i : items) {
			const auto& v = values[idx];
			REQUIRE((i == v));
			++idx;
		}
	}
	REQUIRE((s1 == 6));
	REQUIRE((s2 == 5));
	REQUIRE((s3 == 4));
	REQUIRE((len == 6));
	REQUIRE((first == 20));
	REQUIRE((last == 18));
	REQUIRE((i1 == 1));
	REQUIRE((i2 == 4));
	REQUIRE((v1 == 11));
	REQUIRE((v2 == 13));
	REQUIRE((v3 == 18));
}

template <typename T>
void ordered_container_check(sol::state& lua, T& items) {
	{
		auto r1 = lua.safe_script(R"(
for i=1,#c do 
	v = c[(i + 10)] 
	assert(v == (i + 10)) 
end
		)",
			sol::script_pass_on_error);
		REQUIRE(r1.valid());
	}
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(13)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(20)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(16)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(i1)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(i2)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17)", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[18] = true", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[20]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	auto backit = items.begin();
	std::size_t len = 0;
	{
		auto e = items.end();
		auto last = backit;
		for (; backit != e; ++backit, ++len) {
			if (backit == e) {
				break;
			}
			last = backit;
		}
		backit = last;
	}
	const int& first = *items.begin();
	const int& last = *backit;
	int i1 = lua["i1"];
	int i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	int values[] = {
		12, 13, 15, 16, 17, 18, 20
	};
	{
		std::size_t idx = 0;
		for (const auto& i : items) {
			const auto& v = values[idx];
			REQUIRE((i == v));
			++idx;
		}
	}
	REQUIRE((s1 == 7));
	REQUIRE((s2 == 6));
	REQUIRE((s3 == 5));
	REQUIRE((len == 7));
	REQUIRE((first == 12));
	REQUIRE((last == 20));
	REQUIRE((i1 == 11));
	REQUIRE((i2 == 14));
	REQUIRE((v1 == 11));
	REQUIRE((v2 == 13));
	REQUIRE((v3 == 20));
}

template <typename T>
void unordered_container_check(sol::state& lua, T& items) {
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(13)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(20)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(16)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(i1)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(i2)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17)", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[18] = true", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[20]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	std::size_t len = items.size();
	int i1 = lua["i1"];
	int i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	int values[] = {
		12, 13, 15, 16, 17, 18, 20
	};
	{
		for (const auto& v : values) {
			auto it = items.find(v);
			REQUIRE((it != items.cend()));
			REQUIRE((*it == v));
		}
	}
	REQUIRE((s1 == 7));
	REQUIRE((s2 == 6));
	REQUIRE((s3 == 5));
	REQUIRE((len == 7));
	REQUIRE((i1 == 11));
	REQUIRE((i2 == 14));
	REQUIRE((v1 == 11));
	REQUIRE((v2 == 13));
	REQUIRE((v3 == 20));
}

template <typename T>
void associative_ordered_container_check(sol::state& lua, T& items) {
	{
		auto r1 = lua.safe_script(R"(
for i=1,#c do 
	v = c[(i + 10)] 
	assert(v == (i + 20))
end
		)",
			sol::script_pass_on_error);
		REQUIRE(r1.valid());
	}
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(13)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(20, 30)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(16, 26)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r3 = lua.safe_script("c:set(12, 31)", sol::script_pass_on_error);
			REQUIRE(r3.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17, 27)", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[18] = 28", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[20]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	auto backit = items.begin();
	std::size_t len = 0;
	{
		auto e = items.end();
		auto last = backit;
		for (; backit != e; ++backit, ++len) {
			if (backit == e) {
				break;
			}
			last = backit;
		}
		backit = last;
	}
	const std::pair<const short, int>& first = *items.begin();
	const std::pair<const short, int>& last = *backit;
	int i1 = lua["i1"];
	int i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	std::pair<const short, int> values[] = {
		{ (short)12, 31 },
		{ (short)13, 23 },
		{ (short)15, 25 },
		{ (short)16, 26 },
		{ (short)17, 27 },
		{ (short)18, 28 },
		{ (short)20, 30 }
	};
	{
		std::size_t idx = 0;
		for (const auto& i : items) {
			const auto& v = values[idx];
			REQUIRE((i == v));
			++idx;
		}
	}
	REQUIRE((s1 == 7));
	REQUIRE((s2 == 6));
	REQUIRE((s3 == 5));
	REQUIRE((len == 7));
	REQUIRE((first.first == 12));
	REQUIRE((last.first == 20));
	REQUIRE((first.second == 31));
	REQUIRE((last.second == 30));
	REQUIRE((i1 == 21));
	REQUIRE((i2 == 24));
	REQUIRE((v1 == 21));
	REQUIRE((v2 == 23));
	REQUIRE((v3 == 30));
}

template <typename T>
void associative_unordered_container_check(sol::state& lua, T& items) {
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(13)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(20, 30)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(16, 26)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r3 = lua.safe_script("c:set(12, 31)", sol::script_pass_on_error);
			REQUIRE(r3.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17, 27)", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[18] = 28", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[20]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	std::size_t len = items.size();
	int i1 = lua["i1"];
	int i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	std::pair<const short, int> values[] = {
		{ (short)12, 31 },
		{ (short)13, 23 },
		{ (short)15, 25 },
		{ (short)16, 26 },
		{ (short)17, 27 },
		{ (short)18, 28 },
		{ (short)20, 30 }
	};
	{
		for (const auto& v : values) {
			auto it = items.find(v.first);
			REQUIRE((it != items.cend()));
			REQUIRE((it->second == v.second));
		}
	}
	REQUIRE((s1 == 7));
	REQUIRE((s2 == 6));
	REQUIRE((s3 == 5));
	REQUIRE((len == 7));
	REQUIRE((i1 == 21));
	REQUIRE((i2 == 24));
	REQUIRE((v1 == 21));
	REQUIRE((v2 == 23));
	REQUIRE((v3 == 30));
}

template <typename T>
void associative_ordered_container_key_value_check(sol::state& lua, T& data, T& reflect) {
	typedef typename T::key_type K;
	typedef typename T::mapped_type V;
	lua["collect"] = [&reflect](K k, V v) {
		reflect.insert({ k, v });
	};

#if SOL_LUA_VERSION > 502
	lua["val"] = data;
	auto r = lua.safe_script(R"(
for k, v in pairs(val) do
	collect(k, v)
end
print()
)", sol::script_pass_on_error);
	REQUIRE(r.valid());
#else
	reflect = data;
#endif
	REQUIRE((data == reflect));
}

template <typename T>
void fixed_container_check(sol::state& lua, T& items) {
	{
		auto r1 = lua.safe_script(R"(
for i=1,#c do 
	v = c[i] 
	assert(v == (i + 10)) 
end
		)", sol::script_pass_on_error);
		REQUIRE(r1.valid());
	}
	{
		auto ffind = [&]() {
			auto r1 = lua.safe_script("i1 = c:find(11)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("i2 = c:find(14)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fget = [&]() {
			auto r1 = lua.safe_script("v1 = c:get(2)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("v2 = c:get(5)", sol::script_pass_on_error);
			REQUIRE(r2.valid());
		};
		auto fset = [&]() {
			auto r1 = lua.safe_script("c:set(2, 20)", sol::script_pass_on_error);
			REQUIRE(r1.valid());
			auto r2 = lua.safe_script("c:set(6, 16)", sol::script_pass_on_error);
			REQUIRE_FALSE(r2.valid());
		};
		auto ferase = [&]() {
			auto r5 = lua.safe_script("s1 = #c", sol::script_pass_on_error);
			REQUIRE(r5.valid());
			auto r1 = lua.safe_script("c:erase(i1)", sol::script_pass_on_error);
			REQUIRE_FALSE(r1.valid());
			auto r3 = lua.safe_script("s2 = #c", sol::script_pass_on_error);
			REQUIRE(r3.valid());
			auto r2 = lua.safe_script("c:erase(i2)", sol::script_pass_on_error);
			REQUIRE_FALSE(r2.valid());
			auto r4 = lua.safe_script("s3 = #c", sol::script_pass_on_error);
			REQUIRE(r4.valid());
		};
		auto fadd = [&]() {
			auto r = lua.safe_script("c:add(17)", sol::script_pass_on_error);
			REQUIRE_FALSE(r.valid());
		};
		auto fopset = [&]() {
			auto r = lua.safe_script("c[5] = 18", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		auto fopget = [&]() {
			auto r = lua.safe_script("v3 = c[4]", sol::script_pass_on_error);
			REQUIRE(r.valid());
		};
		REQUIRE_NOTHROW(ffind());
		REQUIRE_NOTHROW(fget());
		REQUIRE_NOTHROW(fset());
		REQUIRE_NOTHROW(ferase());
		REQUIRE_NOTHROW(fadd());
		REQUIRE_NOTHROW(fopset());
		REQUIRE_NOTHROW(fopget());
	}
	auto backit = std::begin(items);
	std::size_t len = 0;
	{
		auto e = std::end(items);
		auto last = backit;
		for (; backit != e; ++backit, ++len) {
			if (backit == e) {
				break;
			}
			last = backit;
		}
		backit = last;
	}
	const int& first = *std::begin(items);
	const int& last = *backit;
	int i1 = lua["i1"];
	int i2 = lua["i2"];
	std::size_t s1 = lua["s1"];
	std::size_t s2 = lua["s2"];
	std::size_t s3 = lua["s3"];
	int v1 = lua["v1"];
	int v2 = lua["v2"];
	int v3 = lua["v3"];
	int values[] = {
		11, 20, 13, 14, 18
	};
	{
		std::size_t idx = 0;
		for (const auto& i : items) {
			const auto& v = values[idx];
			REQUIRE((i == v));
			++idx;
		}
	}
	REQUIRE((first == 11));
	REQUIRE((last == 18));
	REQUIRE((s1 == 5));
	REQUIRE((s2 == 5));
	REQUIRE((s3 == 5));
	REQUIRE((len == 5));
	REQUIRE((i1 == 1));
	REQUIRE((i2 == 4));
	REQUIRE((v1 == 12));
	REQUIRE((v2 == 15));
	REQUIRE((v3 == 14));
}

template <typename T>
void lookup_container_check(sol::state& lua, T&) {
	auto result0 = lua.safe_script("assert(c['a'] == 'a')", sol::script_default_on_error);
	REQUIRE(result0.valid());
	auto result1 = lua.safe_script("assert(c['b'] == 'b')", sol::script_default_on_error);
	REQUIRE(result1.valid());
	auto result2 = lua.safe_script("assert(c['c'] == 'c')", sol::script_default_on_error);
	REQUIRE(result2.valid());
}

TEST_CASE("containers/sequence containers", "check all of the functinos for every single container") {
	SECTION("vector") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::vector<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		sequence_container_check(lua, items);
	}
	SECTION("list") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::list<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		sequence_container_check(lua, items);
	}
	SECTION("forward_list") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::forward_list<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		sequence_container_check(lua, items);
	}
	SECTION("deque") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::deque<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		sequence_container_check(lua, items);
	}
}

TEST_CASE("containers/fixed containers", "check immutable container types") {
	SECTION("array") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::array<int, 5> items{ { 11, 12, 13, 14, 15 } };
		lua["c"] = &items;
		fixed_container_check(lua, items);
	}
	SECTION("array ref") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::array<int, 5> items{ { 11, 12, 13, 14, 15 } };
		lua["c"] = std::ref(items);
		fixed_container_check(lua, items);
	}
	SECTION("c array") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		int items[5] = { 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		fixed_container_check(lua, items);
	}
	SECTION("c array ref") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		int items[5] = { 11, 12, 13, 14, 15 };
		lua["c"] = std::ref(items);
		fixed_container_check(lua, items);
	}
}

TEST_CASE("containers/ordered lookup containers", "check ordered container types") {
	SECTION("set") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::set<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		ordered_container_check(lua, items);
	}
	SECTION("set string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::set<std::string> items({ "a", "b", "c" });
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
	SECTION("multiset") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::multiset<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		ordered_container_check(lua, items);
	}
	SECTION("multiset string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::multiset<std::string> items({ "a", "b", "c" });
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
}

TEST_CASE("containers/unordered lookup containers", "check ordered container types") {
	SECTION("unordered_set") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_set<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		unordered_container_check(lua, items);
	}
	SECTION("unordered_set string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_set<std::string> items({ "a", "b", "c" });
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
	SECTION("unordered_multiset") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_multiset<int> items{ 11, 12, 13, 14, 15 };
		lua["c"] = &items;
		unordered_container_check(lua, items);
	}
	SECTION("unordered_multiset string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_multiset<std::string> items({ "a", "b", "c" });
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
}

TEST_CASE("containers/associative ordered containers", "check associative (map) containers that are ordered fulfill basic functionality requirements") {
	SECTION("map") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::map<short, int> items{
			{ (short)11, 21 },
			{ (short)12, 22 },
			{ (short)13, 23 },
			{ (short)14, 24 },
			{ (short)15, 25 }
		};
		lua["c"] = &items;
		associative_ordered_container_check(lua, items);
	}
	SECTION("map string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::map<std::string, std::string> items{
			{ "a", "a" },
			{ "b", "b" },
			{ "c", "c" }
		};
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
	SECTION("multimap") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::multimap<short, int> items{
			{ (short)11, 21 },
			{ (short)12, 22 },
			{ (short)13, 23 },
			{ (short)14, 24 },
			{ (short)15, 25 }
		};
		lua["c"] = &items;
		associative_ordered_container_check(lua, items);
	}
	SECTION("multimap string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::multimap<std::string, std::string> items{
			{ "a", "a" },
			{ "b", "b" },
			{ "c", "c" }
		};
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
}

TEST_CASE("containers/associative unordered containers", "check associative (map) containers that are ordered that they fulfill basic functionality requirements") {
	SECTION("unordered_map") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_map<short, int> items{
			{ (short)11, 21 },
			{ (short)12, 22 },
			{ (short)13, 23 },
			{ (short)14, 24 },
			{ (short)15, 25 }
		};
		lua["c"] = &items;
		associative_unordered_container_check(lua, items);
	}
	SECTION("unordered_map string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_map<std::string, std::string> items{
			{ "a", "a" },
			{ "b", "b" },
			{ "c", "c" }
		};
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
	SECTION("unordered_multimap") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_multimap<short, int> items{
			{ (short)11, 21 },
			{ (short)12, 22 },
			{ (short)13, 23 },
			{ (short)14, 24 },
			{ (short)15, 25 }
		};
		lua["c"] = &items;
		associative_unordered_container_check(lua, items);
	}
	SECTION("unordered_multimap string") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);

		std::unordered_multimap<std::string, std::string> items{
			{ "a", "a" },
			{ "b", "b" },
			{ "c", "c" }
		};
		lua["c"] = &items;
		lookup_container_check(lua, items);
	}
}

TEST_CASE("containers/associative ordered pairs", "check to make sure pairs works properly for key-value types") {
	struct bar {};
	std::unique_ptr<bar> ua(new bar()), ub(new bar()), uc(new bar());
	bar* a = ua.get();
	bar* b = ub.get();
	bar* c = uc.get();

	SECTION("map") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		std::map<std::string, bar*> data({ { "a", a },{ "b", b },{ "c", c } });
		std::map<std::string, bar*> reflect;
		associative_ordered_container_key_value_check(lua, data, reflect);
	}
	SECTION("multimap") {
		sol::state lua;
		lua.open_libraries(sol::lib::base);
		std::multimap<std::string, bar*> data({ { "a", a },{ "b", b },{ "c", c } });
		std::multimap<std::string, bar*> reflect;
		associative_ordered_container_key_value_check(lua, data, reflect);
	}
}

TEST_CASE("containers/auxiliary functions test", "make sure the manipulation functions are present and usable and working across various container types") {
	sol::state lua;
	lua.open_libraries();

	auto result1 = lua.safe_script(R"(
function g (x)
	x:add(20)
end

function h (x)
	x:add(20, 40)
end

function i (x)
	x:clear()
end

function sf (x,v)
	return x:find(v)
end

)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	// Have the function we
	// just defined in Lua
	sol::function g = lua["g"];
	sol::function h = lua["h"];
	sol::function i = lua["i"];
	sol::function sf = lua["sf"];

	// Set a global variable called
	// "arr" to be a vector of 5 lements
	lua["c_arr"] = std::array<int, 5>{ { 2, 4, 6, 8, 10 } };
	lua["arr"] = std::vector<int>{ 2, 4, 6, 8, 10 };
	lua["map"] = std::map<int, int>{ { 1, 2 }, { 2, 4 }, { 3, 6 }, { 4, 8 }, { 5, 10 } };
	lua["set"] = std::set<int>{ 2, 4, 6, 8, 10 };
	std::array<int, 5>& c_arr = lua["c_arr"];
	std::vector<int>& arr = lua["arr"];
	std::map<int, int>& map = lua["map"];
	std::set<int>& set = lua["set"];
	REQUIRE(c_arr.size() == 5);
	REQUIRE(arr.size() == 5);
	REQUIRE(map.size() == 5);
	REQUIRE(set.size() == 5);

	g(lua["set"]);
	g(lua["arr"]);
	h(lua["map"]);
	REQUIRE(arr.size() == 6);
	REQUIRE(map.size() == 6);
	REQUIRE(set.size() == 6);

	{
		int r = sf(set, 8);
		REQUIRE(r == 8);
		sol::object rn = sf(set, 9);
		REQUIRE(rn == sol::lua_nil);
	}

	{
		int r = sf(map, 3);
		REQUIRE(r == 6);
		sol::object rn = sf(map, 9);
		REQUIRE(rn == sol::lua_nil);
	}

	i(lua["arr"]);
	i(lua["map"]);
	i(lua["set"]);
	REQUIRE(arr.empty());
	REQUIRE(map.empty());
	REQUIRE(set.empty());

	auto result2 = lua.safe_script(R"(
c_arr[1] = 7
c_arr[2] = 7
c_arr[3] = 7
)", sol::script_pass_on_error);
	REQUIRE(result2.valid());
}

TEST_CASE("containers/indices test", "test indices on fixed array types") {
#if 0
	SECTION("zero index test") {
		sol::state lua;
		lua["c_arr"] = std::array<int, 5>{ { 2, 4, 6, 8, 10 } };
		auto result = lua.safe_script(R"(
c_arr[0] = 7
)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}

	SECTION("negative index test") {
		sol::state lua;
		lua["c_arr"] = std::array<int, 5>{ { 2, 4, 6, 8, 10 } };
		auto result = lua.safe_script(R"(
c_arr[-1] = 7
)", sol::script_pass_on_error);
		REQUIRE_FALSE(result.valid());
	}
#endif // Something is wrong with g++'s lower versions: it always fails this test...
}

TEST_CASE("containers/as_container reference", "test that we can force a container to be treated like one despite the trait being false using the proper marker") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.new_usertype<my_object>("my_object",
		sol::constructors<my_object(int)>(),
		sol::call_constructor, sol::constructors<my_object(int)>(),
		"size", &my_object::size,
		"iterable", [](my_object& mo) {
			return sol::as_container(mo);
		});

#if SOL_LUA_VERSION > 501
	auto result1 = lua.safe_script(R"(
mop = my_object.new(20)
for i, v in pairs(mop) do
	assert(i == v)
end
print(mop)
	)", sol::script_pass_on_error);
	REQUIRE(result1.valid());
	REQUIRE_NOTHROW([&]() {
		my_object& mo = lua["mop"];
		REQUIRE((&mo == my_object::last_printed));
	}());
#endif
	auto result2 = lua.safe_script(R"(
mo = my_object(10)
c_mo = mo
c_iterable = mo:iterable()
)", sol::script_pass_on_error);
	REQUIRE(result2.valid());

	REQUIRE_NOTHROW([&]() {
		my_object& mo = lua["c_mo"];
		my_object& mo_iterable = lua["c_iterable"];
		REQUIRE(&mo == &mo_iterable);
		REQUIRE(mo == mo_iterable);
	}());

	auto result3 = lua.safe_script(R"(
s1 = c_mo:size()
s1_len = #c_mo
s1_iterable = c_iterable:size()
s1_iterable_len = #c_iterable
)");
	REQUIRE(result3.valid());

	REQUIRE_NOTHROW([&]() {
		std::size_t s1 = lua["s1"];
		std::size_t s1_len = lua["s1_len"];
		std::size_t s1_iterable = lua["s1_iterable"];
		std::size_t s1_iterable_len = lua["s1_iterable_len"];
		REQUIRE(s1 == 10);
		REQUIRE(s1 == s1_len);
		REQUIRE(s1 == s1_iterable_len);
		REQUIRE(s1_iterable == s1_iterable_len);
	}());

	auto result4 = lua.safe_script(R"(
for i=1,#c_mo do
	v_iterable = c_iterable[i]
	assert(v_iterable == i)
end
)", sol::script_pass_on_error);
	REQUIRE(result4.valid());

	auto result5 = lua.safe_script(R"(
mo(5, 20)
c_iterable:insert(1, 100)
v1 = c_iterable[1]
s2 = c_mo:size()
s2_len = #c_mo
s2_iterable = c_iterable:size()
s2_iterable_len = #c_iterable
print(mo)
	)", sol::script_pass_on_error);
	REQUIRE(result5.valid());

	int v1 = lua["v1"];
	std::size_t s2 = lua["s2"];
	std::size_t s2_len = lua["s2_len"];
	std::size_t s2_iterable = lua["s2_iterable"];
	std::size_t s2_iterable_len = lua["s2_iterable_len"];
	REQUIRE(v1 == 100);
	REQUIRE(s2 == 16);
	REQUIRE(s2 == s2_len);
	REQUIRE(s2 == s2_iterable_len);
	REQUIRE(s2_iterable == s2_iterable_len);
	
	my_object& mo = lua["mo"];
	REQUIRE(&mo == my_object::last_printed);
}

TEST_CASE("containers/as_container", "test that we can force a container to be treated like one despite the trait being false using the proper marker") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("f", [](int v) {
		return sol::as_container(my_object(v));
	});

#if SOL_LUA_VERSION > 501
	auto result1 = lua.safe_script(R"(
mop = f(20)
for i, v in pairs(mop) do
	assert(i == v)
end
	)");
	REQUIRE(result1.valid());
#endif
	auto result2 = lua.safe_script(R"(
mo = f(10)
c_iterable = mo
)");
	REQUIRE(result2.valid());

	{
		my_object& mo = lua["mo"];
		my_object& mo_iterable = lua["c_iterable"];
		REQUIRE(&mo == &mo_iterable);
		REQUIRE(mo == mo_iterable);
	}

	auto result3 = lua.safe_script(R"(
s1_iterable = c_iterable:size()
s1_iterable_len = #c_iterable
)");
	REQUIRE(result3.valid());

	{
		std::size_t s1_iterable = lua["s1_iterable"];
		std::size_t s1_iterable_len = lua["s1_iterable_len"];
		REQUIRE(s1_iterable == 10);
		REQUIRE(s1_iterable == s1_iterable_len);
	}

	auto result4 = lua.safe_script(R"(
for i=1,#c_iterable do
	v_iterable = c_iterable[i]
	assert(v_iterable == i)
end
)");
	REQUIRE(result4.valid());

	auto result5 = lua.safe_script(R"(
c_iterable:insert(1, 100)
v1 = c_iterable:get(1)
s2_iterable = c_iterable:size()
s2_iterable_len = #c_iterable
	)");
	REQUIRE(result5.valid());

	{
		int v1 = lua["v1"];
		std::size_t s2_iterable = lua["s2_iterable"];
		std::size_t s2_iterable_len = lua["s2_iterable_len"];
		REQUIRE(v1 == 100);
		REQUIRE(s2_iterable_len == 11);
		REQUIRE(s2_iterable == s2_iterable_len);
	}
}
