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

#ifndef SOL_FUNCTION_TYPES_HPP
#define SOL_FUNCTION_TYPES_HPP

#include "function_types_core.hpp"
#include "function_types_templated.hpp"
#include "function_types_stateless.hpp"
#include "function_types_stateful.hpp"
#include "function_types_overloaded.hpp"
#include "resolve.hpp"
#include "call.hpp"

namespace sol {
	namespace function_detail {
		template <typename T>
		struct class_indicator {};

		struct call_indicator {};
	} // namespace function_detail

	namespace stack {
		template <typename... Sigs>
		struct pusher<function_sig<Sigs...>> {
			template <bool is_yielding, typename... Sig, typename Fx, typename... Args>
			static void select_convertible(std::false_type, types<Sig...>, lua_State* L, Fx&& fx, Args&&... args) {
				typedef std::remove_pointer_t<std::decay_t<Fx>> clean_fx;
				typedef function_detail::functor_function<clean_fx, is_yielding, true> F;
				set_fx<false, F>(L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename R, typename... A, typename Fx, typename... Args>
			static void select_convertible(std::true_type, types<R(A...)>, lua_State* L, Fx&& fx, Args&&... args) {
				using fx_ptr_t = R (*)(A...);
				fx_ptr_t fxptr = detail::unwrap(std::forward<Fx>(fx));
				select_function<is_yielding>(std::true_type(), L, fxptr, std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename R, typename... A, typename Fx, typename... Args>
			static void select_convertible(types<R(A...)> t, lua_State* L, Fx&& fx, Args&&... args) {
				typedef std::decay_t<meta::unwrap_unqualified_t<Fx>> raw_fx_t;
				typedef R (*fx_ptr_t)(A...);
				typedef std::is_convertible<raw_fx_t, fx_ptr_t> is_convertible;
				select_convertible<is_yielding>(is_convertible(), t, L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void select_convertible(types<>, lua_State* L, Fx&& fx, Args&&... args) {
				typedef meta::function_signature_t<meta::unwrap_unqualified_t<Fx>> Sig;
				select_convertible<is_yielding>(types<Sig>(), L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args>
			static void select_reference_member_variable(std::false_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef std::remove_pointer_t<std::decay_t<Fx>> clean_fx;
				typedef function_detail::member_variable<meta::unwrap_unqualified_t<T>, clean_fx, is_yielding> F;
				set_fx<false, F>(L, std::forward<Fx>(fx), std::forward<T>(obj), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args>
			static void select_reference_member_variable(std::true_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef std::decay_t<Fx> dFx;
				dFx memfxptr(std::forward<Fx>(fx));
				auto userptr = detail::ptr(std::forward<T>(obj), std::forward<Args>(args)...);
				lua_CFunction freefunc = &function_detail::upvalue_member_variable<std::decay_t<decltype(*userptr)>, meta::unqualified_t<Fx>, is_yielding>::call;

				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, memfxptr);
				upvalues += stack::push(L, lightuserdata_value(static_cast<void*>(userptr)));
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void select_member_variable(std::false_type, lua_State* L, Fx&& fx, Args&&... args) {
				select_convertible<is_yielding>(types<Sigs...>(), L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args, meta::disable<meta::is_specialization_of<meta::unqualified_t<T>, function_detail::class_indicator>> = meta::enabler>
			static void select_member_variable(std::true_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef meta::boolean<meta::is_specialization_of<meta::unqualified_t<T>, std::reference_wrapper>::value || std::is_pointer<T>::value> is_reference;
				select_reference_member_variable<is_yielding>(is_reference(), L, std::forward<Fx>(fx), std::forward<T>(obj), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename C>
			static void select_member_variable(std::true_type, lua_State* L, Fx&& fx, function_detail::class_indicator<C>) {
				lua_CFunction freefunc = &function_detail::upvalue_this_member_variable<C, Fx, is_yielding>::call;
				
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, fx);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx>
			static void select_member_variable(std::true_type, lua_State* L, Fx&& fx) {
				typedef typename meta::bind_traits<meta::unqualified_t<Fx>>::object_type C;
				lua_CFunction freefunc = &function_detail::upvalue_this_member_variable<C, Fx, is_yielding>::call;
				
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, fx);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args>
			static void select_reference_member_function(std::false_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef std::decay_t<Fx> clean_fx;
				typedef function_detail::member_function<meta::unwrap_unqualified_t<T>, clean_fx, is_yielding> F;
				set_fx<false, F>(L, std::forward<Fx>(fx), std::forward<T>(obj), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args>
			static void select_reference_member_function(std::true_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef std::decay_t<Fx> dFx;
				dFx memfxptr(std::forward<Fx>(fx));
				auto userptr = detail::ptr(std::forward<T>(obj), std::forward<Args>(args)...);
				lua_CFunction freefunc = &function_detail::upvalue_member_function<std::decay_t<decltype(*userptr)>, meta::unqualified_t<Fx>, is_yielding>::call;

				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, memfxptr);
				upvalues += stack::push(L, lightuserdata_value(static_cast<void*>(userptr)));
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void select_member_function(std::false_type, lua_State* L, Fx&& fx, Args&&... args) {
				select_member_variable<is_yielding>(meta::is_member_object<meta::unqualified_t<Fx>>(), L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename T, typename... Args, meta::disable<meta::is_specialization_of<meta::unqualified_t<T>, function_detail::class_indicator>> = meta::enabler>
			static void select_member_function(std::true_type, lua_State* L, Fx&& fx, T&& obj, Args&&... args) {
				typedef meta::boolean<meta::is_specialization_of<meta::unqualified_t<T>, std::reference_wrapper>::value || std::is_pointer<T>::value> is_reference;
				select_reference_member_function<is_yielding>(is_reference(), L, std::forward<Fx>(fx), std::forward<T>(obj), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename C>
			static void select_member_function(std::true_type, lua_State* L, Fx&& fx, function_detail::class_indicator<C>) {
				lua_CFunction freefunc = &function_detail::upvalue_this_member_function<C, Fx, is_yielding>::call;
				
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, fx);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx>
			static void select_member_function(std::true_type, lua_State* L, Fx&& fx) {
				typedef typename meta::bind_traits<meta::unqualified_t<Fx>>::object_type C;
				lua_CFunction freefunc = &function_detail::upvalue_this_member_function<C, Fx, is_yielding>::call;
				
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, fx);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void select_function(std::false_type, lua_State* L, Fx&& fx, Args&&... args) {
				select_member_function<is_yielding>(std::is_member_function_pointer<meta::unqualified_t<Fx>>(), L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void select_function(std::true_type, lua_State* L, Fx&& fx, Args&&... args) {
				std::decay_t<Fx> target(std::forward<Fx>(fx), std::forward<Args>(args)...);
				lua_CFunction freefunc = &function_detail::upvalue_free_function<Fx, is_yielding>::call;

				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::stack_detail::push_as_upvalues(L, target);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <bool is_yielding>
			static void select_function(std::true_type, lua_State* L, lua_CFunction f) {
				// TODO: support yielding
				stack::push(L, f);
			}

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
			template <bool is_yielding>
			static void select_function(std::true_type, lua_State* L, detail::lua_CFunction_noexcept f) {
				// TODO: support yielding
				stack::push(L, f);
			}
#endif // noexcept function type

			template <bool is_yielding, typename Fx, typename... Args, meta::disable<is_lua_reference<meta::unqualified_t<Fx>>> = meta::enabler>
			static void select(lua_State* L, Fx&& fx, Args&&... args) {
				select_function<is_yielding>(std::is_function<std::remove_pointer_t<meta::unqualified_t<Fx>>>(), L, std::forward<Fx>(fx), std::forward<Args>(args)...);
			}

			template <bool is_yielding, typename Fx, meta::enable<is_lua_reference<meta::unqualified_t<Fx>>> = meta::enabler>
			static void select(lua_State* L, Fx&& fx) {
				// TODO: hoist into lambda in this case??
				stack::push(L, std::forward<Fx>(fx));
			}

			template <bool is_yielding, typename Fx, typename... Args>
			static void set_fx(lua_State* L, Args&&... args) {
				lua_CFunction freefunc = detail::static_trampoline<function_detail::call<meta::unqualified_t<Fx>, 2, is_yielding>>;

				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<Fx>>(L, std::forward<Args>(args)...);
				stack::push(L, c_closure(freefunc, upvalues));
			}

			template <typename Arg0, typename... Args, meta::disable<std::is_same<detail::yield_tag_t, meta::unqualified_t<Arg0>>> = meta::enabler>
			static int push(lua_State* L, Arg0&& arg0, Args&&... args) {
				// Set will always place one thing (function) on the stack
				select<false>(L, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
				return 1;
			}

			template <typename... Args>
			static int push(lua_State* L, detail::yield_tag_t, Args&&... args) {
				// Set will always place one thing (function) on the stack
				select<true>(L, std::forward<Args>(args)...);
				return 1;
			}
		};

		template <typename T>
		struct pusher<yielding_t<T>> {
			template <typename... Args>
			static int push(lua_State* L, const yielding_t<T>& f, Args&&... args) {
				pusher<function_sig<>> p{};
				(void)p;
				return p.push(L, detail::yield_tag, f.func, std::forward<Args>(args)...);
			}

			template <typename... Args>
			static int push(lua_State* L, yielding_t<T>&& f, Args&&... args) {
				pusher<function_sig<>> p{};
				(void)p;
				return p.push(L, detail::yield_tag, f.func, std::forward<Args>(args)...);
			}
		};

		template <typename T, typename... Args>
		struct pusher<function_arguments<T, Args...>> {
			template <std::size_t... I, typename FP>
			static int push_func(std::index_sequence<I...>, lua_State* L, FP&& fp) {
				return stack::push<T>(L, detail::forward_get<I>(fp.arguments)...);
			}

			static int push(lua_State* L, const function_arguments<T, Args...>& fp) {
				return push_func(std::make_index_sequence<sizeof...(Args)>(), L, fp);
			}

			static int push(lua_State* L, function_arguments<T, Args...>&& fp) {
				return push_func(std::make_index_sequence<sizeof...(Args)>(), L, std::move(fp));
			}
		};

		template <typename Signature>
		struct pusher<std::function<Signature>> {
			static int push(lua_State* L, const std::function<Signature>& fx) {
				return pusher<function_sig<Signature>>{}.push(L, fx);
			}

			static int push(lua_State* L, std::function<Signature>&& fx) {
				return pusher<function_sig<Signature>>{}.push(L, std::move(fx));
			}
		};

		template <typename Signature>
		struct pusher<Signature, std::enable_if_t<std::is_member_pointer<Signature>::value>> {
			template <typename F, typename... Args>
			static int push(lua_State* L, F&& f, Args&&... args) {
				pusher<function_sig<>> p{};
				(void)p;
				return p.push(L, std::forward<F>(f), std::forward<Args>(args)...);
			}
		};

		template <typename Signature>
		struct pusher<Signature, std::enable_if_t<meta::all<std::is_function<std::remove_pointer_t<Signature>>, meta::neg<std::is_same<Signature, lua_CFunction>>, meta::neg<std::is_same<Signature, std::remove_pointer_t<lua_CFunction>>>
#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
			,
								meta::neg<std::is_same<Signature, detail::lua_CFunction_noexcept>>, meta::neg<std::is_same<Signature, std::remove_pointer_t<detail::lua_CFunction_noexcept>>>
#endif // noexcept function types
								>::value>> {
			template <typename F>
			static int push(lua_State* L, F&& f) {
				return pusher<function_sig<>>{}.push(L, std::forward<F>(f));
			}
		};

		template <typename... Functions>
		struct pusher<overload_set<Functions...>> {
			static int push(lua_State* L, overload_set<Functions...>&& set) {
				// TODO: yielding
				typedef function_detail::overloaded_function<0, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, std::move(set.functions));
				return 1;
			}

			static int push(lua_State* L, const overload_set<Functions...>& set) {
				// TODO: yielding
				typedef function_detail::overloaded_function<0, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, set.functions);
				return 1;
			}
		};

		template <typename T>
		struct pusher<protect_t<T>> {
			static int push(lua_State* L, protect_t<T>&& pw) {
				lua_CFunction cf = call_detail::call_user<void, false, false, protect_t<T>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<protect_t<T>>>(L, std::move(pw.value));
				return stack::push(L, c_closure(cf, upvalues));
			}

			static int push(lua_State* L, const protect_t<T>& pw) {
				lua_CFunction cf = call_detail::call_user<void, false, false, protect_t<T>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<protect_t<T>>>(L, pw.value);
				return stack::push(L, c_closure(cf, upvalues));
			}
		};

		template <typename F, typename G>
		struct pusher<property_wrapper<F, G>, std::enable_if_t<!std::is_void<F>::value && !std::is_void<G>::value>> {
			static int push(lua_State* L, property_wrapper<F, G>&& pw) {
				return stack::push(L, overload(std::move(pw.read), std::move(pw.write)));
			}
			static int push(lua_State* L, const property_wrapper<F, G>& pw) {
				return stack::push(L, overload(pw.read, pw.write));
			}
		};

		template <typename F>
		struct pusher<property_wrapper<F, void>> {
			static int push(lua_State* L, property_wrapper<F, void>&& pw) {
				return stack::push(L, std::move(pw.read));
			}
			static int push(lua_State* L, const property_wrapper<F, void>& pw) {
				return stack::push(L, pw.read);
			}
		};

		template <typename F>
		struct pusher<property_wrapper<void, F>> {
			static int push(lua_State* L, property_wrapper<void, F>&& pw) {
				return stack::push(L, std::move(pw.write));
			}
			static int push(lua_State* L, const property_wrapper<void, F>& pw) {
				return stack::push(L, pw.write);
			}
		};

		template <typename T>
		struct pusher<var_wrapper<T>> {
			static int push(lua_State* L, var_wrapper<T>&& vw) {
				return stack::push(L, std::move(vw.value));
			}
			static int push(lua_State* L, const var_wrapper<T>& vw) {
				return stack::push(L, vw.value);
			}
		};

		template <typename... Functions>
		struct pusher<factory_wrapper<Functions...>> {
			static int push(lua_State* L, const factory_wrapper<Functions...>& fw) {
				typedef function_detail::overloaded_function<0, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, fw.functions);
				return 1;
			}

			static int push(lua_State* L, factory_wrapper<Functions...>&& fw) {
				typedef function_detail::overloaded_function<0, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, std::move(fw.functions));
				return 1;
			}

			static int push(lua_State* L, const factory_wrapper<Functions...>& set, function_detail::call_indicator) {
				typedef function_detail::overloaded_function<1, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, set.functions);
				return 1;
			}

			static int push(lua_State* L, factory_wrapper<Functions...>&& set, function_detail::call_indicator) {
				typedef function_detail::overloaded_function<1, Functions...> F;
				pusher<function_sig<>>{}.set_fx<false, F>(L, std::move(set.functions));
				return 1;
			}
		};

		template <>
		struct pusher<no_construction> {
			static int push(lua_State* L, no_construction) {
				lua_CFunction cf = &function_detail::no_construction_error;
				return stack::push(L, cf);
			}

			static int push(lua_State* L, no_construction c, function_detail::call_indicator) {
				return push(L, c);
			}
		};

		template <typename T, typename... Lists>
		struct pusher<detail::tagged<T, constructor_list<Lists...>>> {
			static int push(lua_State* L, detail::tagged<T, constructor_list<Lists...>>) {
				lua_CFunction cf = call_detail::construct<T, detail::default_safe_function_calls, true, Lists...>;
				return stack::push(L, cf);
			}

			static int push(lua_State* L, constructor_list<Lists...>) {
				lua_CFunction cf = call_detail::construct<T, detail::default_safe_function_calls, true, Lists...>;
				return stack::push(L, cf);
			}
		};

		template <typename L0, typename... Lists>
		struct pusher<constructor_list<L0, Lists...>> {
			typedef constructor_list<L0, Lists...> cl_t;
			static int push(lua_State* L, cl_t cl) {
				typedef typename meta::bind_traits<L0>::return_type T;
				return stack::push<detail::tagged<T, cl_t>>(L, cl);
			}
		};

		template <typename T, typename... Fxs>
		struct pusher<detail::tagged<T, constructor_wrapper<Fxs...>>> {
			template <typename C>
			static int push(lua_State* L, C&& c) {
				lua_CFunction cf = call_detail::call_user<T, false, false, constructor_wrapper<Fxs...>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<constructor_wrapper<Fxs...>>>(L, std::forward<C>(c));
				return stack::push(L, c_closure(cf, upvalues));
			}
		};

		template <typename F, typename... Fxs>
		struct pusher<constructor_wrapper<F, Fxs...>> {
			template <typename C>
			static int push(lua_State* L, C&& c) {
				typedef typename meta::bind_traits<F>::template arg_at<0> arg0;
				typedef meta::unqualified_t<std::remove_pointer_t<arg0>> T;
				return stack::push<detail::tagged<T, constructor_wrapper<F, Fxs...>>>(L, std::forward<C>(c));
			}
		};

		template <typename T>
		struct pusher<detail::tagged<T, destructor_wrapper<void>>> {
			static int push(lua_State* L, destructor_wrapper<void>) {
				lua_CFunction cf = detail::usertype_alloc_destruct<T>;
				return stack::push(L, cf);
			}
		};

		template <typename T, typename Fx>
		struct pusher<detail::tagged<T, destructor_wrapper<Fx>>> {
			static int push(lua_State* L, destructor_wrapper<Fx>&& c) {
				lua_CFunction cf = call_detail::call_user<T, false, false, destructor_wrapper<Fx>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<destructor_wrapper<Fx>>>(L, std::move(c));
				return stack::push(L, c_closure(cf, upvalues));
			}

			static int push(lua_State* L, const destructor_wrapper<Fx>& c) {
				lua_CFunction cf = call_detail::call_user<T, false, false, destructor_wrapper<Fx>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<destructor_wrapper<Fx>>>(L, c);
				return stack::push(L, c_closure(cf, upvalues));
			}
		};

		template <typename Fx>
		struct pusher<destructor_wrapper<Fx>> {
			static int push(lua_State* L, destructor_wrapper<Fx>&& c) {
				lua_CFunction cf = call_detail::call_user<void, false, false, destructor_wrapper<Fx>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<destructor_wrapper<Fx>>>(L, std::move(c));
				return stack::push(L, c_closure(cf, upvalues));
			}

			static int push(lua_State* L, const destructor_wrapper<Fx>& c) {
				lua_CFunction cf = call_detail::call_user<void, false, false, destructor_wrapper<Fx>, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<destructor_wrapper<Fx>>>(L, c);
				return stack::push(L, c_closure(cf, upvalues));
			}
		};

		template <typename F, typename... Filters>
		struct pusher<filter_wrapper<F, Filters...>> {
			typedef filter_wrapper<F, Filters...> P;

			static int push(lua_State* L, const P& p) {
				lua_CFunction cf = call_detail::call_user<void, false, false, P, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<P>>(L, p);
				return stack::push(L, c_closure(cf, upvalues));
			}

			static int push(lua_State* L, P&& p) {
				lua_CFunction cf = call_detail::call_user<void, false, false, P, 2>;
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push<user<P>>(L, std::move(p));
				return stack::push(L, c_closure(cf, upvalues));
			}
		};
	} // namespace stack
} // namespace sol

#endif // SOL_FUNCTION_TYPES_HPP
