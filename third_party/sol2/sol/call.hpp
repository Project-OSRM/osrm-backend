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

#ifndef SOL_CALL_HPP
#define SOL_CALL_HPP

#include "protect.hpp"
#include "wrapper.hpp"
#include "trampoline.hpp"
#include "property.hpp"
#include "filters.hpp"
#include "stack.hpp"

namespace sol {
	namespace usertype_detail {

	} // namespace usertype_detail

	namespace filter_detail {
		template <int I, int... In>
		inline void handle_filter(static_stack_dependencies<I, In...>, lua_State* L, int&) {
			if (sizeof...(In) == 0) {
				return;
			}
			absolute_index ai(L, I);
			if (type_of(L, ai) != type::userdata) {
				return;
			}
			lua_createtable(L, static_cast<int>(sizeof...(In)), 0);
			stack_reference deps(L, -1);
			auto per_dep = [&L, &deps](int i) {
				lua_pushvalue(L, i);
				luaL_ref(L, deps.stack_index());
			};
			(void)per_dep;
			(void)detail::swallow{ int(), (per_dep(In), int())... };
			lua_setuservalue(L, ai);
		}

		template <int... In>
		inline void handle_filter(returns_self_with<In...>, lua_State* L, int& pushed) {
			pushed = stack::push(L, raw_index(1));
			handle_filter(static_stack_dependencies<-1, In...>(), L, pushed);
		}

		inline void handle_filter(const stack_dependencies& sdeps, lua_State* L, int&) {
			absolute_index ai(L, sdeps.target);
			if (type_of(L, ai) != type::userdata) {
				return;
			}
			lua_createtable(L, static_cast<int>(sdeps.size()), 0);
			stack_reference deps(L, -1);
			for (std::size_t i = 0; i < sdeps.size(); ++i) {
				lua_pushvalue(L, sdeps.stack_indices[i]);
				luaL_ref(L, deps.stack_index());
			}
			lua_setuservalue(L, ai);
		}

		template <typename P, meta::disable<std::is_base_of<detail::filter_base_tag, meta::unqualified_t<P>>> = meta::enabler>
		inline void handle_filter(P&& p, lua_State* L, int& pushed) {
			pushed = std::forward<P>(p)(L, pushed);
		}
	} // namespace filter_detail

	namespace function_detail {
		inline int no_construction_error(lua_State* L) {
			return luaL_error(L, "sol: cannot call this constructor (tagged as non-constructible)");
		}
	} // namespace function_detail

	namespace call_detail {

		template <typename R, typename W>
		inline auto& pick(std::true_type, property_wrapper<R, W>& f) {
			return f.read;
		}

		template <typename R, typename W>
		inline auto& pick(std::false_type, property_wrapper<R, W>& f) {
			return f.write;
		}

		template <typename T, typename List>
		struct void_call : void_call<T, meta::function_args_t<List>> {};

		template <typename T, typename... Args>
		struct void_call<T, types<Args...>> {
			static void call(Args...) {
			}
		};

		template <typename T, bool checked, bool clean_stack>
		struct constructor_match {
			T* obj;

			constructor_match(T* o)
			: obj(o) {
			}

			template <typename Fx, std::size_t I, typename... R, typename... Args>
			int operator()(types<Fx>, index_value<I>, types<R...> r, types<Args...> a, lua_State* L, int, int start) const {
				detail::default_construct func{};
				return stack::call_into_lua<checked, clean_stack>(r, a, L, start, func, obj);
			}
		};

		namespace overload_detail {
			template <std::size_t... M, typename Match, typename... Args>
			inline int overload_match_arity(types<>, std::index_sequence<>, std::index_sequence<M...>, Match&&, lua_State* L, int, int, Args&&...) {
				return luaL_error(L, "sol: no matching function call takes this number of arguments and the specified types");
			}

			template <typename Fx, typename... Fxs, std::size_t I, std::size_t... In, std::size_t... M, typename Match, typename... Args>
			inline int overload_match_arity(types<Fx, Fxs...>, std::index_sequence<I, In...>, std::index_sequence<M...>, Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
				typedef lua_bind_traits<meta::unwrap_unqualified_t<Fx>> traits;
				typedef meta::tuple_types<typename traits::return_type> return_types;
				typedef typename traits::free_args_list args_list;
				// compile-time eliminate any functions that we know ahead of time are of improper arity
				if (!traits::runtime_variadics_t::value && meta::find_in_pack_v<index_value<traits::free_arity>, index_value<M>...>::value) {
					return overload_match_arity(types<Fxs...>(), std::index_sequence<In...>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				if (!traits::runtime_variadics_t::value && traits::free_arity != fxarity) {
					return overload_match_arity(types<Fxs...>(), std::index_sequence<In...>(), std::index_sequence<traits::free_arity, M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				stack::record tracking{};
				if (!stack::stack_detail::check_types<true>{}.check(args_list(), L, start, no_panic, tracking)) {
					return overload_match_arity(types<Fxs...>(), std::index_sequence<In...>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				return matchfx(types<Fx>(), index_value<I>(), return_types(), args_list(), L, fxarity, start, std::forward<Args>(args)...);
			}

			template <std::size_t... M, typename Match, typename... Args>
			inline int overload_match_arity_single(types<>, std::index_sequence<>, std::index_sequence<M...>, Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
				return overload_match_arity(types<>(), std::index_sequence<>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
			}

			template <typename Fx, std::size_t I, std::size_t... M, typename Match, typename... Args>
			inline int overload_match_arity_single(types<Fx>, std::index_sequence<I>, std::index_sequence<M...>, Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
				typedef lua_bind_traits<meta::unwrap_unqualified_t<Fx>> traits;
				typedef meta::tuple_types<typename traits::return_type> return_types;
				typedef typename traits::free_args_list args_list;
				// compile-time eliminate any functions that we know ahead of time are of improper arity
				if (!traits::runtime_variadics_t::value && meta::find_in_pack_v<index_value<traits::free_arity>, index_value<M>...>::value) {
					return overload_match_arity(types<>(), std::index_sequence<>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				if (!traits::runtime_variadics_t::value && traits::free_arity != fxarity) {
					return overload_match_arity(types<>(), std::index_sequence<>(), std::index_sequence<traits::free_arity, M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				return matchfx(types<Fx>(), index_value<I>(), return_types(), args_list(), L, fxarity, start, std::forward<Args>(args)...);
			}

			template <typename Fx, typename Fx1, typename... Fxs, std::size_t I, std::size_t I1, std::size_t... In, std::size_t... M, typename Match, typename... Args>
			inline int overload_match_arity_single(types<Fx, Fx1, Fxs...>, std::index_sequence<I, I1, In...>, std::index_sequence<M...>, Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
				typedef lua_bind_traits<meta::unwrap_unqualified_t<Fx>> traits;
				typedef meta::tuple_types<typename traits::return_type> return_types;
				typedef typename traits::free_args_list args_list;
				// compile-time eliminate any functions that we know ahead of time are of improper arity
				if (!traits::runtime_variadics_t::value && meta::find_in_pack_v<index_value<traits::free_arity>, index_value<M>...>::value) {
					return overload_match_arity(types<Fx1, Fxs...>(), std::index_sequence<I1, In...>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				if (!traits::runtime_variadics_t::value && traits::free_arity != fxarity) {
					return overload_match_arity(types<Fx1, Fxs...>(), std::index_sequence<I1, In...>(), std::index_sequence<traits::free_arity, M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				stack::record tracking{};
				if (!stack::stack_detail::check_types<true>{}.check(args_list(), L, start, no_panic, tracking)) {
					return overload_match_arity(types<Fx1, Fxs...>(), std::index_sequence<I1, In...>(), std::index_sequence<M...>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
				}
				return matchfx(types<Fx>(), index_value<I>(), return_types(), args_list(), L, fxarity, start, std::forward<Args>(args)...);
			}
		} // namespace overload_detail

		template <typename... Functions, typename Match, typename... Args>
		inline int overload_match_arity(Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
			return overload_detail::overload_match_arity_single(types<Functions...>(), std::make_index_sequence<sizeof...(Functions)>(), std::index_sequence<>(), std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
		}

		template <typename... Functions, typename Match, typename... Args>
		inline int overload_match(Match&& matchfx, lua_State* L, int start, Args&&... args) {
			int fxarity = lua_gettop(L) - (start - 1);
			return overload_match_arity<Functions...>(std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
		}

		template <typename T, typename... TypeLists, typename Match, typename... Args>
		inline int construct_match(Match&& matchfx, lua_State* L, int fxarity, int start, Args&&... args) {
			// use same overload resolution matching as all other parts of the framework
			return overload_match_arity<decltype(void_call<T, TypeLists>::call)...>(std::forward<Match>(matchfx), L, fxarity, start, std::forward<Args>(args)...);
		}

		template <typename T, bool checked, bool clean_stack, typename... TypeLists>
		inline int construct_trampolined(lua_State* L) {
			static const auto& meta = usertype_traits<T>::metatable();
			int argcount = lua_gettop(L);
			call_syntax syntax = argcount > 0 ? stack::get_call_syntax(L, usertype_traits<T>::user_metatable(), 1) : call_syntax::dot;
			argcount -= static_cast<int>(syntax);

			T* obj = detail::usertype_allocate<T>(L);
			reference userdataref(L, -1);
			userdataref.pop();

			construct_match<T, TypeLists...>(constructor_match<T, checked, clean_stack>(obj), L, argcount, 1 + static_cast<int>(syntax));

			userdataref.push();
			stack::stack_detail::undefined_metatable<T> umf(L, &meta[0]);
			umf();

			return 1;	
		}

		template <typename T, bool checked, bool clean_stack, typename... TypeLists>
		inline int construct(lua_State* L) {
			return detail::static_trampoline<&construct_trampolined<T, checked, clean_stack, TypeLists...>>(L);
		}

		template <typename F, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename = void>
		struct agnostic_lua_call_wrapper {
			typedef wrapper<meta::unqualified_t<F>> wrap;

			template <typename Fx, typename... Args>
			static int convertible_call(std::true_type, lua_State* L, Fx&& f, Args&&... args) {
				typedef typename wrap::traits_type traits_type;
				typedef typename traits_type::function_pointer_type fp_t;
				fp_t fx = f;
				return agnostic_lua_call_wrapper<fp_t, is_index, is_variable, checked, boost, clean_stack>{}.call(L, fx, std::forward<Args>(args)...);
			}

			template <typename Fx, typename... Args>
			static int convertible_call(std::false_type, lua_State* L, Fx&& f, Args&&... args) {
				typedef typename wrap::returns_list returns_list;
				typedef typename wrap::free_args_list args_list;
				typedef typename wrap::caller caller;
				return stack::call_into_lua<checked, clean_stack>(returns_list(), args_list(), L, boost + 1, caller(), std::forward<Fx>(f), std::forward<Args>(args)...);
			}

			template <typename Fx, typename... Args>
			static int call(lua_State* L, Fx&& f, Args&&... args) {
				typedef typename wrap::traits_type traits_type;
				typedef typename traits_type::function_pointer_type fp_t;
				return convertible_call(std::conditional_t<std::is_class<meta::unqualified_t<F>>::value, std::is_convertible<std::decay_t<Fx>, fp_t>, std::false_type>(), L, std::forward<Fx>(f), std::forward<Args>(args)...);
			}
		};

		template <typename T, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<var_wrapper<T>, true, is_variable, checked, boost, clean_stack, C> {
			template <typename F>
			static int call(lua_State* L, F&& f) {
				typedef is_stack_based<meta::unqualified_t<decltype(detail::unwrap(f.value))>> is_stack;
				if (clean_stack && !is_stack::value) {
					lua_settop(L, 0);
				}
				return stack::push_reference(L, detail::unwrap(f.value));
			}
		};

		template <typename T, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<var_wrapper<T>, false, is_variable, checked, boost, clean_stack, C> {
			template <typename V>
			static int call_assign(std::true_type, lua_State* L, V&& f) {
				detail::unwrap(f.value) = stack::unqualified_get<meta::unwrapped_t<T>>(L, boost + (is_variable ? 3 : 1));
				if (clean_stack) {
					lua_settop(L, 0);
				}
				return 0;
			}

			template <typename... Args>
			static int call_assign(std::false_type, lua_State* L, Args&&...) {
				return luaL_error(L, "sol: cannot write to this variable: copy assignment/constructor not available");
			}

			template <typename... Args>
			static int call_const(std::false_type, lua_State* L, Args&&... args) {
				typedef meta::unwrapped_t<T> R;
				return call_assign(std::is_assignable<std::add_lvalue_reference_t<meta::unqualified_t<R>>, R>(), L, std::forward<Args>(args)...);
			}

			template <typename... Args>
			static int call_const(std::true_type, lua_State* L, Args&&...) {
				return luaL_error(L, "sol: cannot write to a readonly (const) variable");
			}

			template <typename V>
			static int call(lua_State* L, V&& f) {
				return call_const(std::is_const<meta::unwrapped_t<T>>(), L, f);
			}
		};

		template <bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<lua_CFunction_ref, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, lua_CFunction_ref f) {
				return f(L);
			}
		};

		template <bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<lua_CFunction, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, lua_CFunction f) {
				return f(L);
			}
		};

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
		template <bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<detail::lua_CFunction_noexcept, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, detail::lua_CFunction_noexcept f) {
				return f(L);
			}
		};
#endif // noexcept function types

		template <bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<no_prop, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, const no_prop&) {
				return luaL_error(L, is_index ? "sol: cannot read from a writeonly property" : "sol: cannot write to a readonly property");
			}
		};

		template <bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<no_construction, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, const no_construction&) {
				return function_detail::no_construction_error(L);
			}
		};

		template <typename... Args, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<bases<Args...>, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State*, const bases<Args...>&) {
				// Uh. How did you even call this, lul
				return 0;
			}
		};

		template <typename T, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct agnostic_lua_call_wrapper<std::reference_wrapper<T>, is_index, is_variable, checked, boost, clean_stack, C> {
			static int call(lua_State* L, std::reference_wrapper<T> f) {
				return agnostic_lua_call_wrapper<T, is_index, is_variable, checked, boost, clean_stack>{}.call(L, f.get());
			}
		};

		template <typename T, typename F, bool is_index, bool is_variable, bool checked = detail::default_safe_function_calls, int boost = 0, bool clean_stack = true, typename = void>
		struct lua_call_wrapper : agnostic_lua_call_wrapper<F, is_index, is_variable, checked, boost, clean_stack> {};

		template <typename T, typename F, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack>
		struct lua_call_wrapper<T, F, is_index, is_variable, checked, boost, clean_stack, std::enable_if_t<std::is_member_function_pointer<F>::value>> {
			typedef wrapper<meta::unqualified_t<F>> wrap;
			typedef typename wrap::object_type object_type;

			template <typename Fx>
			static int call(lua_State* L, Fx&& f, object_type& o) {
				typedef typename wrap::returns_list returns_list;
				typedef typename wrap::args_list args_list;
				typedef typename wrap::caller caller;
				return stack::call_into_lua<checked, clean_stack>(returns_list(), args_list(), L, boost + (is_variable ? 3 : 2), caller(), std::forward<Fx>(f), o);
			}

			template <typename Fx>
			static int call(lua_State* L, Fx&& f) {
				typedef std::conditional_t<std::is_void<T>::value, object_type, T> Ta;
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeo = stack::unqualified_check_get<Ta*>(L, 1);
				if (!maybeo || maybeo.value() == nullptr) {
					return luaL_error(L, "sol: received nil for 'self' argument (use ':' for accessing member functions, make sure member variables are preceeded by the actual object with '.' syntax)");
				}
				object_type* o = static_cast<object_type*>(maybeo.value());
				return call(L, std::forward<Fx>(f), *o);
#else
				object_type& o = static_cast<object_type&>(*stack::unqualified_get<non_null<Ta*>>(L, 1));
				return call(L, std::forward<Fx>(f), o);
#endif // Safety
			}
		};

		template <typename T, typename F, bool is_variable, bool checked, int boost, bool clean_stack>
		struct lua_call_wrapper<T, F, false, is_variable, checked, boost, clean_stack, std::enable_if_t<std::is_member_object_pointer<F>::value>> {
			typedef lua_bind_traits<F> traits_type;
			typedef wrapper<meta::unqualified_t<F>> wrap;
			typedef typename wrap::object_type object_type;

			template <typename V>
			static int call_assign(std::true_type, lua_State* L, V&& f, object_type& o) {
				typedef typename wrap::args_list args_list;
				typedef typename wrap::caller caller;
				return stack::call_into_lua<checked, clean_stack>(types<void>(), args_list(), L, boost + (is_variable ? 3 : 2), caller(), f, o);
			}

			template <typename V>
			static int call_assign(std::true_type, lua_State* L, V&& f) {
				typedef std::conditional_t<std::is_void<T>::value, object_type, T> Ta;
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeo = stack::check_get<Ta*>(L, 1);
				if (!maybeo || maybeo.value() == nullptr) {
					if (is_variable) {
						return luaL_error(L, "sol: received nil for 'self' argument (bad '.' access?)");
					}
					return luaL_error(L, "sol: received nil for 'self' argument (pass 'self' as first argument)");
				}
				object_type* o = static_cast<object_type*>(maybeo.value());
				return call_assign(std::true_type(), L, f, *o);
#else
				object_type& o = static_cast<object_type&>(*stack::get<non_null<Ta*>>(L, 1));
				return call_assign(std::true_type(), L, f, o);
#endif // Safety
			}

			template <typename... Args>
			static int call_assign(std::false_type, lua_State* L, Args&&...) {
				return luaL_error(L, "sol: cannot write to this variable: copy assignment/constructor not available");
			}

			template <typename... Args>
			static int call_const(std::false_type, lua_State* L, Args&&... args) {
				typedef typename traits_type::return_type R;
				return call_assign(std::is_copy_assignable<meta::unqualified_t<R>>(), L, std::forward<Args>(args)...);
			}

			template <typename... Args>
			static int call_const(std::true_type, lua_State* L, Args&&...) {
				return luaL_error(L, "sol: cannot write to a readonly (const) variable");
			}

			template <typename V>
			static int call(lua_State* L, V&& f) {
				return call_const(std::is_const<typename traits_type::return_type>(), L, std::forward<V>(f));
			}

			template <typename V>
			static int call(lua_State* L, V&& f, object_type& o) {
				return call_const(std::is_const<typename traits_type::return_type>(), L, std::forward<V>(f), o);
			}
		};

		template <typename T, typename F, bool is_variable, bool checked, int boost, bool clean_stack>
		struct lua_call_wrapper<T, F, true, is_variable, checked, boost, clean_stack, std::enable_if_t<std::is_member_object_pointer<F>::value>> {
			typedef lua_bind_traits<F> traits_type;
			typedef wrapper<meta::unqualified_t<F>> wrap;
			typedef typename wrap::object_type object_type;

			template <typename V>
			static int call(lua_State* L, V&& v, object_type& o) {
				typedef typename wrap::returns_list returns_list;
				typedef typename wrap::caller caller;
				F f(std::forward<V>(v));
				return stack::call_into_lua<checked, clean_stack>(returns_list(), types<>(), L, boost + (is_variable ? 3 : 2), caller(), f, o);
			}

			template <typename V>
			static int call(lua_State* L, V&& f) {
				typedef std::conditional_t<std::is_void<T>::value, object_type, T> Ta;
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeo = stack::check_get<Ta*>(L, 1);
				if (!maybeo || maybeo.value() == nullptr) {
					if (is_variable) {
						return luaL_error(L, "sol: 'self' argument is lua_nil (bad '.' access?)");
					}
					return luaL_error(L, "sol: 'self' argument is lua_nil (pass 'self' as first argument)");
				}
				object_type* o = static_cast<object_type*>(maybeo.value());
				return call(L, f, *o);
#else
				object_type& o = static_cast<object_type&>(*stack::get<non_null<Ta*>>(L, 1));
				return call(L, f, o);
#endif // Safety
			}
		};

		template <typename T, typename F, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, readonly_wrapper<F>, false, is_variable, checked, boost, clean_stack, C> {
			typedef lua_bind_traits<F> traits_type;
			typedef wrapper<meta::unqualified_t<F>> wrap;
			typedef typename wrap::object_type object_type;

			template <typename V>
			static int call(lua_State* L, V&&) {
				return luaL_error(L, "sol: cannot write to a sol::readonly variable");
			}

			template <typename V>
			static int call(lua_State* L, V&&, object_type&) {
				return luaL_error(L, "sol: cannot write to a sol::readonly variable");
			}
		};

		template <typename T, typename F, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, readonly_wrapper<F>, true, is_variable, checked, boost, clean_stack, C> : lua_call_wrapper<T, F, true, is_variable, checked, boost, clean_stack, C> {
		};

		template <typename T, typename... Args, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, constructor_list<Args...>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef constructor_list<Args...> F;

			static int call(lua_State* L, F&) {
				const auto& meta = usertype_traits<T>::metatable();
				int argcount = lua_gettop(L);
				call_syntax syntax = argcount > 0 ? stack::get_call_syntax(L, usertype_traits<T>::user_metatable(), 1) : call_syntax::dot;
				argcount -= static_cast<int>(syntax);

				T* obj = detail::usertype_allocate<T>(L);
				reference userdataref(L, -1);

				construct_match<T, Args...>(constructor_match<T, false, clean_stack>(obj), L, argcount, boost + 1 + static_cast<int>(syntax));

				userdataref.push();
				stack::stack_detail::undefined_metatable<T> umf(L, &meta[0]);
				umf();

				return 1;
			}
		};

		template <typename T, typename... Cxs, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, constructor_wrapper<Cxs...>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef constructor_wrapper<Cxs...> F;

			struct onmatch {
				template <typename Fx, std::size_t I, typename... R, typename... Args>
				int operator()(types<Fx>, index_value<I>, types<R...> r, types<Args...> a, lua_State* L, int, int start, F& f) {
					const auto& meta = usertype_traits<T>::metatable();
					T* obj = detail::usertype_allocate<T>(L);
					reference userdataref(L, -1);
					
					auto& func = std::get<I>(f.functions);
					stack::call_into_lua<checked, clean_stack>(r, a, L, boost + start, func, detail::implicit_wrapper<T>(obj));

					userdataref.push();
					stack::stack_detail::undefined_metatable<T> umf(L, &meta[0]);
					umf();

					return 1;
				}
			};

			static int call(lua_State* L, F& f) {
				call_syntax syntax = stack::get_call_syntax(L, usertype_traits<T>::user_metatable(), 1);
				int syntaxval = static_cast<int>(syntax);
				int argcount = lua_gettop(L) - syntaxval;
				return construct_match<T, meta::pop_front_type_t<meta::function_args_t<Cxs>>...>(onmatch(), L, argcount, 1 + syntaxval, f);
			}
		};

		template <typename T, typename Fx, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack>
		struct lua_call_wrapper<T, destructor_wrapper<Fx>, is_index, is_variable, checked, boost, clean_stack, std::enable_if_t<std::is_void<Fx>::value>> {
			typedef destructor_wrapper<Fx> F;

			static int call(lua_State* L, const F&) {
				return detail::usertype_alloc_destruct<T>(L);
			}
		};

		template <typename T, typename Fx, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack>
		struct lua_call_wrapper<T, destructor_wrapper<Fx>, is_index, is_variable, checked, boost, clean_stack, std::enable_if_t<!std::is_void<Fx>::value>> {
			typedef destructor_wrapper<Fx> F;

			static int call_void(std::true_type, lua_State* L, const F& f) {
				typedef meta::bind_traits<meta::unqualified_t<decltype(f.fx)>> bt;
				typedef typename bt::template arg_at<0> arg0;
				typedef meta::unqualified_t<arg0> O;

				O& obj = stack::get<O>(L);
				f.fx(detail::implicit_wrapper<O>(obj));
				return 0;
			}

			static int call_void(std::false_type, lua_State* L, const F& f) {
				T& obj = stack::get<T>(L);
				f.fx(detail::implicit_wrapper<T>(obj));
				return 0;
			}

			static int call(lua_State* L, const F& f) {
				return call_void(std::is_void<T>(), L, f);
			}
		};

		template <typename T, typename... Fs, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, overload_set<Fs...>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef overload_set<Fs...> F;

			struct on_match {
				template <typename Fx, std::size_t I, typename... R, typename... Args>
				int operator()(types<Fx>, index_value<I>, types<R...>, types<Args...>, lua_State* L, int, int, F& fx) {
					auto& f = std::get<I>(fx.functions);
					return lua_call_wrapper<T, Fx, is_index, is_variable, checked, boost>{}.call(L, f);
				}
			};

			static int call(lua_State* L, F& fx) {
				return overload_match_arity<Fs...>(on_match(), L, lua_gettop(L), 1, fx);
			}
		};

		template <typename T, typename... Fs, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, factory_wrapper<Fs...>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef factory_wrapper<Fs...> F;

			struct on_match {
				template <typename Fx, std::size_t I, typename... R, typename... Args>
				int operator()(types<Fx>, index_value<I>, types<R...>, types<Args...>, lua_State* L, int, int, F& fx) {
					auto& f = std::get<I>(fx.functions);
					return lua_call_wrapper<T, Fx, is_index, is_variable, checked, boost, clean_stack>{}.call(L, f);
				}
			};

			static int call(lua_State* L, F& fx) {
				return overload_match_arity<Fs...>(on_match(), L, lua_gettop(L) - boost, 1 + boost, fx);
			}
		};

		template <typename T, typename R, typename W, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, property_wrapper<R, W>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef std::conditional_t<is_index, R, W> P;
			typedef meta::unqualified_t<P> U;
			typedef wrapper<U> wrap;
			typedef lua_bind_traits<U> traits_type;
			typedef meta::unqualified_t<typename traits_type::template arg_at<0>> object_type;

			template <typename F>
			static int self_call(std::true_type, lua_State* L, F&& f) {
				// The type being void means we don't have any arguments, so it might be a free functions?
				typedef typename traits_type::free_args_list args_list;
				typedef typename wrap::returns_list returns_list;
				typedef typename wrap::caller caller;
				return stack::call_into_lua<checked, clean_stack>(returns_list(), args_list(), L, boost + (is_variable ? 3 : 2), caller(), f);
			}

			template <typename F>
			static int self_call(std::false_type, lua_State* L, F&& f) {
				typedef meta::pop_front_type_t<typename traits_type::free_args_list> args_list;
				typedef T Ta;
				typedef std::remove_pointer_t<object_type> Oa;
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeo = stack::check_get<Ta*>(L, 1);
				if (!maybeo || maybeo.value() == nullptr) {
					if (is_variable) {
						return luaL_error(L, "sol: 'self' argument is lua_nil (bad '.' access?)");
					}
					return luaL_error(L, "sol: 'self' argument is lua_nil (pass 'self' as first argument)");
				}
				Oa* o = static_cast<Oa*>(maybeo.value());
#else
				Oa* o = static_cast<Oa*>(stack::get<non_null<Ta*>>(L, 1));
#endif // Safety
				typedef typename wrap::returns_list returns_list;
				typedef typename wrap::caller caller;
				return stack::call_into_lua<checked, clean_stack>(returns_list(), args_list(), L, boost + (is_variable ? 3 : 2), caller(), f, detail::implicit_wrapper<Oa>(*o));
			}

			template <typename F, typename... Args>
			static int defer_call(std::false_type, lua_State* L, F&& f, Args&&... args) {
				return self_call(meta::any<std::is_void<object_type>, meta::boolean<lua_type_of<meta::unwrap_unqualified_t<object_type>>::value != type::userdata>>(), L, pick(meta::boolean<is_index>(), f), std::forward<Args>(args)...);
			}

			template <typename F, typename... Args>
			static int defer_call(std::true_type, lua_State* L, F&& f, Args&&... args) {
				auto& p = pick(meta::boolean<is_index>(), std::forward<F>(f));
				return lua_call_wrapper<T, meta::unqualified_t<decltype(p)>, is_index, is_variable, checked, boost, clean_stack>{}.call(L, p, std::forward<Args>(args)...);
			}

			template <typename F, typename... Args>
			static int call(lua_State* L, F&& f, Args&&... args) {
				typedef meta::any<
					std::is_void<U>,
					std::is_same<U, no_prop>,
					meta::is_specialization_of<U, var_wrapper>,
					meta::is_specialization_of<U, constructor_wrapper>,
					meta::is_specialization_of<U, constructor_list>,
					std::is_member_pointer<U>>
					is_specialized;
				return defer_call(is_specialized(), L, std::forward<F>(f), std::forward<Args>(args)...);
			}
		};

		template <typename T, typename V, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, protect_t<V>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef protect_t<V> F;

			template <typename... Args>
			static int call(lua_State* L, F& fx, Args&&... args) {
				return lua_call_wrapper<T, V, is_index, is_variable, true, boost, clean_stack>{}.call(L, fx.value, std::forward<Args>(args)...);
			}
		};

		template <typename T, typename F, typename... Filters, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, filter_wrapper<F, Filters...>, is_index, is_variable, checked, boost, clean_stack, C> {
			typedef filter_wrapper<F, Filters...> P;

			template <std::size_t... In>
			static int call(std::index_sequence<In...>, lua_State* L, P& fx) {
				int pushed = lua_call_wrapper<T, F, is_index, is_variable, checked, boost, false, C>{}.call(L, fx.value);
				(void)detail::swallow{ int(), (filter_detail::handle_filter(std::get<In>(fx.filters), L, pushed), int())... };
				return pushed;
			}

			static int call(lua_State* L, P& fx) {
				typedef typename P::indices indices;
				return call(indices(), L, fx);
			}
		};

		template <typename T, typename Sig, typename P, bool is_index, bool is_variable, bool checked, int boost, bool clean_stack, typename C>
		struct lua_call_wrapper<T, function_arguments<Sig, P>, is_index, is_variable, checked, boost, clean_stack, C> {
			template <typename F>
			static int call(lua_State* L, F&& f) {
				return lua_call_wrapper<T, meta::unqualified_t<P>, is_index, is_variable, checked, boost, clean_stack>{}.call(L, std::get<0>(f.arguments));
			}
		};

		template <typename T, bool is_index, bool is_variable, int boost = 0, bool checked = detail::default_safe_function_calls, bool clean_stack = true, typename Fx, typename... Args>
		inline int call_wrapped(lua_State* L, Fx&& fx, Args&&... args) {
			return lua_call_wrapper<T, meta::unqualified_t<Fx>, is_index, is_variable, checked, boost, clean_stack>{}.call(L, std::forward<Fx>(fx), std::forward<Args>(args)...);
		}

		template <typename T, bool is_index, bool is_variable, typename F, int start = 1, bool checked = detail::default_safe_function_calls, bool clean_stack = true>
		inline int call_user(lua_State* L) {
			auto& fx = stack::unqualified_get<user<F>>(L, upvalue_index(start));
			return call_wrapped<T, is_index, is_variable, 0, checked, clean_stack>(L, fx);
		}

		template <typename T, typename = void>
		struct is_var_bind : std::false_type {};

		template <typename T>
		struct is_var_bind<T, std::enable_if_t<std::is_member_object_pointer<T>::value>> : std::true_type {};

		template <>
		struct is_var_bind<no_prop> : std::true_type {};

		template <typename R, typename W>
		struct is_var_bind<property_wrapper<R, W>> : std::true_type {};

		template <typename T>
		struct is_var_bind<var_wrapper<T>> : std::true_type {};

		template <typename T>
		struct is_var_bind<readonly_wrapper<T>> : is_var_bind<meta::unqualified_t<T>> {};

		template <typename F, typename... Filters>
		struct is_var_bind<filter_wrapper<F, Filters...>> : is_var_bind<meta::unqualified_t<F>> {};
	} // namespace call_detail

	template <typename T>
	struct is_variable_binding : call_detail::is_var_bind<meta::unqualified_t<T>> {};

	template <typename T>
	struct is_function_binding : meta::neg<is_variable_binding<T>> {};

} // namespace sol

#endif // SOL_CALL_HPP
