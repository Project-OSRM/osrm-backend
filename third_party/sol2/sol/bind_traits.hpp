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

#ifndef SOL_BIND_TRAITS_HPP
#define SOL_BIND_TRAITS_HPP

#include "feature_test.hpp"
#include "tuple.hpp"

namespace sol {
namespace meta {
	namespace meta_detail {

		template <class F>
		struct check_deducible_signature {
			struct nat {};
			template <class G>
			static auto test(int) -> decltype(&G::operator(), void());
			template <class>
			static auto test(...) -> nat;

			using type = std::is_void<decltype(test<F>(0))>;
		};
	} // namespace meta_detail

	template <class F>
	struct has_deducible_signature : meta_detail::check_deducible_signature<F>::type {};

	namespace meta_detail {

		template <std::size_t I, typename T>
		struct void_tuple_element : meta::tuple_element<I, T> {};

		template <std::size_t I>
		struct void_tuple_element<I, std::tuple<>> { typedef void type; };

		template <std::size_t I, typename T>
		using void_tuple_element_t = typename void_tuple_element<I, T>::type;

		template <bool it_is_noexcept, bool has_c_variadic, typename T, typename R, typename... Args>
		struct basic_traits {
		private:
			typedef std::conditional_t<std::is_void<T>::value, int, T>& first_type;

		public:
			static const bool is_noexcept = it_is_noexcept;
			static const bool is_member_function = std::is_void<T>::value;
			static const bool has_c_var_arg = has_c_variadic;
			static const std::size_t arity = sizeof...(Args);
			static const std::size_t free_arity = sizeof...(Args) + static_cast<std::size_t>(!std::is_void<T>::value);
			typedef types<Args...> args_list;
			typedef std::tuple<Args...> args_tuple;
			typedef T object_type;
			typedef R return_type;
			typedef tuple_types<R> returns_list;
			typedef R(function_type)(Args...);
			typedef std::conditional_t<std::is_void<T>::value, args_list, types<first_type, Args...>> free_args_list;
			typedef std::conditional_t<std::is_void<T>::value, R(Args...), R(first_type, Args...)> free_function_type;
			typedef std::conditional_t<std::is_void<T>::value, R (*)(Args...), R (*)(first_type, Args...)> free_function_pointer_type;
			typedef std::remove_pointer_t<free_function_pointer_type> signature_type;
			template <std::size_t i>
			using arg_at = void_tuple_element_t<i, args_tuple>;
		};

		template <typename Signature, bool b = has_deducible_signature<Signature>::value>
		struct fx_traits : basic_traits<false, false, void, void> {};

		// Free Functions
		template <typename R, typename... Args>
		struct fx_traits<R(Args...), false> : basic_traits<false, false, void, R, Args...> {
			typedef R (*function_pointer_type)(Args...);
		};

		template <typename R, typename... Args>
		struct fx_traits<R (*)(Args...), false> : basic_traits<false, false, void, R, Args...> {
			typedef R (*function_pointer_type)(Args...);
		};

		template <typename R, typename... Args>
		struct fx_traits<R(Args..., ...), false> : basic_traits<false, true, void, R, Args...> {
			typedef R (*function_pointer_type)(Args..., ...);
		};

		template <typename R, typename... Args>
		struct fx_traits<R (*)(Args..., ...), false> : basic_traits<false, true, void, R, Args...> {
			typedef R (*function_pointer_type)(Args..., ...);
		};

		// Member Functions
		/* C-Style Variadics */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...), false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...);
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...), false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...);
		};

		/* Const Volatile */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile;
		};

		/* Member Function Qualifiers */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...)&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) &;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...)&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) &;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...)&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) &&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...)&&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) &&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const&&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const&&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const&&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile&&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile&&, false> : basic_traits<false, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile&&;
		};

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE

		template <typename R, typename... Args>
		struct fx_traits<R(Args...) noexcept, false> : basic_traits<true, false, void, R, Args...> {
			typedef R (*function_pointer_type)(Args...) noexcept;
		};

		template <typename R, typename... Args>
		struct fx_traits<R (*)(Args...) noexcept, false> : basic_traits<true, false, void, R, Args...> {
			typedef R (*function_pointer_type)(Args...) noexcept;
		};

		template <typename R, typename... Args>
		struct fx_traits<R(Args..., ...) noexcept, false> : basic_traits<true, true, void, R, Args...> {
			typedef R (*function_pointer_type)(Args..., ...) noexcept;
		};

		template <typename R, typename... Args>
		struct fx_traits<R (*)(Args..., ...) noexcept, false> : basic_traits<true, true, void, R, Args...> {
			typedef R (*function_pointer_type)(Args..., ...) noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) noexcept;
		};

		/* Const Volatile */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) & noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) & noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) & noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) & noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) && noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) && noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) && noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) && noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const&& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const&& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const&& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const&& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args...) const volatile&& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args...) const volatile&& noexcept;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (T::*)(Args..., ...) const volatile&& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (T::*function_pointer_type)(Args..., ...) const volatile&& noexcept;
		};

#endif // noexcept is part of a function's type

#if defined(_MSC_VER) && defined(_M_IX86)
		template <typename R, typename... Args>
		struct fx_traits<R __stdcall(Args...), false> : basic_traits<false, false, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args...);
		};

		template <typename R, typename... Args>
		struct fx_traits<R(__stdcall*)(Args...), false> : basic_traits<false, false, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args...);
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...), false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...);
		};

		/* Const Volatile */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile;
		};

		/* Member Function Qualifiers */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...)&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) &;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...)&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) &&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const&&;
		};

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile&&, false> : basic_traits<false, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile&&;
		};

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE

		template <typename R, typename... Args>
		struct fx_traits<R __stdcall(Args...) noexcept, false> : basic_traits<true, false, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args...) noexcept;
		};

		template <typename R, typename... Args>
		struct fx_traits<R (__stdcall *)(Args...) noexcept, false> : basic_traits<true, false, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args...) noexcept;
		};

		/* __stdcall cannot be applied to functions with varargs*/
		/*template <typename R, typename... Args>
		struct fx_traits<__stdcall R(Args..., ...) noexcept, false> : basic_traits<true, true, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args..., ...) noexcept;
		};

		template <typename R, typename... Args>
		struct fx_traits<R (__stdcall *)(Args..., ...) noexcept, false> : basic_traits<true, true, void, R, Args...> {
			typedef R(__stdcall* function_pointer_type)(Args..., ...) noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) noexcept;
		};*/

		/* Const Volatile */
		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const volatile noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const volatile noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) & noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) & noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) & noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) & noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const& noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const& noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile& noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const volatile& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const volatile& noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) && noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) && noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) && noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) && noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const&& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const&& noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const&& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const&& noexcept;
		};*/

		template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args...) const volatile&& noexcept, false> : basic_traits<true, false, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args...) const volatile&& noexcept;
		};

		/* __stdcall does not work with varargs */
		/*template <typename T, typename R, typename... Args>
		struct fx_traits<R (__stdcall T::*)(Args..., ...) const volatile&& noexcept, false> : basic_traits<true, true, T, R, Args...> {
			typedef R (__stdcall T::*function_pointer_type)(Args..., ...) const volatile&& noexcept;
		};*/
#endif // noexcept is part of a function's type
#endif // __stdcall x86 VC++ bug

		template <typename Signature>
		struct fx_traits<Signature, true> : fx_traits<typename fx_traits<decltype(&Signature::operator())>::function_type, false> {};

		template <typename Signature, bool b = std::is_member_object_pointer<Signature>::value>
		struct callable_traits : fx_traits<std::decay_t<Signature>> {
		};

		template <typename R, typename T>
		struct callable_traits<R(T::*), true> {
			typedef std::conditional_t<std::is_array<R>::value, std::add_lvalue_reference_t<T>, R> return_type;
			typedef return_type Arg;
			typedef T object_type;
			using signature_type = R(T::*);
			static const bool is_noexcept = false;
			static const bool is_member_function = false;
			static const std::size_t arity = 1;
			static const std::size_t free_arity = 2;
			typedef std::tuple<Arg> args_tuple;
			typedef types<Arg> args_list;
			typedef types<T, Arg> free_args_list;
			typedef meta::tuple_types<return_type> returns_list;
			typedef return_type(function_type)(T&, return_type);
			typedef return_type(*function_pointer_type)(T&, Arg);
			typedef return_type(*free_function_pointer_type)(T&, Arg);
			template <std::size_t i>
			using arg_at = void_tuple_element_t<i, args_tuple>;
		};

	} // namespace meta_detail

	template <typename Signature>
	struct bind_traits : meta_detail::callable_traits<Signature> {};

	template <typename Signature>
	using function_args_t = typename bind_traits<Signature>::args_list;

	template <typename Signature>
	using function_signature_t = typename bind_traits<Signature>::signature_type;

	template <typename Signature>
	using function_return_t = typename bind_traits<Signature>::return_type;
}
} // namespace sol::meta

#endif // SOL_BIND_TRAITS_HPP
