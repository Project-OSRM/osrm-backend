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

#ifndef SOL_STACK_CHECK_UNQUALIFIED_HPP
#define SOL_STACK_CHECK_UNQUALIFIED_HPP

#include "stack_core.hpp"
#include "usertype_traits.hpp"
#include "inheritance.hpp"
#include <memory>
#include <functional>
#include <utility>
#include <cmath>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <optional>
#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
#include <variant>
#endif // SOL_STD_VARIANT
#endif // SOL_CXX17_FEATURES

namespace sol {
namespace stack {
	namespace stack_detail {
		template <typename T, bool poptable = true>
		inline bool check_metatable(lua_State* L, int index = -2) {
			const auto& metakey = usertype_traits<T>::metatable();
			luaL_getmetatable(L, &metakey[0]);
			const type expectedmetatabletype = static_cast<type>(lua_type(L, -1));
			if (expectedmetatabletype != type::lua_nil) {
				if (lua_rawequal(L, -1, index) == 1) {
					lua_pop(L, 1 + static_cast<int>(poptable));
					return true;
				}
			}
			lua_pop(L, 1);
			return false;
		}

		template <type expected, int (*check_func)(lua_State*, int)>
		struct basic_check {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				tracking.use(1);
				bool success = check_func(L, index) == 1;
				if (!success) {
					// expected type, actual type
					handler(L, index, expected, type_of(L, index), "");
				}
				return success;
			}
		};
	} // namespace stack_detail

	template <typename T, typename>
	struct userdata_checker {
		template <typename Handler>
		static bool check(lua_State*, int, type, Handler&&, record&) {
			return false;
		}
	};

	template <typename T, type expected, typename>
	struct checker {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			const type indextype = type_of(L, index);
			bool success = expected == indextype;
			if (!success) {
				// expected type, actual type, message
				handler(L, index, expected, indextype, "");
			}
			return success;
		}
	};

	template <typename T, type expected, typename C>
	struct qualified_checker : checker<meta::unqualified_t<T>, lua_type_of<meta::unqualified_t<T>>::value, C> {};

	template <typename T>
	struct checker<T, type::number, std::enable_if_t<std::is_integral<T>::value>> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
#if SOL_LUA_VERSION >= 503
#if defined(SOL_STRINGS_ARE_NUMBERS) && SOL_STRINGS_ARE_NUMBERS
			int isnum = 0;
			lua_tointegerx(L, index, &isnum);
			const bool success = isnum != 0;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::number, type_of(L, index), "not a numeric type or numeric string");
			}
#elif (defined(SOL_SAFE_NUMERICS) && SOL_SAFE_NUMERICS) && !(defined(SOL_NO_CHECK_NUMBER_PRECISION) && SOL_NO_CHECK_NUMBER_PRECISION)
			// this check is precise, does not convert
			if (lua_isinteger(L, index) == 1) {
				return true;
			}
			const bool success = false;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::number, type_of(L, index), "not a numeric (integral) type");
			}
#else
			type t = type_of(L, index);
			const bool success = t == type::number;
#endif // If numbers are enabled, use the imprecise check
			if (!success) {
				// expected type, actual type
				handler(L, index, type::number, type_of(L, index), "not a numeric type");
			}
			return success;
#else
#if !defined(SOL_STRINGS_ARE_NUMBERS) || !SOL_STRINGS_ARE_NUMBERS
			// must pre-check, because it will convert
			type t = type_of(L, index);
			if (t != type::number) {
				// expected type, actual type
				handler(L, index, type::number, t, "not a numeric type");
				return false;
			}
#endif // Do not allow strings to be numbers
#if (defined(SOL_SAFE_NUMERICS) && SOL_SAFE_NUMERICS) && !(defined(SOL_NO_CHECK_NUMBER_PRECISION) && SOL_NO_CHECK_NUMBER_PRECISION)
			int isnum = 0;
			const lua_Number v = lua_tonumberx(L, index, &isnum);
			const bool success = isnum != 0 && static_cast<lua_Number>(llround(v)) == v;
#else
			const bool success = true;
#endif // Safe numerics and number precision checking
			if (!success) {
				// expected type, actual type
#if defined(SOL_STRINGS_ARE_NUMBERS) && SOL_STRINGS_ARE_NUMBERS
				handler(L, index, type::number, type_of(L, index), "not a numeric type or numeric string");
#else
				handler(L, index, type::number, t, "not a numeric type");
#endif
			}
			return success;
#endif // Lua Version 5.3 versus others
		}
	};

	template <typename T>
	struct checker<T, type::number, std::enable_if_t<std::is_floating_point<T>::value>> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
#if defined(SOL_STRINGS_ARE_NUMBERS) && SOL_STRINGS_ARE_NUMBERS
			bool success = lua_isnumber(L, index) == 1;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::number, type_of(L, index), "not a numeric type or numeric string");
			}
			return success;
#else
			type t = type_of(L, index);
			bool success = t == type::number;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::number, t, "not a numeric type");
			}
			return success;
#endif // Strings are Numbers
		}
	};

	template <type expected, typename C>
	struct checker<lua_nil_t, expected, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			bool success = lua_isnil(L, index);
			if (success) {
				tracking.use(1);
				return success;
			}
			tracking.use(0);
			success = lua_isnone(L, index);
			if (!success) {
				// expected type, actual type
				handler(L, index, expected, type_of(L, index), "");
			}
			return success;
		}
	};

	template <typename C>
	struct checker<detail::non_lua_nil_t, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return !stack::unqualified_check<lua_nil_t>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <type expected, typename C>
	struct checker<nullopt_t, expected, C> : checker<lua_nil_t> {};

	template <typename C>
	struct checker<this_state, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State*, int, Handler&&, record& tracking) {
			tracking.use(0);
			return true;
		}
	};

	template <typename C>
	struct checker<this_main_state, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State*, int, Handler&&, record& tracking) {
			tracking.use(0);
			return true;
		}
	};

	template <typename C>
	struct checker<this_environment, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State*, int, Handler&&, record& tracking) {
			tracking.use(0);
			return true;
		}
	};

	template <typename C>
	struct checker<variadic_args, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State*, int, Handler&&, record& tracking) {
			tracking.use(0);
			return true;
		}
	};

	template <typename C>
	struct checker<type, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State*, int, Handler&&, record& tracking) {
			tracking.use(0);
			return true;
		}
	};

	template <typename T, typename C>
	struct checker<T, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			bool success = is_lua_reference<T>::value || !lua_isnone(L, index);
			if (!success) {
				// expected type, actual type
				handler(L, index, type::poly, type_of(L, index), "");
			}
			return success;
		}
	};

	template <typename T, typename C>
	struct checker<T, type::lightuserdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			type t = type_of(L, index);
			bool success = t == type::userdata || t == type::lightuserdata;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::lightuserdata, t, "");
			}
			return success;
		}
	};

	template <typename C>
	struct checker<userdata_value, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			type t = type_of(L, index);
			bool success = t == type::userdata;
			if (!success) {
				// expected type, actual type
				handler(L, index, type::userdata, t, "");
			}
			return success;
		}
	};

	template <typename B, typename C>
	struct checker<basic_userdata<B>, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack::check<userdata_value>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename T, typename C>
	struct checker<user<T>, type::userdata, C> : checker<user<T>, type::lightuserdata, C> {};

	template <typename T, typename C>
	struct checker<non_null<T>, type::userdata, C> : checker<T, lua_type_of<T>::value, C> {};

	template <typename C>
	struct checker<lua_CFunction, type::function, C> : stack_detail::basic_check<type::function, lua_iscfunction> {};
	template <typename C>
	struct checker<std::remove_pointer_t<lua_CFunction>, type::function, C> : checker<lua_CFunction, type::function, C> {};
	template <typename C>
	struct checker<c_closure, type::function, C> : checker<lua_CFunction, type::function, C> {};

	template <typename T, typename C>
	struct checker<T, type::function, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			type t = type_of(L, index);
			if (t == type::lua_nil || t == type::none || t == type::function) {
				// allow for lua_nil to be returned
				return true;
			}
			if (t != type::userdata && t != type::table) {
				handler(L, index, type::function, t, "must be a function or table or a userdata");
				return false;
			}
			// Do advanced check for call-style userdata?
			static const auto& callkey = to_string(meta_function::call);
			if (lua_getmetatable(L, index) == 0) {
				// No metatable, no __call key possible
				handler(L, index, type::function, t, "value is not a function and does not have overriden metatable");
				return false;
			}
			if (lua_isnoneornil(L, -1)) {
				lua_pop(L, 1);
				handler(L, index, type::function, t, "value is not a function and does not have valid metatable");
				return false;
			}
			lua_getfield(L, -1, &callkey[0]);
			if (lua_isnoneornil(L, -1)) {
				lua_pop(L, 2);
				handler(L, index, type::function, t, "value's metatable does not have __call overridden in metatable, cannot call this type");
				return false;
			}
			// has call, is definitely a function
			lua_pop(L, 2);
			return true;
		}
	};

	template <typename T, typename C>
	struct checker<T, type::table, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			type t = type_of(L, index);
			if (t == type::table) {
				return true;
			}
			if (t != type::userdata) {
				handler(L, index, type::table, t, "value is not a table or a userdata that can behave like one");
				return false;
			}
			return true;
		}
	};

	template <type expected, typename C>
	struct checker<metatable_t, expected, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			if (lua_getmetatable(L, index) == 0) {
				return true;
			}
			type t = type_of(L, -1);
			if (t == type::table || t == type::none || t == type::lua_nil) {
				lua_pop(L, 1);
				return true;
			}
			if (t != type::userdata) {
				lua_pop(L, 1);
				handler(L, index, expected, t, "value does not have a valid metatable");
				return false;
			}
			return true;
		}
	};

	template <typename C>
	struct checker<env_t, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			type t = type_of(L, index);
			if (t == type::table || t == type::none || t == type::lua_nil || t == type::userdata) {
				return true;
			}
			handler(L, index, type::table, t, "value cannot not have a valid environment");
			return true;
		}
	};

	template <typename E, typename C>
	struct checker<basic_environment<E>, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			tracking.use(1);
			if (lua_getmetatable(L, index) == 0) {
				return true;
			}
			type t = type_of(L, -1);
			if (t == type::table || t == type::none || t == type::lua_nil) {
				lua_pop(L, 1);
				return true;
			}
			if (t != type::userdata) {
				lua_pop(L, 1);
				handler(L, index, type::table, t, "value does not have a valid metatable");
				return false;
			}
			return true;
		}
	};

	template <typename T, typename C>
	struct checker<detail::as_value_tag<T>, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			const type indextype = type_of(L, index);
			return check(types<T>(), L, index, indextype, handler, tracking);
		}

		template <typename U, typename Handler>
		static bool check(types<U>, lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
#if defined(SOL_ENABLE_INTEROP) && SOL_ENABLE_INTEROP
			userdata_checker<extensible<T>> uc;
			(void)uc;
			if (uc.check(L, index, indextype, handler, tracking)) {
				return true;
			}
#endif // interop extensibility
			tracking.use(1);
			if (indextype != type::userdata) {
				handler(L, index, type::userdata, indextype, "value is not a valid userdata");
				return false;
			}
			if (meta::any<std::is_same<T, lightuserdata_value>, std::is_same<T, userdata_value>, std::is_same<T, userdata>, std::is_same<T, lightuserdata>>::value)
				return true;
			if (lua_getmetatable(L, index) == 0) {
				return true;
			}
			int metatableindex = lua_gettop(L);
			if (stack_detail::check_metatable<U>(L, metatableindex))
				return true;
			if (stack_detail::check_metatable<U*>(L, metatableindex))
				return true;
			if (stack_detail::check_metatable<detail::unique_usertype<U>>(L, metatableindex))
				return true;
			if (stack_detail::check_metatable<as_container_t<U>>(L, metatableindex))
				return true;
			bool success = false;
			if (detail::has_derived<T>::value) {
				auto pn = stack::pop_n(L, 1);
				lua_pushstring(L, &detail::base_class_check_key()[0]);
				lua_rawget(L, metatableindex);
				if (type_of(L, -1) != type::lua_nil) {
					void* basecastdata = lua_touserdata(L, -1);
					detail::inheritance_check_function ic = reinterpret_cast<detail::inheritance_check_function>(basecastdata);
					success = ic(usertype_traits<T>::qualified_name());
				}
			}
			if (!success) {
				lua_pop(L, 1);
				handler(L, index, type::userdata, indextype, "value at this index does not properly reflect the desired type");
				return false;
			}
			lua_pop(L, 1);
			return true;
		}
	};

	template <typename T, typename C>
	struct checker<detail::as_pointer_tag<T>, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
			if (indextype == type::lua_nil) {
				tracking.use(1);
				return true;
			}
			return stack_detail::check_usertype<T>(std::false_type(), L, index, indextype, std::forward<Handler>(handler), tracking);
		}

		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			const type indextype = type_of(L, index);
			return check(L, index, handler, indextype, tracking);
		}
	};

	template <typename T, typename C>
	struct checker<T, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return check_usertype<T>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename T, typename C>
	struct checker<T*, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return check_usertype<T*>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename X>
	struct checker<X, type::userdata, std::enable_if_t<is_unique_usertype<X>::value>> {
		typedef typename unique_usertype_traits<X>::type T;
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			const type indextype = type_of(L, index);
			tracking.use(1);
			if (indextype != type::userdata) {
				handler(L, index, type::userdata, indextype, "value is not a userdata");
				return false;
			}
			if (lua_getmetatable(L, index) == 0) {
				return true;
			}
			int metatableindex = lua_gettop(L);
			if (stack_detail::check_metatable<detail::unique_usertype<T>>(L, metatableindex)) {
				void* memory = lua_touserdata(L, index);
				memory = detail::align_usertype_unique_destructor(memory);
				detail::unique_destructor& pdx = *static_cast<detail::unique_destructor*>(memory);
				bool success = &detail::usertype_unique_alloc_destroy<T, X> == pdx;
				if (!success) {
					memory = detail::align_usertype_unique_tag<true>(memory);
#if 0
					// New version
#else
					const char*& name_tag = *static_cast<const char**>(memory);
					success = usertype_traits<X>::qualified_name() == name_tag;
#endif
					if (!success) {
						handler(L, index, type::userdata, indextype, "value is a userdata but is not the correct unique usertype");
					}
				}
				return success;
			}
			lua_pop(L, 1);
			handler(L, index, type::userdata, indextype, "unrecognized userdata (not pushed by sol?)");
			return false;
		}
	};

	template <typename T, typename C>
	struct checker<std::reference_wrapper<T>, type::userdata, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack::check<T>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename... Args, typename C>
	struct checker<std::tuple<Args...>, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack::multi_check<Args...>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename A, typename B, typename C>
	struct checker<std::pair<A, B>, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack::multi_check<A, B>(L, index, std::forward<Handler>(handler), tracking);
		}
	};

	template <typename T, typename C>
	struct checker<optional<T>, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&&, record& tracking) {
			type t = type_of(L, index);
			if (t == type::none) {
				tracking.use(0);
				return true;
			}
			if (t == type::lua_nil) {
				tracking.use(1);
				return true;
			}
			return stack::check<T>(L, index, no_panic, tracking);
		}
	};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES

	template <typename T, typename C>
	struct checker<std::optional<T>, type::poly, C> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&&, record& tracking) {
			type t = type_of(L, index);
			if (t == type::none) {
				tracking.use(0);
				return true;
			}
			if (t == type::lua_nil) {
				tracking.use(1);
				return true;
			}
			return stack::check<T>(L, index, no_panic, tracking);
		}
	};

#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT

	template <typename... Tn, typename C>
	struct checker<std::variant<Tn...>, type::poly, C> {
		typedef std::variant<Tn...> V;
		typedef std::variant_size<V> V_size;
		typedef std::integral_constant<bool, V_size::value == 0> V_is_empty;

		template <typename Handler>
		static bool is_one(std::integral_constant<std::size_t, 0>, lua_State* L, int index, Handler&& handler, record& tracking) {
			if (V_is_empty::value && lua_isnone(L, index)) {
				return true;
			}
			tracking.use(1);
			handler(L, index, type::poly, type_of(L, index), "value does not fit any type present in the variant");
			return false;
		}

		template <std::size_t I, typename Handler>
		static bool is_one(std::integral_constant<std::size_t, I>, lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef std::variant_alternative_t<I - 1, V> T;
			record temp_tracking = tracking;
			if (stack::check<T>(L, index, no_panic, temp_tracking)) {
				tracking = temp_tracking;
				return true;
			}
			return is_one(std::integral_constant<std::size_t, I - 1>(), L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return is_one(std::integral_constant<std::size_t, V_size::value>(), L, index, std::forward<Handler>(handler), tracking);
		}
	};

#endif // SOL_STD_VARIANT

#endif // SOL_CXX17_FEATURES
}
} // namespace sol::stack

#endif // SOL_STACK_CHECK_UNQUALIFIED_HPP
