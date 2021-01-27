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

#ifndef SOL_USERTYPE_CORE_HPP
#define SOL_USERTYPE_CORE_HPP

#include "wrapper.hpp"
#include "stack.hpp"
#include "types.hpp"
#include "stack_reference.hpp"
#include "usertype_traits.hpp"
#include "inheritance.hpp"
#include "raii.hpp"
#include "deprecate.hpp"
#include "object.hpp"

#include <sstream>

namespace sol {
	namespace usertype_detail {
		struct no_comp {
			template <typename A, typename B>
			bool operator()(A&&, B&&) const {
				return false;
			}
		};

		template <typename T>
		int is_check(lua_State* L) {
			return stack::push(L, stack::check<T>(L, 1, &no_panic));
		}

		template <typename T>
		inline int member_default_to_string(std::true_type, lua_State* L) {
			decltype(auto) ts = stack::get<T>(L, 1).to_string();
			return stack::push(L, std::forward<decltype(ts)>(ts));
		}

		template <typename T>
		inline int member_default_to_string(std::false_type, lua_State* L) {
			return luaL_error(L, "cannot perform to_string on '%s': no 'to_string' overload in namespace, 'to_string' member function, or operator<<(ostream&, ...) present", detail::demangle<T>().data());
		}

		template <typename T>
		inline int adl_default_to_string(std::true_type, lua_State* L) {
			using namespace std;
			decltype(auto) ts = to_string(stack::get<T>(L, 1));
			return stack::push(L, std::forward<decltype(ts)>(ts));
		}

		template <typename T>
		inline int adl_default_to_string(std::false_type, lua_State* L) {
			return member_default_to_string<T>(meta::supports_to_string_member<T>(), L);
		}

		template <typename T>
		inline int oss_default_to_string(std::true_type, lua_State* L) {
			std::ostringstream oss;
			oss << stack::unqualified_get<T>(L, 1);
			return stack::push(L, oss.str());
		}

		template <typename T>
		inline int oss_default_to_string(std::false_type, lua_State* L) {
			return adl_default_to_string<T>(meta::supports_adl_to_string<T>(), L);
		}

		template <typename T>
		inline int default_to_string(lua_State* L) {
			return oss_default_to_string<T>(meta::supports_ostream_op<T>(), L);
		}

		template <typename T, typename Op>
		int comparsion_operator_wrap(lua_State* L) {
			auto maybel = stack::unqualified_check_get<T&>(L, 1);
			if (maybel) {
				auto mayber = stack::unqualified_check_get<T&>(L, 2);
				if (mayber) {
					auto& l = *maybel;
					auto& r = *mayber;
					if (std::is_same<no_comp, Op>::value) {
						return stack::push(L, detail::ptr(l) == detail::ptr(r));
					}
					else {
						Op op;
						return stack::push(L, (detail::ptr(l) == detail::ptr(r)) || op(detail::deref(l), detail::deref(r)));
					}
				}
			}
			return stack::push(L, false);
		}

		template <typename T, typename Op, typename Supports, typename Regs, meta::enable<Supports> = meta::enabler>
		inline void make_reg_op(Regs& l, int& index, const char* name) {
			lua_CFunction f = &comparsion_operator_wrap<T, Op>;
			l[index] = luaL_Reg{ name, f };
			++index;
		}

		template <typename T, typename Op, typename Supports, typename Regs, meta::disable<Supports> = meta::enabler>
		inline void make_reg_op(Regs&, int&, const char*) {
			// Do nothing if there's no support
		}

		template <typename T, typename Supports, typename Regs, meta::enable<Supports> = meta::enabler>
		inline void make_to_string_op(Regs& l, int& index) {
			const char* name = to_string(meta_function::to_string).c_str();
			lua_CFunction f = &detail::static_trampoline<&default_to_string<T>>;
			l[index] = luaL_Reg{ name, f };
			++index;
		}

		template <typename T, typename Supports, typename Regs, meta::disable<Supports> = meta::enabler>
		inline void make_to_string_op(Regs&, int&) {
			// Do nothing if there's no support
		}

		template <typename T, typename Regs, meta::enable<meta::has_deducible_signature<T>> = meta::enabler>
		inline void make_call_op(Regs& l, int& index) {
			const char* name = to_string(meta_function::call).c_str();
			lua_CFunction f = &c_call<decltype(&T::operator()), &T::operator()>;
			l[index] = luaL_Reg{ name, f };
			++index;
		}

		template <typename T, typename Regs, meta::disable<meta::has_deducible_signature<T>> = meta::enabler>
		inline void make_call_op(Regs&, int&) {
			// Do nothing if there's no support
		}

		template <typename T, typename Regs>
		inline void make_length_op_const(std::true_type, Regs& l, int& index) {
			const char* name = to_string(meta_function::length).c_str();
#if defined(__clang__)
			l[index] = luaL_Reg{ name, &c_call<decltype(&T::size), &T::size> };
#else
			typedef decltype(std::declval<T>().size()) R;
			using sz_func = R(T::*)()const;
			l[index] = luaL_Reg{ name, &c_call<decltype(static_cast<sz_func>(&T::size)), static_cast<sz_func>(&T::size)> };
#endif
			++index;
		}

		template <typename T, typename Regs>
		inline void make_length_op_const(std::false_type, Regs& l, int& index) {
			const char* name = to_string(meta_function::length).c_str();
#if defined(__clang__)
			l[index] = luaL_Reg{ name, &c_call<decltype(&T::size), &T::size> };
#else
			typedef decltype(std::declval<T>().size()) R;
			using sz_func = R(T::*)();
			l[index] = luaL_Reg{ name, &c_call<decltype(static_cast<sz_func>(&T::size)), static_cast<sz_func>(&T::size)> };
#endif
			++index;
		}

		template <typename T, typename Regs, meta::enable<meta::has_size<T>, meta::has_size<const T>> = meta::enabler>
		inline void make_length_op(Regs& l, int& index) {
			make_length_op_const<T>(meta::has_size<const T>(), l, index);
		}

		template <typename T, typename Regs, meta::disable<meta::has_size<T>, meta::has_size<const T>> = meta::enabler>
		inline void make_length_op(Regs&, int&) {
			// Do nothing if there's no support
		}

		template <typename T, typename Regs, meta::enable<meta::neg<std::is_pointer<T>>, std::is_destructible<T>>>
		void make_destructor(Regs& l, int& index) {
			const char* name = to_string(meta_function::garbage_collect).c_str();
			l[index] = luaL_Reg{ name, is_unique_usertype<T>::value ? &detail::unique_destruct<T> : &detail::usertype_alloc_destruct<T> };
			++index;
		}

		template <typename T, typename Regs, meta::disable<meta::neg<std::is_pointer<T>>, std::is_destructible<T>>>
		void make_destructor(Regs& l, int& index) {
			if (!std::is_destructible<T>::value) {
				// if the value is not destructible, plant an erroring __gc method
				// to warn the user of a problem when it comes around
				// this won't trigger if the user performs `new_usertype` / `new_simple_usertype` and
				// rigs the class up properly
				const char* name = to_string(meta_function::garbage_collect).c_str();
				l[index] = luaL_Reg{ name, &detail::cannot_destruct<T> };
				++index;
			}
		}

		template <typename T, typename Regs, typename Fx>
		void insert_default_registrations(std::false_type, Regs&, int&, Fx&&) {
			// no-op
		}

		template <typename T, typename Regs, typename Fx>
		void insert_default_registrations(std::true_type, Regs& l, int& index, Fx&& fx) {
			if (fx(meta_function::less_than)) {
				const char* name = to_string(meta_function::less_than).c_str();
				usertype_detail::make_reg_op<T, std::less<>, meta::supports_op_less<T>>(l, index, name);
			}
			if (fx(meta_function::less_than_or_equal_to)) {
				const char* name = to_string(meta_function::less_than_or_equal_to).c_str();
				usertype_detail::make_reg_op<T, std::less_equal<>, meta::supports_op_less_equal<T>>(l, index, name);
			}
			if (fx(meta_function::equal_to)) {
				const char* name = to_string(meta_function::equal_to).c_str();
				usertype_detail::make_reg_op<T, std::conditional_t<meta::supports_op_equal<T>::value, std::equal_to<>, usertype_detail::no_comp>, std::true_type>(l, index, name);
			}
			if (fx(meta_function::pairs)) {
				const char* name = to_string(meta_function::pairs).c_str();
				l[index] = luaL_Reg{ name, container_usertype_metatable<as_container_t<T>>::pairs_call };
				++index;
			}
			if (fx(meta_function::length)) {
				usertype_detail::make_length_op<T>(l, index);
			}
			if (fx(meta_function::to_string)) {
				usertype_detail::make_to_string_op<T, is_to_stringable<T>>(l, index);
			}
			if (fx(meta_function::call_function)) {
				usertype_detail::make_call_op<T>(l, index);
			}
		}

		template <typename T, typename Regs, typename Fx>
		void insert_default_registrations(Regs& l, int& index, Fx&& fx) {
			insert_default_registrations<T>(is_automagical<T>(), l, index, std::forward<Fx>(fx));
		}
	} // namespace usertype_detail

	namespace stack { namespace stack_detail {
		template <typename T>
		struct undefined_metatable {
			typedef meta::all<meta::neg<std::is_pointer<T>>, std::is_destructible<T>> is_destructible;
			typedef std::remove_pointer_t<T> P;
			lua_State* L;
			const char* key;

			undefined_metatable(lua_State* l, const char* k)
			: L(l), key(k) {
			}

			void operator()() const {
				if (luaL_newmetatable(L, key) == 1) {
					luaL_Reg l[32]{};
					int index = 0;
					auto fx = [](meta_function) { return true; };
					usertype_detail::insert_default_registrations<P>(l, index, fx);
					usertype_detail::make_destructor<T>(l, index);
					luaL_setfuncs(L, l, 0);

					// __type table
					lua_createtable(L, 0, 2);
					const std::string& name = detail::demangle<T>();
					lua_pushlstring(L, name.c_str(), name.size());
					lua_setfield(L, -2, "name");
					lua_CFunction is_func = &usertype_detail::is_check<T>;
					lua_pushcclosure(L, is_func, 0);
					lua_setfield(L, -2, "is");
					lua_setfield(L, -2, to_string(meta_function::type).c_str());
				}
				lua_setmetatable(L, -2);
			}
		};
	}
	} // namespace stack::stack_detail
} // namespace sol

#endif // SOL_USERTYPE_CORE_HPP
