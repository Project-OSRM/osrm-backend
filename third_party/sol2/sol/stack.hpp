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

#ifndef SOL_STACK_HPP
#define SOL_STACK_HPP

#include "trampoline.hpp"
#include "stack_core.hpp"
#include "stack_reference.hpp"
#include "stack_check.hpp"
#include "stack_get.hpp"
#include "stack_check_get.hpp"
#include "stack_push.hpp"
#include "stack_pop.hpp"
#include "stack_field.hpp"
#include "stack_probe.hpp"
#include <cstring>
#include <array>

namespace sol {
	namespace detail {
		using typical_chunk_name_t = char[32];

		inline const std::string& default_chunk_name() {
			static const std::string name = "";
			return name;
		}

		template <std::size_t N>
		const char* make_chunk_name(const string_view& code, const std::string& chunkname, char (&basechunkname)[N]) {
			if (chunkname.empty()) {
				auto it = code.cbegin();
				auto e = code.cend();
				std::size_t i = 0;
				static const std::size_t n = N - 4;
				for (i = 0; i < n && it != e; ++i, ++it) {
					basechunkname[i] = *it;
				}
				if (it != e) {
					for (std::size_t c = 0; c < 3; ++i, ++c) {
						basechunkname[i] = '.';
					}
				}
				basechunkname[i] = '\0';
				return &basechunkname[0];
			}
			else {
				return chunkname.c_str();
			}
		}
	} // namespace detail

	namespace stack {
		namespace stack_detail {
			template <typename T>
			inline int push_as_upvalues(lua_State* L, T& item) {
				typedef std::decay_t<T> TValue;
				static const std::size_t itemsize = sizeof(TValue);
				static const std::size_t voidsize = sizeof(void*);
				static const std::size_t voidsizem1 = voidsize - 1;
				static const std::size_t data_t_count = (sizeof(TValue) + voidsizem1) / voidsize;
				typedef std::array<void*, data_t_count> data_t;

				data_t data{ {} };
				std::memcpy(&data[0], std::addressof(item), itemsize);
				int pushcount = 0;
				for (auto&& v : data) {
					pushcount += push(L, lightuserdata_value(v));
				}
				return pushcount;
			}

			template <typename T>
			inline std::pair<T, int> get_as_upvalues(lua_State* L, int index = 2) {
				static const std::size_t data_t_count = (sizeof(T) + (sizeof(void*) - 1)) / sizeof(void*);
				typedef std::array<void*, data_t_count> data_t;
				data_t voiddata{ {} };
				for (std::size_t i = 0, d = 0; d < sizeof(T); ++i, d += sizeof(void*)) {
					voiddata[i] = get<lightuserdata_value>(L, upvalue_index(index++));
				}
				return std::pair<T, int>(*reinterpret_cast<T*>(static_cast<void*>(voiddata.data())), index);
			}

			struct evaluator {
				template <typename Fx, typename... Args>
				static decltype(auto) eval(types<>, std::index_sequence<>, lua_State*, int, record&, Fx&& fx, Args&&... args) {
					return std::forward<Fx>(fx)(std::forward<Args>(args)...);
				}

				template <typename Fx, typename Arg, typename... Args, std::size_t I, std::size_t... Is, typename... FxArgs>
				static decltype(auto) eval(types<Arg, Args...>, std::index_sequence<I, Is...>, lua_State* L, int start, record& tracking, Fx&& fx, FxArgs&&... fxargs) {
					return eval(types<Args...>(), std::index_sequence<Is...>(), L, start, tracking, std::forward<Fx>(fx), std::forward<FxArgs>(fxargs)..., stack_detail::unchecked_get<Arg>(L, start + tracking.used, tracking));
				}
			};

			template <bool checkargs = detail::default_safe_function_calls , std::size_t... I, typename R, typename... Args, typename Fx, typename... FxArgs, typename = std::enable_if_t<!std::is_void<R>::value >>
			inline decltype(auto) call(types<R>, types<Args...> ta, std::index_sequence<I...> tai, lua_State* L, int start, Fx&& fx, FxArgs&&... args) {
#ifndef _MSC_VER
				static_assert(meta::all<meta::is_not_move_only<Args>...>::value, "One of the arguments being bound is a move-only type, and it is not being taken by reference: this will break your code. Please take a reference and std::move it manually if this was your intention.");
#endif // This compiler make me so sad
				argument_handler<types<R, Args...>> handler{};
				multi_check<checkargs, Args...>(L, start, handler);
				record tracking{};
				return evaluator{}.eval(ta, tai, L, start, tracking, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
			}

			template <bool checkargs = detail::default_safe_function_calls, std::size_t... I, typename... Args, typename Fx, typename... FxArgs>
			inline void call(types<void>, types<Args...> ta, std::index_sequence<I...> tai, lua_State* L, int start, Fx&& fx, FxArgs&&... args) {
#ifndef _MSC_VER
				static_assert(meta::all<meta::is_not_move_only<Args>...>::value, "One of the arguments being bound is a move-only type, and it is not being taken by reference: this will break your code. Please take a reference and std::move it manually if this was your intention.");
#endif // This compiler make me so fucking sad
				argument_handler<types<void, Args...>> handler{};
				multi_check<checkargs, Args...>(L, start, handler);
				record tracking{};
				evaluator{}.eval(ta, tai, L, start, tracking, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
			}
		} // namespace stack_detail

		template <typename T>
		int set_ref(lua_State* L, T&& arg, int tableindex = -2) {
			push(L, std::forward<T>(arg));
			return luaL_ref(L, tableindex);
		}

		template <bool check_args = detail::default_safe_function_calls, typename R, typename... Args, typename Fx, typename... FxArgs, typename = std::enable_if_t<!std::is_void<R>::value>>
		inline decltype(auto) call(types<R> tr, types<Args...> ta, lua_State* L, int start, Fx&& fx, FxArgs&&... args) {
			typedef std::make_index_sequence<sizeof...(Args)> args_indices;
			return stack_detail::call<check_args>(tr, ta, args_indices(), L, start, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, typename R, typename... Args, typename Fx, typename... FxArgs, typename = std::enable_if_t<!std::is_void<R>::value>>
		inline decltype(auto) call(types<R> tr, types<Args...> ta, lua_State* L, Fx&& fx, FxArgs&&... args) {
			return call<check_args>(tr, ta, L, 1, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, typename... Args, typename Fx, typename... FxArgs>
		inline void call(types<void> tr, types<Args...> ta, lua_State* L, int start, Fx&& fx, FxArgs&&... args) {
			typedef std::make_index_sequence<sizeof...(Args)> args_indices;
			stack_detail::call<check_args>(tr, ta, args_indices(), L, start, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, typename... Args, typename Fx, typename... FxArgs>
		inline void call(types<void> tr, types<Args...> ta, lua_State* L, Fx&& fx, FxArgs&&... args) {
			call<check_args>(tr, ta, L, 1, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, typename R, typename... Args, typename Fx, typename... FxArgs, typename = std::enable_if_t<!std::is_void<R>::value>>
		inline decltype(auto) call_from_top(types<R> tr, types<Args...> ta, lua_State* L, Fx&& fx, FxArgs&&... args) {
			typedef meta::count_for_pack<lua_size, Args...> expected_count;
			return call<check_args>(tr, ta, L, (std::max)(static_cast<int>(lua_gettop(L) - expected_count::value), static_cast<int>(0)), std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, typename... Args, typename Fx, typename... FxArgs>
		inline void call_from_top(types<void> tr, types<Args...> ta, lua_State* L, Fx&& fx, FxArgs&&... args) {
			typedef meta::count_for_pack<lua_size, Args...> expected_count;
			call<check_args>(tr, ta, L, (std::max)(static_cast<int>(lua_gettop(L) - expected_count::value), static_cast<int>(0)), std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
		}

		template <bool check_args = detail::default_safe_function_calls, bool clean_stack = true, typename... Args, typename Fx, typename... FxArgs>
		inline int call_into_lua(types<void> tr, types<Args...> ta, lua_State* L, int start, Fx&& fx, FxArgs&&... fxargs) {
			call<check_args>(tr, ta, L, start, std::forward<Fx>(fx), std::forward<FxArgs>(fxargs)...);
			if (clean_stack) {
				lua_settop(L, 0);
			}
			return 0;
		}

		template <bool check_args = detail::default_safe_function_calls, bool clean_stack = true, typename Ret0, typename... Ret, typename... Args, typename Fx, typename... FxArgs, typename = std::enable_if_t<meta::neg<std::is_void<Ret0>>::value>>
		inline int call_into_lua(types<Ret0, Ret...>, types<Args...> ta, lua_State* L, int start, Fx&& fx, FxArgs&&... fxargs) {
			decltype(auto) r = call<check_args>(types<meta::return_type_t<Ret0, Ret...>>(), ta, L, start, std::forward<Fx>(fx), std::forward<FxArgs>(fxargs)...);
			typedef meta::unqualified_t<decltype(r)> R;
			typedef meta::any<is_stack_based<R>,
				std::is_same<R, absolute_index>,
				std::is_same<R, ref_index>,
				std::is_same<R, raw_index>>
				is_stack;
			if (clean_stack && !is_stack::value) {
				lua_settop(L, 0);
			}
			return push_reference(L, std::forward<decltype(r)>(r));
		}

		template <bool check_args = detail::default_safe_function_calls, bool clean_stack = true, typename Fx, typename... FxArgs>
		inline int call_lua(lua_State* L, int start, Fx&& fx, FxArgs&&... fxargs) {
			typedef lua_bind_traits<meta::unqualified_t<Fx>> traits_type;
			typedef typename traits_type::args_list args_list;
			typedef typename traits_type::returns_list returns_list;
			return call_into_lua<check_args, clean_stack>(returns_list(), args_list(), L, start, std::forward<Fx>(fx), std::forward<FxArgs>(fxargs)...);
		}

		inline call_syntax get_call_syntax(lua_State* L, const string_view& key, int index) {
			if (lua_gettop(L) == 0) {
				return call_syntax::dot;
			}
			luaL_getmetatable(L, key.data());
			auto pn = pop_n(L, 1);
			if (lua_compare(L, -1, index, LUA_OPEQ) != 1) {
				return call_syntax::dot;
			}
			return call_syntax::colon;
		}

		inline void script(lua_State* L, const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name(code, chunkname, basechunkname);
			if (luaL_loadbufferx(L, code.data(), code.size(), chunknametarget, to_string(mode).c_str()) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
				lua_error(L);
			}
		}

		inline void script_file(lua_State* L, const std::string& filename, load_mode mode = load_mode::any) {
			if (luaL_loadfilex(L, filename.c_str(), to_string(mode).c_str()) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
				lua_error(L);
			}
		}

		inline void luajit_exception_handler(lua_State* L, int (*handler)(lua_State*, lua_CFunction) = detail::c_trampoline) {
#if defined(SOL_LUAJIT) && !defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
			if (L == nullptr) {
				return;
			}
			lua_pushlightuserdata(L, (void*)handler);
			auto pn = pop_n(L, 1);
			luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
#else
			(void)L;
			(void)handler;
#endif
		}

		inline void luajit_exception_off(lua_State* L) {
#if defined(SOL_LUAJIT)
			if (L == nullptr) {
				return;
			}
			luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_OFF);
#else
			(void)L;
#endif
		}
	} // namespace stack
} // namespace sol

#endif // SOL_STACK_HPP
