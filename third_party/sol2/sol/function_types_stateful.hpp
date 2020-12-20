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

#ifndef SOL_FUNCTION_TYPES_STATEFUL_HPP
#define SOL_FUNCTION_TYPES_STATEFUL_HPP

#include "function_types_core.hpp"

namespace sol {
namespace function_detail {
	template <typename Func, bool is_yielding, bool no_trampoline>
	struct functor_function {
		typedef std::decay_t<meta::unwrap_unqualified_t<Func>> function_type;
		function_type fx;

		template <typename... Args>
		functor_function(function_type f, Args&&... args)
		: fx(std::move(f), std::forward<Args>(args)...) {
		}

		int call(lua_State* L) {
			int nr = call_detail::call_wrapped<void, true, false>(L, fx);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			if (!no_trampoline) {
				auto f = [&](lua_State*) -> int { return this->call(L); };
				return detail::trampoline(L, f);
			}
			else {
				return call(L);
			}
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct member_function {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef meta::function_return_t<function_type> return_type;
		typedef meta::function_args_t<function_type> args_lists;
		function_type invocation;
		T member;

		template <typename... Args>
		member_function(function_type f, Args&&... args)
		: invocation(std::move(f)), member(std::forward<Args>(args)...) {
		}

		int call(lua_State* L) {
			int nr = call_detail::call_wrapped<T, true, false, -1>(L, invocation, detail::unwrap(detail::deref(member)));
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			auto f = [&](lua_State*) -> int { return this->call(L); };
			return detail::trampoline(L, f);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct member_variable {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef typename meta::bind_traits<function_type>::return_type return_type;
		typedef typename meta::bind_traits<function_type>::args_list args_lists;
		function_type var;
		T member;
		typedef std::add_lvalue_reference_t<meta::unwrapped_t<std::remove_reference_t<decltype(detail::deref(member))>>> M;

		template <typename... Args>
		member_variable(function_type v, Args&&... args)
		: var(std::move(v)), member(std::forward<Args>(args)...) {
		}

		int call(lua_State* L) {
			int nr;
			{
				M mem = detail::unwrap(detail::deref(member));
				switch (lua_gettop(L)) {
				case 0:
					nr = call_detail::call_wrapped<T, true, false, -1>(L, var, mem);
					break;
				case 1:
					nr = call_detail::call_wrapped<T, false, false, -1>(L, var, mem);
					break;
				default:
					nr = luaL_error(L, "sol: incorrect number of arguments to member variable function");
					break;
				}
			}
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			auto f = [&](lua_State*) -> int { return this->call(L); };
			return detail::trampoline(L, f);
		}
	};
}
} // namespace sol::function_detail

#endif // SOL_FUNCTION_TYPES_STATEFUL_HPP
