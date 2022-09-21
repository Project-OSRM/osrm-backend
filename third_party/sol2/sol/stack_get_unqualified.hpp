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

#ifndef SOL_STACK_UNQUALIFIED_GET_HPP
#define SOL_STACK_UNQUALIFIED_GET_HPP

#include "stack_core.hpp"
#include "usertype_traits.hpp"
#include "inheritance.hpp"
#include "overload.hpp"
#include "error.hpp"
#include "unicode.hpp"

#include <memory>
#include <functional>
#include <utility>
#include <cstdlib>
#include <cmath>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <string_view>
#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
#include <variant>
#endif // Apple clang screwed up
#endif // C++17

namespace sol {
namespace stack {

	template <typename U>
	struct userdata_getter<U> {
		typedef stack_detail::strip_extensible_t<U> T;

		static std::pair<bool, T*> get(lua_State*, int, void*, record&) {
			return { false, nullptr };
		}
	};

	template <typename T, typename>
	struct getter {
		static T& get(lua_State* L, int index, record& tracking) {
			return getter<detail::as_value_tag<T>>{}.get(L, index, tracking);
		}
	};

	template <typename T, typename C>
	struct qualified_getter : getter<meta::unqualified_t<T>, C> {};

	template <typename T>
	struct getter<T, std::enable_if_t<std::is_floating_point<T>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return static_cast<T>(lua_tonumber(L, index));
		}
	};

	template <typename T>
	struct getter<T, std::enable_if_t<std::is_integral<T>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
#if SOL_LUA_VERSION >= 503
			if (lua_isinteger(L, index) != 0) {
				return static_cast<T>(lua_tointeger(L, index));
			}
#endif
			return static_cast<T>(llround(lua_tonumber(L, index)));
		}
	};

	template <typename T>
	struct getter<T, std::enable_if_t<std::is_enum<T>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return static_cast<T>(lua_tointegerx(L, index, nullptr));
		}
	};

	template <typename T>
	struct getter<as_table_t<T>> {
		typedef meta::unqualified_t<T> Tu;

		template <typename V>
		static void push_back_at_end(std::true_type, types<V>, lua_State* L, T& arr, std::size_t) {
			arr.push_back(stack::get<V>(L, -lua_size<V>::value));
		}

		template <typename V>
		static void push_back_at_end(std::false_type, types<V> t, lua_State* L, T& arr, std::size_t idx) {
			insert_at_end(meta::has_insert<Tu>(), t, L, arr, idx);
		}

		template <typename V>
		static void insert_at_end(std::true_type, types<V>, lua_State* L, T& arr, std::size_t) {
			using std::end;
			arr.insert(end(arr), stack::get<V>(L, -lua_size<V>::value));
		}

		template <typename V>
		static void insert_at_end(std::false_type, types<V>, lua_State* L, T& arr, std::size_t idx) {
			arr[idx] = stack::get<V>(L, -lua_size<V>::value);
		}

		static bool max_size_check(std::false_type, T&, std::size_t) {
			return false;
		}

		static bool max_size_check(std::true_type, T& arr, std::size_t idx) {
			return idx >= arr.max_size();
		}

		static T get(lua_State* L, int relindex, record& tracking) {
			return get(meta::has_key_value_pair<meta::unqualified_t<T>>(), L, relindex, tracking);
		}

		static T get(std::false_type, lua_State* L, int relindex, record& tracking) {
			typedef typename T::value_type V;
			return get(types<V>(), L, relindex, tracking);
		}

		template <typename V>
		static T get(types<V> t, lua_State* L, int relindex, record& tracking) {
			tracking.use(1);

			int index = lua_absindex(L, relindex);
			T arr;
			std::size_t idx = 0;
#if SOL_LUA_VERSION >= 503
			// This method is HIGHLY performant over regular table iteration thanks to the Lua API changes in 5.3
			// Questionable in 5.4
			for (lua_Integer i = 0;; i += lua_size<V>::value) {
				if (max_size_check(meta::has_max_size<Tu>(), arr, idx)) {
					return arr;
				}
				bool isnil = false;
				for (int vi = 0; vi < lua_size<V>::value; ++vi) {
#if defined(LUA_NILINTABLE) && LUA_NILINTABLE
					lua_pushinteger(L, static_cast<lua_Integer>(i + vi));
					if (lua_keyin(L, index) == 0) {
						// it's time to stop
						isnil = true;
					}
					else {
						// we have a key, have to get the value
						lua_geti(L, index, i + vi);
					}
#else
					type vt = static_cast<type>(lua_geti(L, index, i + vi));
					isnil = vt == type::none
						|| vt == type::lua_nil;
#endif
					if (isnil) {
						if (i == 0) {
							break;
						}
#if defined(LUA_NILINTABLE) && LUA_NILINTABLE
						lua_pop(L, vi);
#else
						lua_pop(L, (vi + 1));
#endif
						return arr;
					}
				}
				if (isnil) {
#if defined(LUA_NILINTABLE) && LUA_NILINTABLE
#else
					lua_pop(L, lua_size<V>::value);
#endif
					continue;
				}
				push_back_at_end(meta::has_push_back<Tu>(), t, L, arr, idx);
				++idx;
				lua_pop(L, lua_size<V>::value);
			}
#else
			// Zzzz slower but necessary thanks to the lower version API and missing functions qq
			for (lua_Integer i = 0;; i += lua_size<V>::value, lua_pop(L, lua_size<V>::value)) {
				if (idx >= arr.max_size()) {
					return arr;
				}
				bool isnil = false;
				for (int vi = 0; vi < lua_size<V>::value; ++vi) {
					lua_pushinteger(L, i);
					lua_gettable(L, index);
					type vt = type_of(L, -1);
					isnil = vt == type::lua_nil;
					if (isnil) {
						if (i == 0) {
							break;
						}
						lua_pop(L, (vi + 1));
						return arr;
					}
				}
				if (isnil)
					continue;
				push_back_at_end(meta::has_push_back<Tu>(), t, L, arr, idx);
				++idx;
			}
#endif
			return arr;
		}

		static T get(std::true_type, lua_State* L, int index, record& tracking) {
			typedef typename T::value_type P;
			typedef typename P::first_type K;
			typedef typename P::second_type V;
			return get(types<K, V>(), L, index, tracking);
		}

		template <typename K, typename V>
		static T get(types<K, V>, lua_State* L, int relindex, record& tracking) {
			tracking.use(1);

			T associative;
			int index = lua_absindex(L, relindex);
			lua_pushnil(L);
			while (lua_next(L, index) != 0) {
				decltype(auto) key = stack::check_get<K>(L, -2);
				if (!key) {
					lua_pop(L, 1);
					continue;
				}
				associative.emplace(std::forward<decltype(*key)>(*key), stack::get<V>(L, -1));
				lua_pop(L, 1);
			}
			return associative;
		}
	};

	template <typename T, typename Al>
	struct getter<as_table_t<std::forward_list<T, Al>>> {
		typedef std::forward_list<T, Al> C;

		static C get(lua_State* L, int relindex, record& tracking) {
			return get(meta::has_key_value_pair<C>(), L, relindex, tracking);
		}

		static C get(std::true_type, lua_State* L, int index, record& tracking) {
			typedef typename T::value_type P;
			typedef typename P::first_type K;
			typedef typename P::second_type V;
			return get(types<K, V>(), L, index, tracking);
		}

		static C get(std::false_type, lua_State* L, int relindex, record& tracking) {
			typedef typename C::value_type V;
			return get(types<V>(), L, relindex, tracking);
		}

		template <typename V>
		static C get(types<V>, lua_State* L, int relindex, record& tracking) {
			tracking.use(1);

			int index = lua_absindex(L, relindex);
			C arr;
			auto at = arr.cbefore_begin();
			std::size_t idx = 0;
#if SOL_LUA_VERSION >= 503
			// This method is HIGHLY performant over regular table iteration thanks to the Lua API changes in 5.3
			for (lua_Integer i = 0;; i += lua_size<V>::value, lua_pop(L, lua_size<V>::value)) {
				if (idx >= arr.max_size()) {
					return arr;
				}
				bool isnil = false;
				for (int vi = 0; vi < lua_size<V>::value; ++vi) {
					type t = static_cast<type>(lua_geti(L, index, i + vi));
					isnil = t == type::lua_nil;
					if (isnil) {
						if (i == 0) {
							break;
						}
						lua_pop(L, (vi + 1));
						return arr;
					}
				}
				if (isnil)
					continue;
				at = arr.insert_after(at, stack::get<V>(L, -lua_size<V>::value));
				++idx;
			}
#else
			// Zzzz slower but necessary thanks to the lower version API and missing functions qq
			for (lua_Integer i = 0;; i += lua_size<V>::value, lua_pop(L, lua_size<V>::value)) {
				if (idx >= arr.max_size()) {
					return arr;
				}
				bool isnil = false;
				for (int vi = 0; vi < lua_size<V>::value; ++vi) {
					lua_pushinteger(L, i);
					lua_gettable(L, index);
					type t = type_of(L, -1);
					isnil = t == type::lua_nil;
					if (isnil) {
						if (i == 0) {
							break;
						}
						lua_pop(L, (vi + 1));
						return arr;
					}
				}
				if (isnil)
					continue;
				at = arr.insert_after(at, stack::get<V>(L, -lua_size<V>::value));
				++idx;
			}
#endif
			return arr;
		}

		template <typename K, typename V>
		static C get(types<K, V>, lua_State* L, int relindex, record& tracking) {
			tracking.use(1);

			C associative;
			auto at = associative.cbefore_begin();
			int index = lua_absindex(L, relindex);
			lua_pushnil(L);
			while (lua_next(L, index) != 0) {
				decltype(auto) key = stack::check_get<K>(L, -2);
				if (!key) {
					lua_pop(L, 1);
					continue;
				}
				at = associative.emplace_after(at, std::forward<decltype(*key)>(*key), stack::get<V>(L, -1));
				lua_pop(L, 1);
			}
			return associative;
		}
	};

	template <typename T>
	struct getter<nested<T>, std::enable_if_t<!is_container<T>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			getter<T> g;
			// VC++ has a bad warning here: shut it up
			(void)g;
			return g.get(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<nested<T>, std::enable_if_t<meta::all<is_container<T>, meta::neg<meta::has_key_value_pair<meta::unqualified_t<T>>>>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			typedef typename T::value_type V;
			getter<as_table_t<T>> g;
			// VC++ has a bad warning here: shut it up
			(void)g;
			return g.get(types<nested<V>>(), L, index, tracking);
		}
	};

	template <typename T>
	struct getter<nested<T>, std::enable_if_t<meta::all<is_container<T>, meta::has_key_value_pair<meta::unqualified_t<T>>>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			typedef typename T::value_type P;
			typedef typename P::first_type K;
			typedef typename P::second_type V;
			getter<as_table_t<T>> g;
			// VC++ has a bad warning here: shut it up
			(void)g;
			return g.get(types<K, nested<V>>(), L, index, tracking);
		}
	};

	template <typename T>
	struct getter<T, std::enable_if_t<is_lua_reference<T>::value>> {
		static T get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return T(L, index);
		}
	};

	template <>
	struct getter<userdata_value> {
		static userdata_value get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return userdata_value(lua_touserdata(L, index));
		}
	};

	template <>
	struct getter<lightuserdata_value> {
		static lightuserdata_value get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return lightuserdata_value(lua_touserdata(L, index));
		}
	};

	template <typename T>
	struct getter<light<T>> {
		static light<T> get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			void* memory = lua_touserdata(L, index);
			return light<T>(static_cast<T*>(memory));
		}
	};

	template <typename T>
	struct getter<user<T>> {
		static std::add_lvalue_reference_t<T> get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			void* memory = lua_touserdata(L, index);
			memory = detail::align_user<T>(memory);
			return *static_cast<std::remove_reference_t<T>*>(memory);
		}
	};

	template <typename T>
	struct getter<user<T*>> {
		static T* get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			void* memory = lua_touserdata(L, index);
			memory = detail::align_user<T*>(memory);
			return static_cast<T*>(memory);
		}
	};

	template <>
	struct getter<type> {
		static type get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return static_cast<type>(lua_type(L, index));
		}
	};

	template <>
	struct getter<bool> {
		static bool get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return lua_toboolean(L, index) != 0;
		}
	};

	template <>
	struct getter<std::string> {
		static std::string get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			std::size_t len;
			auto str = lua_tolstring(L, index, &len);
			return std::string(str, len);
		}
	};

	template <>
	struct getter<const char*> {
		static const char* get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			size_t sz;
			return lua_tolstring(L, index, &sz);
		}
	};

	template <>
	struct getter<char> {
		static char get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			size_t len;
			auto str = lua_tolstring(L, index, &len);
			return len > 0 ? str[0] : '\0';
		}
	};

	template <typename Traits>
	struct getter<basic_string_view<char, Traits>> {
		static string_view get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			size_t sz;
			const char* str = lua_tolstring(L, index, &sz);
			return basic_string_view<char, Traits>(str, sz);
		}
	};

	template <typename Traits, typename Al>
	struct getter<std::basic_string<wchar_t, Traits, Al>> {
		typedef std::basic_string<wchar_t, Traits, Al> S;
		static S get(lua_State* L, int index, record& tracking) {
			typedef std::conditional_t<sizeof(wchar_t) == 2, char16_t, char32_t> Ch;
			typedef typename std::allocator_traits<Al>::template rebind_alloc<Ch> ChAl;
			typedef std::char_traits<Ch> ChTraits;
			getter<std::basic_string<Ch, ChTraits, ChAl>> g;
			(void)g;
			return g.template get_into<S>(L, index, tracking);
		}
	};

	template <typename Traits, typename Al>
	struct getter<std::basic_string<char16_t, Traits, Al>> {
		template <typename F>
		static void convert(const char* strb, const char* stre, F&& f) {
			char32_t cp = 0;
			for (const char* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf8_to_code_point(strtarget, stre);
				if (dr.error != unicode::error_code::ok) {
					cp = unicode::unicode_detail::replacement;
					++strtarget;
				}
				else {
					cp = dr.codepoint;
					strtarget = dr.next;
				}
				auto er = unicode::code_point_to_utf16(cp);
				f(er);
			}
		}

		template <typename S>
		static S get_into(lua_State* L, int index, record& tracking) {
			typedef typename S::value_type Ch;
			tracking.use(1);
			size_t len;
			auto utf8p = lua_tolstring(L, index, &len);
			if (len < 1)
				return S();
			std::size_t needed_size = 0;
			const char* strb = utf8p;
			const char* stre = utf8p + len;
			auto count_units = [&needed_size](const unicode::encoded_result<char16_t> er) {
				needed_size += er.code_units_size;
			};
			convert(strb, stre, count_units);
			S r(needed_size, static_cast<Ch>(0));
			r.resize(needed_size);
			Ch* target = &r[0];
			auto copy_units = [&target](const unicode::encoded_result<char16_t> er) {
				std::memcpy(target, er.code_units.data(), er.code_units_size * sizeof(Ch));
				target += er.code_units_size;
			};
			convert(strb, stre, copy_units);
			return r;
		}

		static std::basic_string<char16_t, Traits, Al> get(lua_State* L, int index, record& tracking) {
			return get_into<std::basic_string<char16_t, Traits, Al>>(L, index, tracking);
		}
	};

	template <typename Traits, typename Al>
	struct getter<std::basic_string<char32_t, Traits, Al>> {
		template <typename F>
		static void convert(const char* strb, const char* stre, F&& f) {
			char32_t cp = 0;
			for (const char* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf8_to_code_point(strtarget, stre);
				if (dr.error != unicode::error_code::ok) {
					cp = unicode::unicode_detail::replacement;
					++strtarget;
				}
				else {
					cp = dr.codepoint;
					strtarget = dr.next;
				}
				auto er = unicode::code_point_to_utf32(cp);
				f(er);
			}
		}

		template <typename S>
		static S get_into(lua_State* L, int index, record& tracking) {
			typedef typename S::value_type Ch;
			tracking.use(1);
			size_t len;
			auto utf8p = lua_tolstring(L, index, &len);
			if (len < 1)
				return S();
			std::size_t needed_size = 0;
			const char* strb = utf8p;
			const char* stre = utf8p + len;
			auto count_units = [&needed_size](const unicode::encoded_result<char32_t> er) {
				needed_size += er.code_units_size;
			};
			convert(strb, stre, count_units);
			S r(needed_size, static_cast<Ch>(0));
			r.resize(needed_size);
			Ch* target = &r[0];
			auto copy_units = [&target](const unicode::encoded_result<char32_t> er) {
				std::memcpy(target, er.code_units.data(), er.code_units_size * sizeof(Ch));
				target += er.code_units_size;
			};
			convert(strb, stre, copy_units);
			return r;
		}

		static std::basic_string<char32_t, Traits, Al> get(lua_State* L, int index, record& tracking) {
			return get_into<std::basic_string<char32_t, Traits, Al>>(L, index, tracking);
		}
	};

	template <>
	struct getter<char16_t> {
		static char16_t get(lua_State* L, int index, record& tracking) {
			string_view utf8 = stack::get<string_view>(L, index, tracking);
			const char* strb = utf8.data();
			const char* stre = utf8.data() + utf8.size();
			char32_t cp = 0;
			auto dr = unicode::utf8_to_code_point(strb, stre);
			if (dr.error != unicode::error_code::ok) {
				cp = unicode::unicode_detail::replacement;
			}
			else {
				cp = dr.codepoint;
			}
			auto er = unicode::code_point_to_utf16(cp);
			return er.code_units[0];
		}
	};

	template <>
	struct getter<char32_t> {
		static char32_t get(lua_State* L, int index, record& tracking) {
			string_view utf8 = stack::get<string_view>(L, index, tracking);
			const char* strb = utf8.data();
			const char* stre = utf8.data() + utf8.size();
			char32_t cp = 0;
			auto dr = unicode::utf8_to_code_point(strb, stre);
			if (dr.error != unicode::error_code::ok) {
				cp = unicode::unicode_detail::replacement;
			}
			else {
				cp = dr.codepoint;
			}
			auto er = unicode::code_point_to_utf32(cp);
			return er.code_units[0];
		}
	};

	template <>
	struct getter<wchar_t> {
		static wchar_t get(lua_State* L, int index, record& tracking) {
			typedef std::conditional_t<sizeof(wchar_t) == 2, char16_t, char32_t> Ch;
			getter<Ch> g;
			(void)g;
			auto c = g.get(L, index, tracking);
			return static_cast<wchar_t>(c);
		}
	};

	template <>
	struct getter<meta_function> {
		static meta_function get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			const char* name = getter<const char*>{}.get(L, index, tracking);
			const auto& mfnames = meta_function_names();
			for (std::size_t i = 0; i < mfnames.size(); ++i)
				if (mfnames[i] == name)
					return static_cast<meta_function>(i);
			return meta_function::construct;
		}
	};

	template <>
	struct getter<lua_nil_t> {
		static lua_nil_t get(lua_State*, int, record& tracking) {
			tracking.use(1);
			return lua_nil;
		}
	};

	template <>
	struct getter<std::nullptr_t> {
		static std::nullptr_t get(lua_State*, int, record& tracking) {
			tracking.use(1);
			return nullptr;
		}
	};

	template <>
	struct getter<nullopt_t> {
		static nullopt_t get(lua_State*, int, record& tracking) {
			tracking.use(1);
			return nullopt;
		}
	};

	template <>
	struct getter<this_state> {
		static this_state get(lua_State* L, int, record& tracking) {
			tracking.use(0);
			return this_state(L);
		}
	};

	template <>
	struct getter<this_main_state> {
		static this_main_state get(lua_State* L, int, record& tracking) {
			tracking.use(0);
			return this_main_state(main_thread(L, L));
		}
	};

	template <>
	struct getter<lua_CFunction> {
		static lua_CFunction get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return lua_tocfunction(L, index);
		}
	};

	template <>
	struct getter<c_closure> {
		static c_closure get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return c_closure(lua_tocfunction(L, index), -1);
		}
	};

	template <>
	struct getter<error> {
		static error get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			size_t sz = 0;
			const char* err = lua_tolstring(L, index, &sz);
			if (err == nullptr) {
				return error(detail::direct_error, "");
			}
			return error(detail::direct_error, std::string(err, sz));
		}
	};

	template <>
	struct getter<void*> {
		static void* get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return lua_touserdata(L, index);
		}
	};

	template <>
	struct getter<const void*> {
		static const void* get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			return lua_touserdata(L, index);
		}
	};

	template <typename T>
	struct getter<detail::as_value_tag<T>> {
		static T* get_no_lua_nil(lua_State* L, int index, record& tracking) {
			void* memory = lua_touserdata(L, index);
#if defined(SOL_ENABLE_INTEROP) && SOL_ENABLE_INTEROP
			userdata_getter<extensible<T>> ug;
			(void)ug;
			auto ugr = ug.get(L, index, memory, tracking);
			if (ugr.first) {
				return ugr.second;
			}
#endif // interop extensibility
			tracking.use(1);
			void* rawdata = detail::align_usertype_pointer(memory);
			void** pudata = static_cast<void**>(rawdata);
			void* udata = *pudata;
			return get_no_lua_nil_from(L, udata, index, tracking);
		}

		static T* get_no_lua_nil_from(lua_State* L, void* udata, int index, record&) {
			if (detail::has_derived<T>::value && luaL_getmetafield(L, index, &detail::base_class_cast_key()[0]) != 0) {
				void* basecastdata = lua_touserdata(L, -1);
				detail::inheritance_cast_function ic = reinterpret_cast<detail::inheritance_cast_function>(basecastdata);
				// use the casting function to properly adjust the pointer for the desired T
				udata = ic(udata, usertype_traits<T>::qualified_name());
				lua_pop(L, 1);
			}
			T* obj = static_cast<T*>(udata);
			return obj;
		}

		static T& get(lua_State* L, int index, record& tracking) {
			return *get_no_lua_nil(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<detail::as_pointer_tag<T>> {
		static T* get(lua_State* L, int index, record& tracking) {
			type t = type_of(L, index);
			if (t == type::lua_nil) {
				tracking.use(1);
				return nullptr;
			}
			getter<detail::as_value_tag<T>> g;
			// Avoid VC++ warning
			(void)g;
			return g.get_no_lua_nil(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<non_null<T*>> {
		static T* get(lua_State* L, int index, record& tracking) {
			getter<detail::as_value_tag<T>> g;
			// Avoid VC++ warning
			(void)g;
			return g.get_no_lua_nil(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<T&> {
		static T& get(lua_State* L, int index, record& tracking) {
			getter<detail::as_value_tag<T>> g;
			// Avoid VC++ warning
			(void)g;
			return g.get(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<std::reference_wrapper<T>> {
		static T& get(lua_State* L, int index, record& tracking) {
			getter<T&> g;
			// Avoid VC++ warning
			(void)g;
			return g.get(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<T*> {
		static T* get(lua_State* L, int index, record& tracking) {
			getter<detail::as_pointer_tag<T>> g;
			// Avoid VC++ warning
			(void)g;
			return g.get(L, index, tracking);
		}
	};

	template <typename T>
	struct getter<T, std::enable_if_t<is_unique_usertype<T>::value>> {
		typedef typename unique_usertype_traits<T>::type P;
		typedef typename unique_usertype_traits<T>::actual_type Real;

		static Real& get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			void* memory = lua_touserdata(L, index);
			memory = detail::align_usertype_unique<Real>(memory);
			Real* mem = static_cast<Real*>(memory);
			return *mem;
		}
	};

	template <typename... Tn>
	struct getter<std::tuple<Tn...>> {
		typedef std::tuple<decltype(stack::get<Tn>(nullptr, 0))...> R;

		template <typename... Args>
		static R apply(std::index_sequence<>, lua_State*, int, record&, Args&&... args) {
			// Fuck you too, VC++
			return R{ std::forward<Args>(args)... };
		}

		template <std::size_t I, std::size_t... Ix, typename... Args>
		static R apply(std::index_sequence<I, Ix...>, lua_State* L, int index, record& tracking, Args&&... args) {
			// Fuck you too, VC++
			typedef std::tuple_element_t<I, std::tuple<Tn...>> T;
			return apply(std::index_sequence<Ix...>(), L, index, tracking, std::forward<Args>(args)..., stack::get<T>(L, index + tracking.used, tracking));
		}

		static R get(lua_State* L, int index, record& tracking) {
			return apply(std::make_index_sequence<sizeof...(Tn)>(), L, index, tracking);
		}
	};

	template <typename A, typename B>
	struct getter<std::pair<A, B>> {
		static decltype(auto) get(lua_State* L, int index, record& tracking) {
			return std::pair<decltype(stack::get<A>(L, index)), decltype(stack::get<B>(L, index))>{ stack::get<A>(L, index, tracking), stack::get<B>(L, index + tracking.used, tracking) };
		}
	};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES


#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
	template <typename... Tn>
	struct getter<std::variant<Tn...>> {
		typedef std::variant<Tn...> V;
		typedef std::variant_size<V> V_size;
		typedef std::integral_constant<bool, V_size::value == 0> V_is_empty;

		static V get_empty(std::true_type, lua_State*, int, record&) {
			return V();
		}

		static V get_empty(std::false_type, lua_State* L, int index, record& tracking) {
			typedef std::variant_alternative_t<0, V> T;
			// This should never be reached...
			// please check your code and understand what you did to bring yourself here
			std::abort();
			return V(std::in_place_index<0>, stack::get<T>(L, index, tracking));
		}

		static V get_one(std::integral_constant<std::size_t, 0>, lua_State* L, int index, record& tracking) {
			return get_empty(V_is_empty(), L, index, tracking);
		}

		template <std::size_t I>
		static V get_one(std::integral_constant<std::size_t, I>, lua_State* L, int index, record& tracking) {
			typedef std::variant_alternative_t<I - 1, V> T;
			record temp_tracking = tracking;
			if (stack::check<T>(L, index, no_panic, temp_tracking)) {
				tracking = temp_tracking;
				return V(std::in_place_index<I - 1>, stack::get<T>(L, index));
			}
			return get_one(std::integral_constant<std::size_t, I - 1>(), L, index, tracking);
		}

		static V get(lua_State* L, int index, record& tracking) {
			return get_one(std::integral_constant<std::size_t, V_size::value>(), L, index, tracking);
		}
	};
#endif // SOL_STD_VARIANT
#endif // SOL_CXX17_FEATURES
}
} // namespace sol::stack

#endif // SOL_STACK_UNQUALIFIED_GET_HPP
