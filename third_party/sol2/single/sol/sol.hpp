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

// This file was generated with a script.
// Generated 2018-11-28 08:50:22.534324 UTC
// This header was generated with sol v2.20.6 (revision 9b782ff)
// https://github.com/ThePhD/sol2

#ifndef SOL_SINGLE_INCLUDE_HPP
#define SOL_SINGLE_INCLUDE_HPP

// beginning of sol.hpp

#ifndef SOL_HPP
#define SOL_HPP

#if defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING) || defined(UE_SERVER)
#define SOL_INSIDE_UNREAL 1
#endif // Unreal Engine 4 bullshit

#if defined(SOL_INSIDE_UNREAL) && SOL_INSIDE_UNREAL
#ifdef check
#define SOL_INSIDE_UNREAL_REMOVED_CHECK 1
#undef check
#endif 
#endif // Unreal Engine 4 Bullshit

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#elif defined(__clang__)
#elif defined _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4503 ) // decorated name horse shit
#pragma warning( disable : 4702 ) // unreachable code
#pragma warning( disable: 4127 ) // 'conditional expression is constant' yeah that's the point your old compilers don't have `if constexpr` you jerk
#pragma warning( disable: 4505 ) // some other nonsense warning
#endif // clang++ vs. g++ vs. VC++

// beginning of sol/forward.hpp

// beginning of sol/feature_test.hpp

#if (defined(__cplusplus) && __cplusplus == 201703L) || (defined(_MSC_VER) && _MSC_VER > 1900 && ((defined(_HAS_CXX17) && _HAS_CXX17 == 1) || (defined(_MSVC_LANG) && (_MSVC_LANG > 201402L))))
#ifndef SOL_CXX17_FEATURES
#define SOL_CXX17_FEATURES 1
#endif // C++17 features macro
#endif // C++17 features check

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#if defined(__cpp_noexcept_function_type) || ((defined(_MSC_VER) && _MSC_VER > 1911) && (defined(_MSVC_LANG) && ((_MSVC_LANG >= 201403L))))
#ifndef SOL_NOEXCEPT_FUNCTION_TYPE
#define SOL_NOEXCEPT_FUNCTION_TYPE 1
#endif // noexcept is part of a function's type
#endif // compiler-specific checks
#if defined(__clang__) && defined(__APPLE__)
#if defined(__has_include)
#if __has_include(<variant>)
#define SOL_STD_VARIANT 1
#endif // has include nonsense
#endif // __has_include
#else
#define SOL_STD_VARIANT 1
#endif // Clang screws up variant
#endif // C++17 only

// beginning of sol/config.hpp

#ifdef _MSC_VER
	#if defined(_DEBUG) && !defined(NDEBUG)

	#ifndef SOL_IN_DEBUG_DETECTED
	#define SOL_IN_DEBUG_DETECTED 1
	#endif

	#endif // VC++ Debug macros

	#ifndef _CPPUNWIND
	#ifndef SOL_NO_EXCEPTIONS
	#define SOL_NO_EXCEPTIONS 1
	#endif
	#endif // Automatic Exceptions

	#ifndef _CPPRTTI
	#ifndef SOL_NO_RTTI
	#define SOL_NO_RTTI 1
	#endif
	#endif // Automatic RTTI
#elif defined(__GNUC__) || defined(__clang__)

	#if !defined(NDEBUG) && !defined(__OPTIMIZE__)

	#ifndef SOL_IN_DEBUG_DETECTED
	#define SOL_IN_DEBUG_DETECTED 1
	#endif

	#endif // Not Debug && g++ optimizer flag

	#ifndef __EXCEPTIONS
	#ifndef SOL_NO_EXCEPTIONS
	#define SOL_NO_EXCEPTIONS 1
	#endif
	#endif // No Exceptions

	#ifndef __GXX_RTTI
	#ifndef SOL_NO_RTII
	#define SOL_NO_RTTI 1
	#endif
	#endif // No RTTI

#endif // vc++ || clang++/g++

#if defined(SOL_CHECK_ARGUMENTS) && SOL_CHECK_ARGUMENTS

	// Checks low-level getter function
	// (and thusly, affects nearly entire framework)
	#if !defined(SOL_SAFE_GETTER)
	#define SOL_SAFE_GETTER 1
	#endif

	// Checks access on usertype functions
	// local my_obj = my_type.new()
	// my_obj.my_member_function()
	// -- bad syntax and crash
	#if !defined(SOL_SAFE_USERTYPE)
	#define SOL_SAFE_USERTYPE 1
	#endif

	// Checks sol::reference derived boundaries
	// sol::function ref(L, 1);
	// sol::userdata sref(L, 2);
	#if !defined(SOL_SAFE_REFERENCES)
	#define SOL_SAFE_REFERENCES 1
	#endif

	// Changes all typedefs of sol::function to point to the 
	// protected_function version, instead of unsafe_function
	#if !defined(SOL_SAFE_FUNCTION)
	#define SOL_SAFE_FUNCTION 1
	#endif

	// Checks function parameters and
	// returns upon call into/from Lua
	// local a = 1
	// local b = "woof"
	// my_c_function(a, b)
	#if !defined(SOL_SAFE_FUNCTION_CALLS)
	#define SOL_SAFE_FUNCTION_CALLS 1
	#endif

	// Checks conversions
	// int v = lua["bark"];
	// int v2 = my_sol_function();
	#if !defined(SOL_SAFE_PROXIES)
	#define SOL_SAFE_PROXIES 1
	#endif

	// Check overflowing number conversions
	// for things like 64 bit integers that don't fit in a typical lua_Number
	// for Lua 5.1 and 5.2
	#if !defined(SOL_SAFE_NUMERICS)
	#define SOL_SAFE_NUMERICS 1
	#endif

	// Turn off Number Precision Checks
	// if this is defined, we do not do range 
	// checks on integers / unsigned integers that might
	// be bigger than what Lua can represent
	#if !defined(SOL_NO_CHECK_NUMBER_PRECISION)
	// off by default
	#define SOL_NO_CHECK_NUMBER_PRECISION 0
	#endif

#endif // Turn on Safety for all if top-level macro is defined

#if defined(SOL_IN_DEBUG_DETECTED) && SOL_IN_DEBUG_DETECTED

	#if !defined(SOL_SAFE_REFERENCES)
	// Ensure that references are forcefully type-checked upon construction
	#define SOL_SAFE_REFERENCES 1
	#endif

	// Safe usertypes checks for errors such as
	// obj = my_type.new()
	// obj.f() -- note the '.' instead of ':'
	// usertypes should be safe no matter what
	#if !defined(SOL_SAFE_USERTYPE)
	#define SOL_SAFE_USERTYPE 1
	#endif

	#if !defined(SOL_SAFE_FUNCTION_CALLS)
	// Function calls from Lua should be automatically safe in debug mode
	#define SOL_SAFE_FUNCTION_CALLS 1
	#endif

	// Print any exceptions / errors that occur
	// in debug mode to the default error stream / console
	#if !defined(SOL_PRINT_ERRORS)
	#define SOL_PRINT_ERRORS 1
	#endif

#endif // DEBUG: Turn on all debug safety features for VC++ / g++ / clang++ and similar

#if !defined(SOL_PRINT_ERRORS)
#define SOL_PRINT_ERRORS 0
#endif

#if !defined(SOL_DEFAULT_PASS_ON_ERROR)
#define SOL_DEFAULT_PASS_ON_ERROR 0
#endif

#if !defined(SOL_ENABLE_INTEROP)
#define SOL_ENABLE_INTEROP 0
#endif

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) || defined(__OBJC__) || defined(nil)
#if !defined(SOL_NO_NIL)
#define SOL_NO_NIL 1
#endif
#endif // avoiding nil defines / keywords

#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
#ifndef SOL_UNORDERED_MAP_COMPATIBLE_HASH
#define SOL_UNORDERED_MAP_COMPATIBLE_HASH 1
#endif // SOL_UNORDERED_MAP_COMPATIBLE_HASH
#endif 

#ifndef SOL_STACK_STRING_OPTIMIZATION_SIZE
#define SOL_STACK_STRING_OPTIMIZATION_SIZE 1024
#endif // Optimized conversion routines using a KB or so off the stack

// end of sol/config.hpp

// beginning of sol/config_setup.hpp

// end of sol/config_setup.hpp

// end of sol/feature_test.hpp

namespace sol {

	template <bool b>
	class basic_reference;
	using reference = basic_reference<false>;
	using main_reference = basic_reference<true>;
	class stack_reference;

	struct proxy_base_tag;
	template <typename Super>
	struct proxy_base;
	template <typename Table, typename Key>
	struct proxy;

	template <typename T>
	class usertype;
	template <typename T>
	class simple_usertype;
	template <bool, typename T>
	class basic_table_core;
	template <bool b>
	using table_core = basic_table_core<b, reference>;
	template <bool b>
	using main_table_core = basic_table_core<b, main_reference>;
	template <bool b>
	using stack_table_core = basic_table_core<b, stack_reference>;
	template <typename T>
	using basic_table = basic_table_core<false, T>;
	typedef table_core<false> table;
	typedef table_core<true> global_table;
	typedef main_table_core<false> main_table;
	typedef main_table_core<true> main_global_table;
	typedef stack_table_core<false> stack_table;
	typedef stack_table_core<true> stack_global_table;
	template <typename base_t>
	struct basic_environment;
	using environment = basic_environment<reference>;
	using main_environment = basic_environment<main_reference>;
	using stack_environment = basic_environment<stack_reference>;
	template <typename T, bool>
	class basic_function;
	template <typename T, bool, typename H>
	class basic_protected_function;
	using unsafe_function = basic_function<reference, false>;
	using safe_function = basic_protected_function<reference, false, reference>;
	using main_unsafe_function = basic_function<main_reference, false>;
	using main_safe_function = basic_protected_function<main_reference, false, reference>;
	using stack_unsafe_function = basic_function<stack_reference, false>;
	using stack_safe_function = basic_protected_function<stack_reference, false, reference>;
	using stack_aligned_unsafe_function = basic_function<stack_reference, true>;
	using stack_aligned_safe_function = basic_protected_function<stack_reference, true, reference>;
	using protected_function = safe_function;
	using main_protected_function = main_safe_function;
	using stack_protected_function = stack_safe_function;
	using stack_aligned_protected_function = stack_aligned_safe_function;
#if defined(SOL_SAFE_FUNCTION) && SOL_SAFE_FUNCTION
	using function = protected_function;
	using main_function = main_protected_function;
	using stack_function = stack_protected_function;
#else
	using function = unsafe_function;
	using main_function = main_unsafe_function;
	using stack_function = stack_unsafe_function;
#endif
	using stack_aligned_function = stack_aligned_unsafe_function;
	using stack_aligned_stack_handler_function = basic_protected_function<stack_reference, true, stack_reference>;

	struct unsafe_function_result;
	struct protected_function_result;
	using safe_function_result = protected_function_result;
#if defined(SOL_SAFE_FUNCTION) && SOL_SAFE_FUNCTION
	using function_result = safe_function_result;
#else
	using function_result = unsafe_function_result;
#endif

	template <typename base_t>
	class basic_object;
	template <typename base_t>
	class basic_userdata;
	template <typename base_t>
	class basic_lightuserdata;
	template <typename base_t>
	class basic_coroutine;
	template <typename base_t>
	class basic_thread;

	using object = basic_object<reference>;
	using userdata = basic_userdata<reference>;
	using lightuserdata = basic_lightuserdata<reference>;
	using thread = basic_thread<reference>;
	using coroutine = basic_coroutine<reference>;
	using main_object = basic_object<main_reference>;
	using main_userdata = basic_userdata<main_reference>;
	using main_lightuserdata = basic_lightuserdata<main_reference>;
	using main_coroutine = basic_coroutine<main_reference>;
	using stack_object = basic_object<stack_reference>;
	using stack_userdata = basic_userdata<stack_reference>;
	using stack_lightuserdata = basic_lightuserdata<stack_reference>;
	using stack_thread = basic_thread<stack_reference>;
	using stack_coroutine = basic_coroutine<stack_reference>;

	struct stack_proxy_base;
	struct stack_proxy;
	struct variadic_args;
	struct variadic_results;
	struct stack_count;
	struct this_state;
	struct this_main_state;
	struct this_environment;

	template <typename T>
	struct as_table_t;
	template <typename T>
	struct as_container_t;
	template <typename T>
	struct nested;
	template <typename T>
	struct light;
	template <typename T>
	struct user;
	template <typename T>
	struct as_args_t;
	template <typename T>
	struct protect_t;
	template <typename F, typename... Filters>
	struct filter_wrapper;

	template <typename T>
	struct usertype_traits;
	template <typename T>
	struct unique_usertype_traits;
} // namespace sol

// end of sol/forward.hpp

// beginning of sol/state.hpp

// beginning of sol/state_view.hpp

// beginning of sol/error.hpp

#include <stdexcept>
#include <string>

namespace sol {
	namespace detail {
		struct direct_error_tag {};
		const auto direct_error = direct_error_tag{};
	} // namespace detail

	class error : public std::runtime_error {
	private:
		// Because VC++ is upsetting, most of the time!
		std::string w;

	public:
		error(const std::string& str)
		: error(detail::direct_error, "lua: error: " + str) {
		}
		error(std::string&& str)
		: error(detail::direct_error, "lua: error: " + std::move(str)) {
		}
		error(detail::direct_error_tag, const std::string& str)
		: std::runtime_error(""), w(str) {
		}
		error(detail::direct_error_tag, std::string&& str)
		: std::runtime_error(""), w(std::move(str)) {
		}

		error(const error& e) = default;
		error(error&& e) = default;
		error& operator=(const error& e) = default;
		error& operator=(error&& e) = default;

		virtual const char* what() const noexcept override {
			return w.c_str();
		}
	};

} // namespace sol

// end of sol/error.hpp

// beginning of sol/table.hpp

// beginning of sol/table_core.hpp

// beginning of sol/proxy.hpp

// beginning of sol/traits.hpp

// beginning of sol/tuple.hpp

#include <tuple>
#include <cstddef>

namespace sol {
	namespace detail {
		using swallow = std::initializer_list<int>;
	} // namespace detail

	template <typename... Args>
	struct types {
		typedef std::make_index_sequence<sizeof...(Args)> indices;
		static constexpr std::size_t size() {
			return sizeof...(Args);
		}
	};
	namespace meta {
		namespace detail {
			template <typename... Args>
			struct tuple_types_ { typedef types<Args...> type; };

			template <typename... Args>
			struct tuple_types_<std::tuple<Args...>> { typedef types<Args...> type; };
		} // namespace detail

		template <typename T>
		using unqualified = std::remove_cv<std::remove_reference_t<T>>;

		template <typename T>
		using unqualified_t = typename unqualified<T>::type;

		template <typename... Args>
		using tuple_types = typename detail::tuple_types_<Args...>::type;

		template <typename Arg>
		struct pop_front_type;

		template <typename Arg>
		using pop_front_type_t = typename pop_front_type<Arg>::type;

		template <typename... Args>
		struct pop_front_type<types<Args...>> {
			typedef void front_type;
			typedef types<Args...> type;
		};

		template <typename Arg, typename... Args>
		struct pop_front_type<types<Arg, Args...>> {
			typedef Arg front_type;
			typedef types<Args...> type;
		};

		template <std::size_t N, typename Tuple>
		using tuple_element = std::tuple_element<N, std::remove_reference_t<Tuple>>;

		template <std::size_t N, typename Tuple>
		using tuple_element_t = std::tuple_element_t<N, std::remove_reference_t<Tuple>>;

		template <std::size_t N, typename Tuple>
		using unqualified_tuple_element = unqualified<tuple_element_t<N, Tuple>>;

		template <std::size_t N, typename Tuple>
		using unqualified_tuple_element_t = unqualified_t<tuple_element_t<N, Tuple>>;

	} // namespace meta
} // namespace sol

// end of sol/tuple.hpp

// beginning of sol/bind_traits.hpp

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

// end of sol/bind_traits.hpp

// beginning of sol/string_view.hpp

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <string_view>
#endif // C++17 features
#include <functional>
#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
#include <boost/functional/hash.hpp>
#endif

namespace sol {
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	template <typename C, typename T = std::char_traits<C>>
	using basic_string_view = std::basic_string_view<C, T>;
	typedef std::string_view string_view;
	typedef std::wstring_view wstring_view;
	typedef std::u16string_view u16string_view;
	typedef std::u32string_view u32string_view;
	typedef std::hash<std::string_view> string_view_hash;
#else
	template <typename Char, typename Traits = std::char_traits<Char>>
	struct basic_string_view {
		std::size_t s;
		const Char* p;

		basic_string_view(const std::string& r)
		: basic_string_view(r.data(), r.size()) {
		}
		basic_string_view(const Char* ptr)
		: basic_string_view(ptr, Traits::length(ptr)) {
		}
		basic_string_view(const Char* ptr, std::size_t sz)
		: s(sz), p(ptr) {
		}

		static int compare(const Char* lhs_p, std::size_t lhs_sz, const Char* rhs_p, std::size_t rhs_sz) {
			int result = Traits::compare(lhs_p, rhs_p, lhs_sz < rhs_sz ? lhs_sz : rhs_sz);
			if (result != 0)
				return result;
			if (lhs_sz < rhs_sz)
				return -1;
			if (lhs_sz > rhs_sz)
				return 1;
			return 0;
		}

		const Char* begin() const {
			return p;
		}

		const Char* end() const {
			return p + s;
		}

		const Char* cbegin() const {
			return p;
		}

		const Char* cend() const {
			return p + s;
		}

		const Char* data() const {
			return p;
		}

		std::size_t size() const {
			return s;
		}

		std::size_t length() const {
			return size();
		}

		operator std::basic_string<Char, Traits>() const {
			return std::basic_string<Char, Traits>(data(), size());
		}

		bool operator==(const basic_string_view& r) const {
			return compare(p, s, r.data(), r.size()) == 0;
		}

		bool operator==(const Char* r) const {
			return compare(r, Traits::length(r), p, s) == 0;
		}

		bool operator==(const std::basic_string<Char, Traits>& r) const {
			return compare(r.data(), r.size(), p, s) == 0;
		}

		bool operator!=(const basic_string_view& r) const {
			return !(*this == r);
		}

		bool operator!=(const char* r) const {
			return !(*this == r);
		}

		bool operator!=(const std::basic_string<Char, Traits>& r) const {
			return !(*this == r);
		}
	};

	template <typename Ch, typename Tr = std::char_traits<Ch>>
	struct basic_string_view_hash {
		typedef basic_string_view<Ch, Tr> argument_type;
		typedef std::size_t result_type;

		template <typename Al>
		result_type operator()(const std::basic_string<Ch, Tr, Al>& r) const {
			return (*this)(argument_type(r.c_str(), r.size()));
		}

		result_type operator()(const argument_type& r) const {
#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
			return boost::hash_range(r.begin(), r.end());
#else
			// Modified, from libstdc++
			// An implementation attempt at Fowler No Voll, 1a.
			// Supposedly, used in MSVC,
			// GCC (libstdc++) uses MurmurHash of some sort for 64-bit though...?
			// But, well. Can't win them all, right?
			// This should normally only apply when NOT using boost,
			// so this should almost never be tapped into...
			std::size_t hash = 0;
			const unsigned char* cptr = reinterpret_cast<const unsigned char*>(r.data());
			for (std::size_t sz = r.size(); sz != 0; --sz) {
				hash ^= static_cast<size_t>(*cptr++);
				hash *= static_cast<size_t>(1099511628211ULL);
			}
			return hash; 
#endif
		}
	};
} // namespace sol

namespace std {
	template <typename Ch, typename Tr>
	struct hash< ::sol::basic_string_view<Ch, Tr> > : ::sol::basic_string_view_hash<Ch, Tr> {};
} // namespace std

namespace sol {
	using string_view = basic_string_view<char>;
	using wstring_view = basic_string_view<wchar_t>;
	using u16string_view = basic_string_view<char16_t>;
	using u32string_view = basic_string_view<char32_t>;
	using string_view_hash = std::hash<string_view>;
#endif // C++17 Support
} // namespace sol

// end of sol/string_view.hpp

#include <type_traits>
#include <cstdint>
#include <memory>
#include <array>
#include <iterator>
#include <iosfwd>

namespace sol {
	template <std::size_t I>
	using index_value = std::integral_constant<std::size_t, I>;

	namespace meta {
		typedef std::array<char, 1> sfinae_yes_t;
		typedef std::array<char, 2> sfinae_no_t;

		template <typename T>
		struct identity { typedef T type; };

		template <typename T>
		using identity_t = typename identity<T>::type;

		template <typename... Args>
		struct is_tuple : std::false_type {};

		template <typename... Args>
		struct is_tuple<std::tuple<Args...>> : std::true_type {};

		template <typename T>
		struct is_builtin_type : std::integral_constant<bool, std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_array<T>::value> {};

		template <typename T>
		struct unwrapped {
			typedef T type;
		};

		template <typename T>
		struct unwrapped<std::reference_wrapper<T>> {
			typedef T type;
		};

		template <typename T>
		using unwrapped_t = typename unwrapped<T>::type;

		template <typename T>
		struct unwrap_unqualified : unwrapped<unqualified_t<T>> {};

		template <typename T>
		using unwrap_unqualified_t = typename unwrap_unqualified<T>::type;

		template <typename T>
		struct remove_member_pointer;

		template <typename R, typename T>
		struct remove_member_pointer<R T::*> {
			typedef R type;
		};

		template <typename R, typename T>
		struct remove_member_pointer<R T::*const> {
			typedef R type;
		};

		template <typename T>
		using remove_member_pointer_t = remove_member_pointer<T>;

		namespace meta_detail {
			template <typename T, template <typename...> class Templ>
			struct is_specialization_of : std::false_type {};
			template <typename... T, template <typename...> class Templ>
			struct is_specialization_of<Templ<T...>, Templ> : std::true_type {};
		}

		template <typename T, template <typename...> class Templ>
		using is_specialization_of = meta_detail::is_specialization_of<std::remove_cv_t<T>, Templ>;

		template <class T, class...>
		struct all_same : std::true_type {};

		template <class T, class U, class... Args>
		struct all_same<T, U, Args...> : std::integral_constant<bool, std::is_same<T, U>::value && all_same<T, Args...>::value> {};

		template <class T, class...>
		struct any_same : std::false_type {};

		template <class T, class U, class... Args>
		struct any_same<T, U, Args...> : std::integral_constant<bool, std::is_same<T, U>::value || any_same<T, Args...>::value> {};

		template <bool B>
		using boolean = std::integral_constant<bool, B>;

		template <typename T>
		using invoke_t = typename T::type;

		template <typename T>
		using invoke_b = boolean<T::value>;

		template <typename T>
		using neg = boolean<!T::value>;

		template <typename Condition, typename Then, typename Else>
		using condition = std::conditional_t<Condition::value, Then, Else>;

		template <typename... Args>
		struct all : boolean<true> {};

		template <typename T, typename... Args>
		struct all<T, Args...> : condition<T, all<Args...>, boolean<false>> {};

		template <typename... Args>
		struct any : boolean<false> {};

		template <typename T, typename... Args>
		struct any<T, Args...> : condition<T, boolean<true>, any<Args...>> {};

		enum class enable_t {
			_
		};

		constexpr const auto enabler = enable_t::_;

		template <bool value, typename T = void>
		using disable_if_t = std::enable_if_t<!value, T>;

		template <typename... Args>
		using enable = std::enable_if_t<all<Args...>::value, enable_t>;

		template <typename... Args>
		using disable = std::enable_if_t<neg<all<Args...>>::value, enable_t>;

		template <typename... Args>
		using enable_any = std::enable_if_t<any<Args...>::value, enable_t>;

		template <typename... Args>
		using disable_any = std::enable_if_t<neg<any<Args...>>::value, enable_t>;

		template <typename V, typename... Vs>
		struct find_in_pack_v : boolean<false> {};

		template <typename V, typename Vs1, typename... Vs>
		struct find_in_pack_v<V, Vs1, Vs...> : any<boolean<(V::value == Vs1::value)>, find_in_pack_v<V, Vs...>> {};

		namespace meta_detail {
			template <std::size_t I, typename T, typename... Args>
			struct index_in_pack : std::integral_constant<std::size_t, SIZE_MAX> {};

			template <std::size_t I, typename T, typename T1, typename... Args>
			struct index_in_pack<I, T, T1, Args...> : std::conditional_t<std::is_same<T, T1>::value, std::integral_constant<std::ptrdiff_t, I>, index_in_pack<I + 1, T, Args...>> {};
		} // namespace meta_detail

		template <typename T, typename... Args>
		struct index_in_pack : meta_detail::index_in_pack<0, T, Args...> {};

		template <typename T, typename List>
		struct index_in : meta_detail::index_in_pack<0, T, List> {};

		template <typename T, typename... Args>
		struct index_in<T, types<Args...>> : meta_detail::index_in_pack<0, T, Args...> {};

		template <std::size_t I, typename... Args>
		struct at_in_pack {};

		template <std::size_t I, typename... Args>
		using at_in_pack_t = typename at_in_pack<I, Args...>::type;

		template <std::size_t I, typename Arg, typename... Args>
		struct at_in_pack<I, Arg, Args...> : std::conditional<I == 0, Arg, at_in_pack_t<I - 1, Args...>> {};

		template <typename Arg, typename... Args>
		struct at_in_pack<0, Arg, Args...> { typedef Arg type; };

		namespace meta_detail {
			template <std::size_t Limit, std::size_t I, template <typename...> class Pred, typename... Ts>
			struct count_for_pack : std::integral_constant<std::size_t, 0> {};
			template <std::size_t Limit, std::size_t I, template <typename...> class Pred, typename T, typename... Ts>
						struct count_for_pack<Limit, I, Pred, T, Ts...> : std::conditional_t < sizeof...(Ts)
					== 0
				|| Limit<2,
					   std::integral_constant<std::size_t, I + static_cast<std::size_t>(Limit != 0 && Pred<T>::value)>,
					   count_for_pack<Limit - 1, I + static_cast<std::size_t>(Pred<T>::value), Pred, Ts...>> {};
			template <std::size_t I, template <typename...> class Pred, typename... Ts>
			struct count_2_for_pack : std::integral_constant<std::size_t, 0> {};
			template <std::size_t I, template <typename...> class Pred, typename T, typename U, typename... Ts>
			struct count_2_for_pack<I, Pred, T, U, Ts...> : std::conditional_t<sizeof...(Ts) == 0,
													   std::integral_constant<std::size_t, I + static_cast<std::size_t>(Pred<T>::value)>,
													   count_2_for_pack<I + static_cast<std::size_t>(Pred<T>::value), Pred, Ts...>> {};
		} // namespace meta_detail

		template <template <typename...> class Pred, typename... Ts>
		struct count_for_pack : meta_detail::count_for_pack<sizeof...(Ts), 0, Pred, Ts...> {};

		template <template <typename...> class Pred, typename List>
		struct count_for;

		template <template <typename...> class Pred, typename... Args>
		struct count_for<Pred, types<Args...>> : count_for_pack<Pred, Args...> {};

		template <std::size_t Limit, template <typename...> class Pred, typename... Ts>
		struct count_for_to_pack : meta_detail::count_for_pack<Limit, 0, Pred, Ts...> {};

		template <template <typename...> class Pred, typename... Ts>
		struct count_2_for_pack : meta_detail::count_2_for_pack<0, Pred, Ts...> {};

		template <typename... Args>
		struct return_type {
			typedef std::tuple<Args...> type;
		};

		template <typename T>
		struct return_type<T> {
			typedef T type;
		};

		template <>
		struct return_type<> {
			typedef void type;
		};

		template <typename... Args>
		using return_type_t = typename return_type<Args...>::type;

		namespace meta_detail {
			template <typename>
			struct always_true : std::true_type {};
			struct is_invokable_tester {
				template <typename Fun, typename... Args>
				static always_true<decltype(std::declval<Fun>()(std::declval<Args>()...))> test(int);
				template <typename...>
				static std::false_type test(...);
			};
		} // namespace meta_detail

		template <typename T>
		struct is_invokable;
		template <typename Fun, typename... Args>
		struct is_invokable<Fun(Args...)> : decltype(meta_detail::is_invokable_tester::test<Fun, Args...>(0)) {};

		namespace meta_detail {

			template <typename T, typename = void>
			struct is_callable : std::is_function<std::remove_pointer_t<T>> {};

			template <typename T>
			struct is_callable<T, std::enable_if_t<std::is_final<unqualified_t<T>>::value 
				&& std::is_class<unqualified_t<T>>::value
				&&  std::is_same<decltype(void(&T::operator())), void>::value>> {

			};

			template <typename T>
			struct is_callable<T, std::enable_if_t<!std::is_final<unqualified_t<T>>::value && std::is_class<unqualified_t<T>>::value && std::is_destructible<unqualified_t<T>>::value>> {
				using yes = char;
				using no = struct { char s[2]; };

				struct F {
					void operator()();
				};
				struct Derived : T, F {};
				template <typename U, U>
				struct Check;

				template <typename V>
				static no test(Check<void (F::*)(), &V::operator()>*);

				template <typename>
				static yes test(...);

				static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
			};

			template <typename T>
			struct is_callable<T, std::enable_if_t<!std::is_final<unqualified_t<T>>::value && std::is_class<unqualified_t<T>>::value && !std::is_destructible<unqualified_t<T>>::value>> {
				using yes = char;
				using no = struct { char s[2]; };

				struct F {
					void operator()();
				};
				struct Derived : T, F {
					~Derived() = delete;
				};
				template <typename U, U>
				struct Check;

				template <typename V>
				static no test(Check<void (F::*)(), &V::operator()>*);

				template <typename>
				static yes test(...);

				static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
			};

			struct has_begin_end_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename B = decltype(std::declval<U&>().begin()),
					typename E = decltype(std::declval<U&>().end())>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_key_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::key_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_mapped_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::mapped_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_value_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::value_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_iterator_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::iterator>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_key_value_pair_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::value_type,
					typename F = decltype(std::declval<V&>().first),
					typename S = decltype(std::declval<V&>().second)>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			template <typename T>
			struct has_push_back_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().push_back(std::declval<std::add_rvalue_reference_t<typename C::value_type>>())) *);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_insert_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().insert(std::declval<std::add_rvalue_reference_t<typename C::const_iterator>>(), std::declval<std::add_rvalue_reference_t<typename C::value_type>>()))*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_insert_after_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().insert_after(std::declval<std::add_rvalue_reference_t<typename C::const_iterator>>(), std::declval<std::add_rvalue_reference_t<typename C::value_type>>()))*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_size_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().size())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_max_size_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().max_size())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_to_string_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().to_string())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};
#if defined(_MSC_VER) && _MSC_VER <= 1910
			template <typename T, typename U, typename = decltype(std::declval<T&>() < std::declval<U&>())>
			std::true_type supports_op_less_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_less_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() == std::declval<U&>())>
			std::true_type supports_op_equal_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_equal_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() <= std::declval<U&>())>
			std::true_type supports_op_less_equal_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_less_equal_test(...);
			template <typename T, typename OS, typename = decltype(std::declval<OS&>() << std::declval<T&>())>
			std::true_type supports_ostream_op(std::reference_wrapper<T>, std::reference_wrapper<OS>);
			std::false_type supports_ostream_op(...);
			template <typename T, typename = decltype(to_string(std::declval<T&>()))>
			std::true_type supports_adl_to_string(std::reference_wrapper<T>);
			std::false_type supports_adl_to_string(...);
#else
			template <typename T, typename U, typename = decltype(std::declval<T&>() < std::declval<U&>())>
			std::true_type supports_op_less_test(const T&, const U&);
			std::false_type supports_op_less_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() == std::declval<U&>())>
			std::true_type supports_op_equal_test(const T&, const U&);
			std::false_type supports_op_equal_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() <= std::declval<U&>())>
			std::true_type supports_op_less_equal_test(const T&, const U&);
			std::false_type supports_op_less_equal_test(...);
			template <typename T, typename OS, typename = decltype(std::declval<OS&>() << std::declval<T&>())>
			std::true_type supports_ostream_op(const T&, const OS&);
			std::false_type supports_ostream_op(...);
			template <typename T, typename = decltype(to_string(std::declval<T&>()))>
			std::true_type supports_adl_to_string(const T&);
			std::false_type supports_adl_to_string(...);
#endif

			template <typename T, bool b>
			struct is_matched_lookup_impl : std::false_type {};
			template <typename T>
			struct is_matched_lookup_impl<T, true> : std::is_same<typename T::key_type, typename T::value_type> {};
		} // namespace meta_detail

#if defined(_MSC_VER) && _MSC_VER <= 1910
		template <typename T, typename U = T>
		using supports_op_less = decltype(meta_detail::supports_op_less_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = T>
		using supports_op_equal = decltype(meta_detail::supports_op_equal_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = T>
		using supports_op_less_equal = decltype(meta_detail::supports_op_less_equal_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = std::ostream>
		using supports_ostream_op = decltype(meta_detail::supports_ostream_op(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T>
		using supports_adl_to_string = decltype(meta_detail::supports_adl_to_string(std::ref(std::declval<T&>())));
#else
		template <typename T, typename U = T>
		using supports_op_less = decltype(meta_detail::supports_op_less_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = T>
		using supports_op_equal = decltype(meta_detail::supports_op_equal_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = T>
		using supports_op_less_equal = decltype(meta_detail::supports_op_less_equal_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = std::ostream>
		using supports_ostream_op = decltype(meta_detail::supports_ostream_op(std::declval<T&>(), std::declval<U&>()));
		template <typename T>
		using supports_adl_to_string = decltype(meta_detail::supports_adl_to_string(std::declval<T&>()));
#endif
		template <typename T>
		using supports_to_string_member = meta::boolean<meta_detail::has_to_string_test<T>::value>;

		template <typename T>
		struct is_callable : boolean<meta_detail::is_callable<T>::value> {};

		template <typename T>
		struct has_begin_end : decltype(meta_detail::has_begin_end_impl::test<T>(0)) {};

		template <typename T>
		struct has_key_value_pair : decltype(meta_detail::has_key_value_pair_impl::test<T>(0)) {};

		template <typename T>
		struct has_key_type : decltype(meta_detail::has_key_type_impl::test<T>(0)) {};

		template <typename T>
		struct has_mapped_type : decltype(meta_detail::has_mapped_type_impl::test<T>(0)) {};

		template <typename T>
		struct has_iterator : decltype(meta_detail::has_iterator_impl::test<T>(0)) {};

		template <typename T>
		struct has_value_type : decltype(meta_detail::has_value_type_impl::test<T>(0)) {};

		template <typename T>
		using has_push_back = meta::boolean<meta_detail::has_push_back_test<T>::value>;

		template <typename T>
		using has_max_size = meta::boolean<meta_detail::has_max_size_test<T>::value>;

		template <typename T>
		using has_insert = meta::boolean<meta_detail::has_insert_test<T>::value>;

		template <typename T>
		using has_insert_after = meta::boolean<meta_detail::has_insert_after_test<T>::value>;

		template <typename T>
		using has_size = meta::boolean<meta_detail::has_size_test<T>::value || meta_detail::has_size_test<const T>::value>;

		template <typename T>
		struct is_associative : meta::all<has_key_type<T>, has_key_value_pair<T>, has_mapped_type<T>> {};

		template <typename T>
		struct is_lookup : meta::all<has_key_type<T>, has_value_type<T>> {};

		template <typename T>
		struct is_matched_lookup : meta_detail::is_matched_lookup_impl<T, is_lookup<T>::value> {};

		template <typename T>
		using is_string_like = any<
			is_specialization_of<meta::unqualified_t<T>, std::basic_string>,
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			is_specialization_of<meta::unqualified_t<T>, std::basic_string_view>,
#else
			is_specialization_of<meta::unqualified_t<T>, basic_string_view>,
#endif
			meta::all<std::is_array<unqualified_t<T>>, meta::any_same<meta::unqualified_t<std::remove_all_extents_t<meta::unqualified_t<T>>>, char, char16_t, char32_t, wchar_t>>
		>;

		template <typename T>
		using is_string_constructible = any<
			meta::all<std::is_array<unqualified_t<T>>, std::is_same<meta::unqualified_t<std::remove_all_extents_t<meta::unqualified_t<T>>>, char>>,
			std::is_same<unqualified_t<T>, const char*>, 
			std::is_same<unqualified_t<T>, char>, std::is_same<unqualified_t<T>, std::string>, std::is_same<unqualified_t<T>, std::initializer_list<char>>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			, std::is_same<unqualified_t<T>, std::string_view>
#endif
			>;

		template <typename T>
		struct is_pair : std::false_type {};

		template <typename T1, typename T2>
		struct is_pair<std::pair<T1, T2>> : std::true_type {};

		template <typename T>
		using is_c_str = any<
			std::is_same<std::decay_t<unqualified_t<T>>, const char*>,
			std::is_same<std::decay_t<unqualified_t<T>>, char*>,
			std::is_same<unqualified_t<T>, std::string>>;

		template <typename T>
		struct is_move_only : all<
							  neg<std::is_reference<T>>,
							  neg<std::is_copy_constructible<unqualified_t<T>>>,
							  std::is_move_constructible<unqualified_t<T>>> {};

		template <typename T>
		using is_not_move_only = neg<is_move_only<T>>;

		namespace meta_detail {
			template <typename T, meta::disable<meta::is_specialization_of<meta::unqualified_t<T>, std::tuple>> = meta::enabler>
			decltype(auto) force_tuple(T&& x) {
				return std::tuple<std::decay_t<T>>(std::forward<T>(x));
			}

			template <typename T, meta::enable<meta::is_specialization_of<meta::unqualified_t<T>, std::tuple>> = meta::enabler>
			decltype(auto) force_tuple(T&& x) {
				return std::forward<T>(x);
			}
		} // namespace meta_detail

		template <typename... X>
		decltype(auto) tuplefy(X&&... x) {
			return std::tuple_cat(meta_detail::force_tuple(std::forward<X>(x))...);
		}

		template <typename T, typename = void>
		struct iterator_tag {
			using type = std::input_iterator_tag;
		};

		template <typename T>
		struct iterator_tag<T, std::conditional_t<false, typename T::iterator_category, void>> {
			using type = typename T::iterator_category;
		};

	} // namespace meta

	namespace detail {
		template <typename T>
		struct is_pointer_like : std::is_pointer<T> {};
		template <typename T, typename D>
		struct is_pointer_like<std::unique_ptr<T, D>> : std::true_type {};
		template <typename T>
		struct is_pointer_like<std::shared_ptr<T>> : std::true_type {};

		template <std::size_t I, typename Tuple>
		decltype(auto) forward_get(Tuple&& tuple) {
			return std::forward<meta::tuple_element_t<I, Tuple>>(std::get<I>(tuple));
		}

		template <std::size_t... I, typename Tuple>
		auto forward_tuple_impl(std::index_sequence<I...>, Tuple&& tuple) -> decltype(std::tuple<decltype(forward_get<I>(tuple))...>(forward_get<I>(tuple)...)) {
			return std::tuple<decltype(forward_get<I>(tuple))...>(std::move(std::get<I>(tuple))...);
		}

		template <typename Tuple>
		auto forward_tuple(Tuple&& tuple) {
			auto x = forward_tuple_impl(std::make_index_sequence<std::tuple_size<meta::unqualified_t<Tuple>>::value>(), std::forward<Tuple>(tuple));
			return x;
		}

		template <typename T>
		auto unwrap(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T>
		T& unwrap(std::reference_wrapper<T> arg) {
			return arg.get();
		}

		template <typename T, meta::enable<meta::neg<is_pointer_like<meta::unqualified_t<T>>>> = meta::enabler>
		auto deref(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T, meta::enable<is_pointer_like<meta::unqualified_t<T>>> = meta::enabler>
		inline auto deref(T&& item) -> decltype(*std::forward<T>(item)) {
			return *std::forward<T>(item);
		}

		template <typename T, meta::disable<is_pointer_like<meta::unqualified_t<T>>, meta::neg<std::is_pointer<meta::unqualified_t<T>>>> = meta::enabler>
		auto deref_non_pointer(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T, meta::enable<is_pointer_like<meta::unqualified_t<T>>, meta::neg<std::is_pointer<meta::unqualified_t<T>>>> = meta::enabler>
		inline auto deref_non_pointer(T&& item) -> decltype(*std::forward<T>(item)) {
			return *std::forward<T>(item);
		}

		template <typename T>
		inline T* ptr(T& val) {
			return std::addressof(val);
		}

		template <typename T>
		inline T* ptr(std::reference_wrapper<T> val) {
			return std::addressof(val.get());
		}

		template <typename T>
		inline T* ptr(T* val) {
			return val;
		}
	} // namespace detail
} // namespace sol

// end of sol/traits.hpp

// beginning of sol/function.hpp

// beginning of sol/stack.hpp

// beginning of sol/trampoline.hpp

// beginning of sol/types.hpp

// beginning of sol/optional.hpp

// beginning of sol/compatibility.hpp

// beginning of sol/compatibility/version.hpp

#if defined(SOL_USING_CXX_LUA) && SOL_USING_CXX_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#if defined(SOL_USING_CXX_LUAJIT) && SOL_USING_CXX_LUAJIT
#include <luajit.h>
#endif // C++ LuaJIT ... whatever that means
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !(SOL_EXCEPTIONS_SAFE_PROPAGATION)) && (!defined(SOL_EXCEPTIONS_ALWAYS_UNSAFE) || !(SOL_EXCEPTIONS_ALWAYS_UNSAFE))
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#endif // Exceptions can be propagated safely using C++-compiled Lua
#else
#include <lua.hpp>
#endif // C++ Mangling for Lua

#ifdef LUAJIT_VERSION
#ifndef SOL_LUAJIT
#define SOL_LUAJIT 1
#ifndef SOL_LUAJIT_VERSION
#define SOL_LUAJIT_VERSION LUAJIT_VERSION_NUM
#endif // SOL_LUAJIT_VERSION definition, if not present
#endif // sol luajit
#endif // luajit

#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502
#define SOL_LUA_VERSION LUA_VERSION_NUM
#elif defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501
#define SOL_LUA_VERSION LUA_VERSION_NUM
#elif !defined(LUA_VERSION_NUM) || !(LUA_VERSION_NUM)
#define SOL_LUA_VERSION 500
#else
#define SOL_LUA_VERSION 502
#endif // Lua Version 502, 501 || luajit, 500

// end of sol/compatibility/version.hpp

#if !defined(SOL_NO_COMPAT) || !(SOL_NO_COMPAT)

#if defined(SOL_USING_CXX_LUA) && SOL_USING_CXX_LUA
#ifndef COMPAT53_LUA_CPP
#define COMPAT53_LUA_CPP 1
#endif // Build Lua Compat layer as C++
#endif
#ifndef COMPAT53_INCLUDE_SOURCE
#define COMPAT53_INCLUDE_SOURCE 1
#endif // Build Compat Layer Inline
// beginning of sol/compatibility/compat-5.3.h

#ifndef KEPLER_PROJECT_COMPAT53_H_
#define KEPLER_PROJECT_COMPAT53_H_

#include <stddef.h>
#include <limits.h>
#include <string.h>
#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
extern "C" {
#endif
#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
}
#endif

#ifndef COMPAT53_PREFIX
/* we chose this name because many other lua bindings / libs have
* their own compatibility layer, and that use the compat53 declaration
* frequently, causing all kinds of linker / compiler issues
*/
#  define COMPAT53_PREFIX kp_compat53
#endif // COMPAT53_PREFIX

#ifndef COMPAT53_API
#  if defined(COMPAT53_INCLUDE_SOURCE) && COMPAT53_INCLUDE_SOURCE
#    if defined(__GNUC__) || defined(__clang__)
#      define COMPAT53_API __attribute__((__unused__)) static
#    else
#      define COMPAT53_API static
#    endif /* Clang/GCC */
#  else /* COMPAT53_INCLUDE_SOURCE */
/* we are not including source, so everything is extern */
#      define COMPAT53_API extern
#  endif /* COMPAT53_INCLUDE_SOURCE */
#endif /* COMPAT53_PREFIX */

#define COMPAT53_CONCAT_HELPER(a, b) a##b
#define COMPAT53_CONCAT(a, b) COMPAT53_CONCAT_HELPER(a, b)

/* declarations for Lua 5.1 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501

/* XXX not implemented:
* lua_arith (new operators)
* lua_upvalueid
* lua_upvaluejoin
* lua_version
* lua_yieldk
*/

#ifndef LUA_OK
#  define LUA_OK 0
#endif
#ifndef LUA_OPADD
#  define LUA_OPADD 0
#endif
#ifndef LUA_OPSUB
#  define LUA_OPSUB 1
#endif
#ifndef LUA_OPMUL
#  define LUA_OPMUL 2
#endif
#ifndef LUA_OPDIV
#  define LUA_OPDIV 3
#endif
#ifndef LUA_OPMOD
#  define LUA_OPMOD 4
#endif
#ifndef LUA_OPPOW
#  define LUA_OPPOW 5
#endif
#ifndef LUA_OPUNM
#  define LUA_OPUNM 6
#endif
#ifndef LUA_OPEQ
#  define LUA_OPEQ 0
#endif
#ifndef LUA_OPLT
#  define LUA_OPLT 1
#endif
#ifndef LUA_OPLE
#  define LUA_OPLE 2
#endif

/* LuaJIT/Lua 5.1 does not have the updated
* error codes for thread status/function returns (but some patched versions do)
* define it only if it's not found
*/
#if !defined(LUA_ERRGCMM)
/* Use + 2 because in some versions of Lua (Lua 5.1)
* LUA_ERRFILE is defined as (LUA_ERRERR+1)
* so we need to avoid it (LuaJIT might have something at this
* integer value too)
*/
#  define LUA_ERRGCMM (LUA_ERRERR + 2)
#endif /* LUA_ERRGCMM define */

typedef size_t lua_Unsigned;

typedef struct luaL_Buffer_53 {
	luaL_Buffer b; /* make incorrect code crash! */
	char *ptr;
	size_t nelems;
	size_t capacity;
	lua_State *L2;
} luaL_Buffer_53;
#define luaL_Buffer luaL_Buffer_53

/* In PUC-Rio 5.1, userdata is a simple FILE*
* In LuaJIT, it's a struct where the first member is a FILE*
* We can't support the `closef` member
*/
typedef struct luaL_Stream {
	FILE *f;
} luaL_Stream;

#define lua_absindex COMPAT53_CONCAT(COMPAT53_PREFIX, _absindex)
COMPAT53_API int lua_absindex(lua_State *L, int i);

#define lua_arith COMPAT53_CONCAT(COMPAT53_PREFIX, _arith)
COMPAT53_API void lua_arith(lua_State *L, int op);

#define lua_compare COMPAT53_CONCAT(COMPAT53_PREFIX, _compare)
COMPAT53_API int lua_compare(lua_State *L, int idx1, int idx2, int op);

#define lua_copy COMPAT53_CONCAT(COMPAT53_PREFIX, _copy)
COMPAT53_API void lua_copy(lua_State *L, int from, int to);

#define lua_getuservalue(L, i) \
  (lua_getfenv((L), (i)), lua_type((L), -1))
#define lua_setuservalue(L, i) \
  (luaL_checktype((L), -1, LUA_TTABLE), lua_setfenv((L), (i)))

#define lua_len COMPAT53_CONCAT(COMPAT53_PREFIX, _len)
COMPAT53_API void lua_len(lua_State *L, int i);

#define lua_pushstring(L, s) \
  (lua_pushstring((L), (s)), lua_tostring((L), -1))

#define lua_pushlstring(L, s, len) \
  ((((len) == 0) ? lua_pushlstring((L), "", 0) : lua_pushlstring((L), (s), (len))), lua_tostring((L), -1))

#ifndef luaL_newlibtable
#  define luaL_newlibtable(L, l) \
  (lua_createtable((L), 0, sizeof((l))/sizeof(*(l))-1))
#endif
#ifndef luaL_newlib
#  define luaL_newlib(L, l) \
  (luaL_newlibtable((L), (l)), luaL_register((L), NULL, (l)))
#endif

#define lua_pushglobaltable(L) \
  lua_pushvalue((L), LUA_GLOBALSINDEX)

#define lua_rawgetp COMPAT53_CONCAT(COMPAT53_PREFIX, _rawgetp)
COMPAT53_API int lua_rawgetp(lua_State *L, int i, const void *p);

#define lua_rawsetp COMPAT53_CONCAT(COMPAT53_PREFIX, _rawsetp)
COMPAT53_API void lua_rawsetp(lua_State *L, int i, const void *p);

#define lua_rawlen(L, i) lua_objlen((L), (i))

#define lua_tointeger(L, i) lua_tointegerx((L), (i), NULL)

#define lua_tonumberx COMPAT53_CONCAT(COMPAT53_PREFIX, _tonumberx)
COMPAT53_API lua_Number lua_tonumberx(lua_State *L, int i, int *isnum);

#define luaL_checkversion COMPAT53_CONCAT(COMPAT53_PREFIX, L_checkversion)
COMPAT53_API void luaL_checkversion(lua_State *L);

#define lua_load COMPAT53_CONCAT(COMPAT53_PREFIX, _load_53)
COMPAT53_API int lua_load(lua_State *L, lua_Reader reader, void *data, const char* source, const char* mode);

#define luaL_loadfilex COMPAT53_CONCAT(COMPAT53_PREFIX, L_loadfilex)
COMPAT53_API int luaL_loadfilex(lua_State *L, const char *filename, const char *mode);

#define luaL_loadbufferx COMPAT53_CONCAT(COMPAT53_PREFIX, L_loadbufferx)
COMPAT53_API int luaL_loadbufferx(lua_State *L, const char *buff, size_t sz, const char *name, const char *mode);

#define luaL_checkstack COMPAT53_CONCAT(COMPAT53_PREFIX, L_checkstack_53)
COMPAT53_API void luaL_checkstack(lua_State *L, int sp, const char *msg);

#define luaL_getsubtable COMPAT53_CONCAT(COMPAT53_PREFIX, L_getsubtable)
COMPAT53_API int luaL_getsubtable(lua_State* L, int i, const char *name);

#define luaL_len COMPAT53_CONCAT(COMPAT53_PREFIX, L_len)
COMPAT53_API lua_Integer luaL_len(lua_State *L, int i);

#define luaL_setfuncs COMPAT53_CONCAT(COMPAT53_PREFIX, L_setfuncs)
COMPAT53_API void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup);

#define luaL_setmetatable COMPAT53_CONCAT(COMPAT53_PREFIX, L_setmetatable)
COMPAT53_API void luaL_setmetatable(lua_State *L, const char *tname);

#define luaL_testudata COMPAT53_CONCAT(COMPAT53_PREFIX, L_testudata)
COMPAT53_API void *luaL_testudata(lua_State *L, int i, const char *tname);

#define luaL_traceback COMPAT53_CONCAT(COMPAT53_PREFIX, L_traceback)
COMPAT53_API void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level);

#define luaL_fileresult COMPAT53_CONCAT(COMPAT53_PREFIX, L_fileresult)
COMPAT53_API int luaL_fileresult(lua_State *L, int stat, const char *fname);

#define luaL_execresult COMPAT53_CONCAT(COMPAT53_PREFIX, L_execresult)
COMPAT53_API int luaL_execresult(lua_State *L, int stat);

#define lua_callk(L, na, nr, ctx, cont) \
  ((void)(ctx), (void)(cont), lua_call((L), (na), (nr)))
#define lua_pcallk(L, na, nr, err, ctx, cont) \
  ((void)(ctx), (void)(cont), lua_pcall((L), (na), (nr), (err)))

#define lua_resume(L, from, nargs) \
  ((void)(from), lua_resume((L), (nargs)))

#define luaL_buffinit COMPAT53_CONCAT(COMPAT53_PREFIX, _buffinit_53)
COMPAT53_API void luaL_buffinit(lua_State *L, luaL_Buffer_53 *B);

#define luaL_prepbuffsize COMPAT53_CONCAT(COMPAT53_PREFIX, _prepbufsize_53)
COMPAT53_API char *luaL_prepbuffsize(luaL_Buffer_53 *B, size_t s);

#define luaL_addlstring COMPAT53_CONCAT(COMPAT53_PREFIX, _addlstring_53)
COMPAT53_API void luaL_addlstring(luaL_Buffer_53 *B, const char *s, size_t l);

#define luaL_addvalue COMPAT53_CONCAT(COMPAT53_PREFIX, _addvalue_53)
COMPAT53_API void luaL_addvalue(luaL_Buffer_53 *B);

#define luaL_pushresult COMPAT53_CONCAT(COMPAT53_PREFIX, _pushresult_53)
COMPAT53_API void luaL_pushresult(luaL_Buffer_53 *B);

#undef luaL_buffinitsize
#define luaL_buffinitsize(L, B, s) \
  (luaL_buffinit((L), (B)), luaL_prepbuffsize((B), (s)))

#undef luaL_prepbuffer
#define luaL_prepbuffer(B) \
  luaL_prepbuffsize((B), LUAL_BUFFERSIZE)

#undef luaL_addchar
#define luaL_addchar(B, c) \
  ((void)((B)->nelems < (B)->capacity || luaL_prepbuffsize((B), 1)), \
   ((B)->ptr[(B)->nelems++] = (c)))

#undef luaL_addsize
#define luaL_addsize(B, s) \
  ((B)->nelems += (s))

#undef luaL_addstring
#define luaL_addstring(B, s) \
  luaL_addlstring((B), (s), strlen((s)))

#undef luaL_pushresultsize
#define luaL_pushresultsize(B, s) \
  (luaL_addsize((B), (s)), luaL_pushresult((B)))

#if defined(LUA_COMPAT_APIINTCASTS)
#define lua_pushunsigned(L, n) \
  lua_pushinteger((L), (lua_Integer)(n))
#define lua_tounsignedx(L, i, is) \
  ((lua_Unsigned)lua_tointegerx((L), (i), (is)))
#define lua_tounsigned(L, i) \
  lua_tounsignedx((L), (i), NULL)
#define luaL_checkunsigned(L, a) \
  ((lua_Unsigned)luaL_checkinteger((L), (a)))
#define luaL_optunsigned(L, a, d) \
  ((lua_Unsigned)luaL_optinteger((L), (a), (lua_Integer)(d)))
#endif

#endif /* Lua 5.1 only */

/* declarations for Lua 5.1 and 5.2 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM <= 502

typedef int lua_KContext;

typedef int(*lua_KFunction)(lua_State *L, int status, lua_KContext ctx);

#define lua_dump(L, w, d, s) \
  ((void)(s), lua_dump((L), (w), (d)))

#define lua_getfield(L, i, k) \
  (lua_getfield((L), (i), (k)), lua_type((L), -1))

#define lua_gettable(L, i) \
  (lua_gettable((L), (i)), lua_type((L), -1))

#define lua_geti COMPAT53_CONCAT(COMPAT53_PREFIX, _geti)
COMPAT53_API int lua_geti(lua_State *L, int index, lua_Integer i);

#define lua_isinteger COMPAT53_CONCAT(COMPAT53_PREFIX, _isinteger)
COMPAT53_API int lua_isinteger(lua_State *L, int index);

#define lua_tointegerx COMPAT53_CONCAT(COMPAT53_PREFIX, _tointegerx_53)
COMPAT53_API lua_Integer lua_tointegerx(lua_State *L, int i, int *isnum);

#define lua_numbertointeger(n, p) \
  ((*(p) = (lua_Integer)(n)), 1)

#define lua_rawget(L, i) \
  (lua_rawget((L), (i)), lua_type((L), -1))

#define lua_rawgeti(L, i, n) \
  (lua_rawgeti((L), (i), (n)), lua_type((L), -1))

#define lua_rotate COMPAT53_CONCAT(COMPAT53_PREFIX, _rotate)
COMPAT53_API void lua_rotate(lua_State *L, int idx, int n);

#define lua_seti COMPAT53_CONCAT(COMPAT53_PREFIX, _seti)
COMPAT53_API void lua_seti(lua_State *L, int index, lua_Integer i);

#define lua_stringtonumber COMPAT53_CONCAT(COMPAT53_PREFIX, _stringtonumber)
COMPAT53_API size_t lua_stringtonumber(lua_State *L, const char *s);

#define luaL_tolstring COMPAT53_CONCAT(COMPAT53_PREFIX, L_tolstring)
COMPAT53_API const char *luaL_tolstring(lua_State *L, int idx, size_t *len);

#define luaL_getmetafield(L, o, e) \
  (luaL_getmetafield((L), (o), (e)) ? lua_type((L), -1) : LUA_TNIL)

#define luaL_newmetatable(L, tn) \
  (luaL_newmetatable((L), (tn)) ? (lua_pushstring((L), (tn)), lua_setfield((L), -2, "__name"), 1) : 0)

#define luaL_requiref COMPAT53_CONCAT(COMPAT53_PREFIX, L_requiref_53)
COMPAT53_API void luaL_requiref(lua_State *L, const char *modname,
	lua_CFunction openf, int glb);

#endif /* Lua 5.1 and Lua 5.2 */

/* declarations for Lua 5.2 */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 502

/* XXX not implemented:
* lua_isyieldable
* lua_getextraspace
* lua_arith (new operators)
* lua_pushfstring (new formats)
*/

#define lua_getglobal(L, n) \
  (lua_getglobal((L), (n)), lua_type((L), -1))

#define lua_getuservalue(L, i) \
  (lua_getuservalue((L), (i)), lua_type((L), -1))

#define lua_pushlstring(L, s, len) \
  (((len) == 0) ? lua_pushlstring((L), "", 0) : lua_pushlstring((L), (s), (len)))

#define lua_rawgetp(L, i, p) \
  (lua_rawgetp((L), (i), (p)), lua_type((L), -1))

#define LUA_KFUNCTION(_name) \
  static int (_name)(lua_State *L, int status, lua_KContext ctx); \
  static int (_name ## _52)(lua_State *L) { \
    lua_KContext ctx; \
    int status = lua_getctx(L, &ctx); \
    return (_name)(L, status, ctx); \
  } \
  static int (_name)(lua_State *L, int status, lua_KContext ctx)

#define lua_pcallk(L, na, nr, err, ctx, cont) \
  lua_pcallk((L), (na), (nr), (err), (ctx), cont ## _52)

#define lua_callk(L, na, nr, ctx, cont) \
  lua_callk((L), (na), (nr), (ctx), cont ## _52)

#define lua_yieldk(L, nr, ctx, cont) \
  lua_yieldk((L), (nr), (ctx), cont ## _52)

#ifdef lua_call
#  undef lua_call
#  define lua_call(L, na, nr) \
  (lua_callk)((L), (na), (nr), 0, NULL)
#endif

#ifdef lua_pcall
#  undef lua_pcall
#  define lua_pcall(L, na, nr, err) \
  (lua_pcallk)((L), (na), (nr), (err), 0, NULL)
#endif

#ifdef lua_yield
#  undef lua_yield
#  define lua_yield(L, nr) \
  (lua_yieldk)((L), (nr), 0, NULL)
#endif

#endif /* Lua 5.2 only */

/* other Lua versions */
#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 501 || LUA_VERSION_NUM > 504

#  error "unsupported Lua version (i.e. not Lua 5.1, 5.2, or 5.3)"

#endif /* other Lua versions except 5.1, 5.2, and 5.3 */

/* helper macro for defining continuation functions (for every version
* *except* Lua 5.2) */
#ifndef LUA_KFUNCTION
#define LUA_KFUNCTION(_name) \
  static int (_name)(lua_State *L, int status, lua_KContext ctx)
#endif

#if defined(COMPAT53_INCLUDE_SOURCE) && COMPAT53_INCLUDE_SOURCE == 1
// beginning of sol/compatibility/compat-5.3.c

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

/* don't compile it again if it already is included via compat53.h */
#ifndef KEPLER_PROJECT_COMPAT53_C_
#define KEPLER_PROJECT_COMPAT53_C_

/* definitions for Lua 5.1 only */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501

#ifndef COMPAT53_FOPEN_NO_LOCK
#  if defined(_MSC_VER)
#    define COMPAT53_FOPEN_NO_LOCK 1
#  else /* otherwise */
#    define COMPAT53_FOPEN_NO_LOCK 0
#  endif /* VC++ only so far */
#endif /* No-lock fopen_s usage if possible */

#if defined(_MSC_VER) && COMPAT53_FOPEN_NO_LOCK
#  include <share.h>
#endif /* VC++ _fsopen for share-allowed file read */

#ifndef COMPAT53_HAVE_STRERROR_R
#  if defined(__GLIBC__) || defined(_POSIX_VERSION) || defined(__APPLE__) || \
      (!defined (__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 6))
#    define COMPAT53_HAVE_STRERROR_R 1
#  else /* none of the defines matched: define to 0 */
#    define COMPAT53_HAVE_STRERROR_R 0
#  endif /* have strerror_r of some form */
#endif /* strerror_r */

#ifndef COMPAT53_HAVE_STRERROR_S
#  if defined(_MSC_VER) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
      (defined(__STDC_LIB_EXT1__) && __STDC_LIB_EXT1__)
#    define COMPAT53_HAVE_STRERROR_S 1
#  else /* not VC++ or C11 */
#    define COMPAT53_HAVE_STRERROR_S 0
#  endif /* strerror_s from VC++ or C11 */
#endif /* strerror_s */

#ifndef COMPAT53_LUA_FILE_BUFFER_SIZE
#  define COMPAT53_LUA_FILE_BUFFER_SIZE 4096
#endif /* Lua File Buffer Size */

static char* compat53_strerror(int en, char* buff, size_t sz) {
#if COMPAT53_HAVE_STRERROR_R
	/* use strerror_r here, because it's available on these specific platforms */
	if (sz > 0) {
		buff[0] = '\0';
		/* we don't care whether the GNU version or the XSI version is used: */
		if (strerror_r(en, buff, sz)) {
			/* Yes, we really DO want to ignore the return value!
			* GCC makes that extra hard, not even a (void) cast will do. */
		}
		if (buff[0] == '\0') {
			/* Buffer is unchanged, so we probably have called GNU strerror_r which
			* returned a static constant string. Chances are that strerror will
			* return the same static constant string and therefore be thread-safe. */
			return strerror(en);
		}
	}
	return buff; /* sz is 0 *or* strerror_r wrote into the buffer */
#elif COMPAT53_HAVE_STRERROR_S
	/* for MSVC and other C11 implementations, use strerror_s since it's
	* provided by default by the libraries */
	strerror_s(buff, sz, en);
	return buff;
#else
	/* fallback, but strerror is not guaranteed to be threadsafe due to modifying
	* errno itself and some impls not locking a static buffer for it ... but most
	* known systems have threadsafe errno: this might only change if the locale
	* is changed out from under someone while this function is being called */
	(void)buff;
	(void)sz;
	return strerror(en);
#endif
}

COMPAT53_API int lua_absindex(lua_State *L, int i) {
	if (i < 0 && i > LUA_REGISTRYINDEX)
		i += lua_gettop(L) + 1;
	return i;
}

static void compat53_call_lua(lua_State *L, char const code[], size_t len,
	int nargs, int nret) {
	lua_rawgetp(L, LUA_REGISTRYINDEX, (void*)code);
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 1);
		if (luaL_loadbuffer(L, code, len, "=none"))
			lua_error(L);
		lua_pushvalue(L, -1);
		lua_rawsetp(L, LUA_REGISTRYINDEX, (void*)code);
	}
	lua_insert(L, -nargs - 1);
	lua_call(L, nargs, nret);
}

static const char compat53_arith_code[] =
"local op,a,b=...\n"
"if op==0 then return a+b\n"
"elseif op==1 then return a-b\n"
"elseif op==2 then return a*b\n"
"elseif op==3 then return a/b\n"
"elseif op==4 then return a%b\n"
"elseif op==5 then return a^b\n"
"elseif op==6 then return -a\n"
"end\n";

COMPAT53_API void lua_arith(lua_State *L, int op) {
	if (op < LUA_OPADD || op > LUA_OPUNM)
		luaL_error(L, "invalid 'op' argument for lua_arith");
	luaL_checkstack(L, 5, "not enough stack slots");
	if (op == LUA_OPUNM)
		lua_pushvalue(L, -1);
	lua_pushnumber(L, op);
	lua_insert(L, -3);
	compat53_call_lua(L, compat53_arith_code,
		sizeof(compat53_arith_code) - 1, 3, 1);
}

static const char compat53_compare_code[] =
"local a,b=...\n"
"return a<=b\n";

COMPAT53_API int lua_compare(lua_State *L, int idx1, int idx2, int op) {
	int result = 0;
	switch (op) {
	case LUA_OPEQ:
		return lua_equal(L, idx1, idx2);
	case LUA_OPLT:
		return lua_lessthan(L, idx1, idx2);
	case LUA_OPLE:
		luaL_checkstack(L, 5, "not enough stack slots");
		idx1 = lua_absindex(L, idx1);
		idx2 = lua_absindex(L, idx2);
		lua_pushvalue(L, idx1);
		lua_pushvalue(L, idx2);
		compat53_call_lua(L, compat53_compare_code,
			sizeof(compat53_compare_code) - 1, 2, 1);
		result = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return result;
	default:
		luaL_error(L, "invalid 'op' argument for lua_compare");
	}
	return 0;
}

COMPAT53_API void lua_copy(lua_State *L, int from, int to) {
	int abs_to = lua_absindex(L, to);
	luaL_checkstack(L, 1, "not enough stack slots");
	lua_pushvalue(L, from);
	lua_replace(L, abs_to);
}

COMPAT53_API void lua_len(lua_State *L, int i) {
	switch (lua_type(L, i)) {
	case LUA_TSTRING:
		lua_pushnumber(L, (lua_Number)lua_objlen(L, i));
		break;
	case LUA_TTABLE:
		if (!luaL_callmeta(L, i, "__len"))
			lua_pushnumber(L, (lua_Number)lua_objlen(L, i));
		break;
	case LUA_TUSERDATA:
		if (luaL_callmeta(L, i, "__len"))
			break;
		/* FALLTHROUGH */
	default:
		luaL_error(L, "attempt to get length of a %s value",
			lua_typename(L, lua_type(L, i)));
	}
}

COMPAT53_API int lua_rawgetp(lua_State *L, int i, const void *p) {
	int abs_i = lua_absindex(L, i);
	lua_pushlightuserdata(L, (void*)p);
	lua_rawget(L, abs_i);
	return lua_type(L, -1);
}

COMPAT53_API void lua_rawsetp(lua_State *L, int i, const void *p) {
	int abs_i = lua_absindex(L, i);
	luaL_checkstack(L, 1, "not enough stack slots");
	lua_pushlightuserdata(L, (void*)p);
	lua_insert(L, -2);
	lua_rawset(L, abs_i);
}

COMPAT53_API lua_Number lua_tonumberx(lua_State *L, int i, int *isnum) {
	lua_Number n = lua_tonumber(L, i);
	if (isnum != NULL) {
		*isnum = (n != 0 || lua_isnumber(L, i));
	}
	return n;
}

COMPAT53_API void luaL_checkversion(lua_State *L) {
	(void)L;
}

COMPAT53_API void luaL_checkstack(lua_State *L, int sp, const char *msg) {
	if (!lua_checkstack(L, sp + LUA_MINSTACK)) {
		if (msg != NULL)
			luaL_error(L, "stack overflow (%s)", msg);
		else {
			lua_pushliteral(L, "stack overflow");
			lua_error(L);
		}
	}
}

COMPAT53_API int luaL_getsubtable(lua_State *L, int i, const char *name) {
	int abs_i = lua_absindex(L, i);
	luaL_checkstack(L, 3, "not enough stack slots");
	lua_pushstring(L, name);
	lua_gettable(L, abs_i);
	if (lua_istable(L, -1))
		return 1;
	lua_pop(L, 1);
	lua_newtable(L);
	lua_pushstring(L, name);
	lua_pushvalue(L, -2);
	lua_settable(L, abs_i);
	return 0;
}

COMPAT53_API lua_Integer luaL_len(lua_State *L, int i) {
	lua_Integer res = 0;
	int isnum = 0;
	luaL_checkstack(L, 1, "not enough stack slots");
	lua_len(L, i);
	res = lua_tointegerx(L, -1, &isnum);
	lua_pop(L, 1);
	if (!isnum)
		luaL_error(L, "object length is not an integer");
	return res;
}

COMPAT53_API void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup + 1, "too many upvalues");
	for (; l->name != NULL; l++) {  /* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)  /* copy upvalues to the top */
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
		lua_settable(L, -(nup + 3)); /* table must be below the upvalues, the name and the closure */
	}
	lua_pop(L, nup);  /* remove upvalues */
}

COMPAT53_API void luaL_setmetatable(lua_State *L, const char *tname) {
	luaL_checkstack(L, 1, "not enough stack slots");
	luaL_getmetatable(L, tname);
	lua_setmetatable(L, -2);
}

COMPAT53_API void *luaL_testudata(lua_State *L, int i, const char *tname) {
	void *p = lua_touserdata(L, i);
	luaL_checkstack(L, 2, "not enough stack slots");
	if (p == NULL || !lua_getmetatable(L, i))
		return NULL;
	else {
		int res = 0;
		luaL_getmetatable(L, tname);
		res = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!res)
			p = NULL;
	}
	return p;
}

static int compat53_countlevels(lua_State *L) {
	lua_Debug ar;
	int li = 1, le = 1;
	/* find an upper bound */
	while (lua_getstack(L, le, &ar)) { li = le; le *= 2; }
	/* do a binary search */
	while (li < le) {
		int m = (li + le) / 2;
		if (lua_getstack(L, m, &ar)) li = m + 1;
		else le = m;
	}
	return le - 1;
}

static int compat53_findfield(lua_State *L, int objidx, int level) {
	if (level == 0 || !lua_istable(L, -1))
		return 0;  /* not found */
	lua_pushnil(L);  /* start 'next' loop */
	while (lua_next(L, -2)) {  /* for each pair in table */
		if (lua_type(L, -2) == LUA_TSTRING) {  /* ignore non-string keys */
			if (lua_rawequal(L, objidx, -1)) {  /* found object? */
				lua_pop(L, 1);  /* remove value (but keep name) */
				return 1;
			}
			else if (compat53_findfield(L, objidx, level - 1)) {  /* try recursively */
				lua_remove(L, -2);  /* remove table (but keep name) */
				lua_pushliteral(L, ".");
				lua_insert(L, -2);  /* place '.' between the two names */
				lua_concat(L, 3);
				return 1;
			}
		}
		lua_pop(L, 1);  /* remove value */
	}
	return 0;  /* not found */
}

static int compat53_pushglobalfuncname(lua_State *L, lua_Debug *ar) {
	int top = lua_gettop(L);
	lua_getinfo(L, "f", ar);  /* push function */
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (compat53_findfield(L, top + 1, 2)) {
		lua_copy(L, -1, top + 1);  /* move name to proper place */
		lua_pop(L, 2);  /* remove pushed values */
		return 1;
	}
	else {
		lua_settop(L, top);  /* remove function and global table */
		return 0;
	}
}

static void compat53_pushfuncname(lua_State *L, lua_Debug *ar) {
	if (*ar->namewhat != '\0')  /* is there a name? */
		lua_pushfstring(L, "function " LUA_QS, ar->name);
	else if (*ar->what == 'm')  /* main? */
		lua_pushliteral(L, "main chunk");
	else if (*ar->what == 'C') {
		if (compat53_pushglobalfuncname(L, ar)) {
			lua_pushfstring(L, "function " LUA_QS, lua_tostring(L, -1));
			lua_remove(L, -2);  /* remove name */
		}
		else
			lua_pushliteral(L, "?");
	}
	else
		lua_pushfstring(L, "function <%s:%d>", ar->short_src, ar->linedefined);
}

#define COMPAT53_LEVELS1 12  /* size of the first part of the stack */
#define COMPAT53_LEVELS2 10  /* size of the second part of the stack */

COMPAT53_API void luaL_traceback(lua_State *L, lua_State *L1,
	const char *msg, int level) {
	lua_Debug ar;
	int top = lua_gettop(L);
	int numlevels = compat53_countlevels(L1);
	int mark = (numlevels > COMPAT53_LEVELS1 + COMPAT53_LEVELS2) ? COMPAT53_LEVELS1 : 0;
	if (msg) lua_pushfstring(L, "%s\n", msg);
	lua_pushliteral(L, "stack traceback:");
	while (lua_getstack(L1, level++, &ar)) {
		if (level == mark) {  /* too many levels? */
			lua_pushliteral(L, "\n\t...");  /* add a '...' */
			level = numlevels - COMPAT53_LEVELS2;  /* and skip to last ones */
		}
		else {
			lua_getinfo(L1, "Slnt", &ar);
			lua_pushfstring(L, "\n\t%s:", ar.short_src);
			if (ar.currentline > 0)
				lua_pushfstring(L, "%d:", ar.currentline);
			lua_pushliteral(L, " in ");
			compat53_pushfuncname(L, &ar);
			lua_concat(L, lua_gettop(L) - top);
		}
	}
	lua_concat(L, lua_gettop(L) - top);
}

COMPAT53_API int luaL_fileresult(lua_State *L, int stat, const char *fname) {
	const char *serr = NULL;
	int en = errno;  /* calls to Lua API may change this value */
	char buf[512] = { 0 };
	if (stat) {
		lua_pushboolean(L, 1);
		return 1;
	}
	else {
		lua_pushnil(L);
		serr = compat53_strerror(en, buf, sizeof(buf));
		if (fname)
			lua_pushfstring(L, "%s: %s", fname, serr);
		else
			lua_pushstring(L, serr);
		lua_pushnumber(L, (lua_Number)en);
		return 3;
	}
}

static int compat53_checkmode(lua_State *L, const char *mode, const char *modename, int err) {
	if (mode && strchr(mode, modename[0]) == NULL) {
		lua_pushfstring(L, "attempt to load a %s chunk (mode is '%s')", modename, mode);
		return err;
	}
	return LUA_OK;
}

typedef struct {
	lua_Reader reader;
	void *ud;
	int has_peeked_data;
	const char *peeked_data;
	size_t peeked_data_size;
} compat53_reader_data;

static const char *compat53_reader(lua_State *L, void *ud, size_t *size) {
	compat53_reader_data *data = (compat53_reader_data *)ud;
	if (data->has_peeked_data) {
		data->has_peeked_data = 0;
		*size = data->peeked_data_size;
		return data->peeked_data;
	}
	else
		return data->reader(L, data->ud, size);
}

COMPAT53_API int lua_load(lua_State *L, lua_Reader reader, void *data, const char *source, const char *mode) {
	int status = LUA_OK;
	compat53_reader_data compat53_data = { reader, data, 1, 0, 0 };
	compat53_data.peeked_data = reader(L, data, &(compat53_data.peeked_data_size));
	if (compat53_data.peeked_data && compat53_data.peeked_data_size &&
		compat53_data.peeked_data[0] == LUA_SIGNATURE[0]) /* binary file? */
		status = compat53_checkmode(L, mode, "binary", LUA_ERRSYNTAX);
	else
		status = compat53_checkmode(L, mode, "text", LUA_ERRSYNTAX);
	if (status != LUA_OK)
		return status;
	/* we need to call the original 5.1 version of lua_load! */
#undef lua_load
	return lua_load(L, compat53_reader, &compat53_data, source);
#define lua_load COMPAT53_CONCAT(COMPAT53_PREFIX, _load_53)
}

typedef struct {
	int n;  /* number of pre-read characters */
	FILE *f;  /* file being read */
	char buff[COMPAT53_LUA_FILE_BUFFER_SIZE];  /* area for reading file */
} compat53_LoadF;

static const char *compat53_getF(lua_State *L, void *ud, size_t *size) {
	compat53_LoadF *lf = (compat53_LoadF *)ud;
	(void)L;  /* not used */
	if (lf->n > 0) {  /* are there pre-read characters to be read? */
		*size = lf->n;  /* return them (chars already in buffer) */
		lf->n = 0;  /* no more pre-read characters */
	}
	else {  /* read a block from file */
		   /* 'fread' can return > 0 *and* set the EOF flag. If next call to
		   'compat53_getF' called 'fread', it might still wait for user input.
		   The next check avoids this problem. */
		if (feof(lf->f)) return NULL;
		*size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);  /* read block */
	}
	return lf->buff;
}

static int compat53_errfile(lua_State *L, const char *what, int fnameindex) {
	char buf[512] = { 0 };
	const char *serr = compat53_strerror(errno, buf, sizeof(buf));
	const char *filename = lua_tostring(L, fnameindex) + 1;
	lua_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
	lua_remove(L, fnameindex);
	return LUA_ERRFILE;
}

static int compat53_skipBOM(compat53_LoadF *lf) {
	const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
	int c;
	lf->n = 0;
	do {
		c = getc(lf->f);
		if (c == EOF || c != *(const unsigned char *)p++) return c;
		lf->buff[lf->n++] = (char)c;  /* to be read by the parser */
	} while (*p != '\0');
	lf->n = 0;  /* prefix matched; discard it */
	return getc(lf->f);  /* return next character */
}

/*
** reads the first character of file 'f' and skips an optional BOM mark
** in its beginning plus its first line if it starts with '#'. Returns
** true if it skipped the first line.  In any case, '*cp' has the
** first "valid" character of the file (after the optional BOM and
** a first-line comment).
*/
static int compat53_skipcomment(compat53_LoadF *lf, int *cp) {
	int c = *cp = compat53_skipBOM(lf);
	if (c == '#') {  /* first line is a comment (Unix exec. file)? */
		do {  /* skip first line */
			c = getc(lf->f);
		} while (c != EOF && c != '\n');
		*cp = getc(lf->f);  /* skip end-of-line, if present */
		return 1;  /* there was a comment */
	}
	else return 0;  /* no comment */
}

COMPAT53_API int luaL_loadfilex(lua_State *L, const char *filename, const char *mode) {
	compat53_LoadF lf;
	int status, readstatus;
	int c;
	int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */
	if (filename == NULL) {
		lua_pushliteral(L, "=stdin");
		lf.f = stdin;
	}
	else {
		lua_pushfstring(L, "@%s", filename);
#if defined(_MSC_VER)
		/* This code is here to stop a deprecation error that stops builds
		* if a certain macro is defined. While normally not caring would
		* be best, some header-only libraries and builds can't afford to
		* dictate this to the user. A quick check shows that fopen_s this
		* goes back to VS 2005, and _fsopen goes back to VS 2003 .NET,
		* possibly even before that so we don't need to do any version
		* number checks, since this has been there since forever.  */

		/* TO USER: if you want the behavior of typical fopen_s/fopen,
		* which does lock the file on VC++, define the macro used below to 0 */
#if COMPAT53_FOPEN_NO_LOCK
		lf.f = _fsopen(filename, "r", _SH_DENYNO); /* do not lock the file in any way */
		if (lf.f == NULL)
			return compat53_errfile(L, "open", fnameindex);
#else /* use default locking version */
		if (fopen_s(&lf.f, filename, "r") != 0)
			return compat53_errfile(L, "open", fnameindex);
#endif /* Locking vs. No-locking fopen variants */
#else
		lf.f = fopen(filename, "r"); /* default stdlib doesn't forcefully lock files here */
		if (lf.f == NULL) return compat53_errfile(L, "open", fnameindex);
#endif
	}
	if (compat53_skipcomment(&lf, &c))  /* read initial portion */
		lf.buff[lf.n++] = '\n';  /* add line to correct line numbers */
	if (c == LUA_SIGNATURE[0] && filename) {  /* binary file? */
#if defined(_MSC_VER)
		if (freopen_s(&lf.f, filename, "rb", lf.f) != 0)
			return compat53_errfile(L, "reopen", fnameindex);
#else
		lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
		if (lf.f == NULL) return compat53_errfile(L, "reopen", fnameindex);
#endif
		compat53_skipcomment(&lf, &c);  /* re-read initial portion */
	}
	if (c != EOF)
		lf.buff[lf.n++] = (char)c;  /* 'c' is the first character of the stream */
	status = lua_load(L, &compat53_getF, &lf, lua_tostring(L, -1), mode);
	readstatus = ferror(lf.f);
	if (filename) fclose(lf.f);  /* close file (even in case of errors) */
	if (readstatus) {
		lua_settop(L, fnameindex);  /* ignore results from 'lua_load' */
		return compat53_errfile(L, "read", fnameindex);
	}
	lua_remove(L, fnameindex);
	return status;
}

COMPAT53_API int luaL_loadbufferx(lua_State *L, const char *buff, size_t sz, const char *name, const char *mode) {
	int status = LUA_OK;
	if (sz > 0 && buff[0] == LUA_SIGNATURE[0]) {
		status = compat53_checkmode(L, mode, "binary", LUA_ERRSYNTAX);
	}
	else {
		status = compat53_checkmode(L, mode, "text", LUA_ERRSYNTAX);
	}
	if (status != LUA_OK)
		return status;
	return luaL_loadbuffer(L, buff, sz, name);
}

#if !defined(l_inspectstat) && \
    (defined(unix) || defined(__unix) || defined(__unix__) || \
     defined(__TOS_AIX__) || defined(_SYSTYPE_BSD) || \
     (defined(__APPLE__) && defined(__MACH__)))
/* some form of unix; check feature macros in unistd.h for details */
#  include <unistd.h>
/* check posix version; the relevant include files and macros probably
* were available before 2001, but I'm not sure */
#  if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#    include <sys/wait.h>
#    define l_inspectstat(stat,what) \
  if (WIFEXITED(stat)) { stat = WEXITSTATUS(stat); } \
  else if (WIFSIGNALED(stat)) { stat = WTERMSIG(stat); what = "signal"; }
#  endif
#endif

/* provide default (no-op) version */
#if !defined(l_inspectstat)
#  define l_inspectstat(stat,what) ((void)0)
#endif

COMPAT53_API int luaL_execresult(lua_State *L, int stat) {
	const char *what = "exit";
	if (stat == -1)
		return luaL_fileresult(L, 0, NULL);
	else {
		l_inspectstat(stat, what);
		if (*what == 'e' && stat == 0)
			lua_pushboolean(L, 1);
		else
			lua_pushnil(L);
		lua_pushstring(L, what);
		lua_pushinteger(L, stat);
		return 3;
	}
}

COMPAT53_API void luaL_buffinit(lua_State *L, luaL_Buffer_53 *B) {
	/* make it crash if used via pointer to a 5.1-style luaL_Buffer */
	B->b.p = NULL;
	B->b.L = NULL;
	B->b.lvl = 0;
	/* reuse the buffer from the 5.1-style luaL_Buffer though! */
	B->ptr = B->b.buffer;
	B->capacity = LUAL_BUFFERSIZE;
	B->nelems = 0;
	B->L2 = L;
}

COMPAT53_API char *luaL_prepbuffsize(luaL_Buffer_53 *B, size_t s) {
	if (B->capacity - B->nelems < s) { /* needs to grow */
		char* newptr = NULL;
		size_t newcap = B->capacity * 2;
		if (newcap - B->nelems < s)
			newcap = B->nelems + s;
		if (newcap < B->capacity) /* overflow */
			luaL_error(B->L2, "buffer too large");
		newptr = (char*)lua_newuserdata(B->L2, newcap);
		memcpy(newptr, B->ptr, B->nelems);
		if (B->ptr != B->b.buffer)
			lua_replace(B->L2, -2); /* remove old buffer */
		B->ptr = newptr;
		B->capacity = newcap;
	}
	return B->ptr + B->nelems;
}

COMPAT53_API void luaL_addlstring(luaL_Buffer_53 *B, const char *s, size_t l) {
	memcpy(luaL_prepbuffsize(B, l), s, l);
	luaL_addsize(B, l);
}

COMPAT53_API void luaL_addvalue(luaL_Buffer_53 *B) {
	size_t len = 0;
	const char *s = lua_tolstring(B->L2, -1, &len);
	if (!s)
		luaL_error(B->L2, "cannot convert value to string");
	if (B->ptr != B->b.buffer)
		lua_insert(B->L2, -2); /* userdata buffer must be at stack top */
	luaL_addlstring(B, s, len);
	lua_remove(B->L2, B->ptr != B->b.buffer ? -2 : -1);
}

void luaL_pushresult(luaL_Buffer_53 *B) {
	lua_pushlstring(B->L2, B->ptr, B->nelems);
	if (B->ptr != B->b.buffer)
		lua_replace(B->L2, -2); /* remove userdata buffer */
}

#endif /* Lua 5.1 */

/* definitions for Lua 5.1 and Lua 5.2 */
#if defined( LUA_VERSION_NUM ) && LUA_VERSION_NUM <= 502

COMPAT53_API int lua_geti(lua_State *L, int index, lua_Integer i) {
	index = lua_absindex(L, index);
	lua_pushinteger(L, i);
	lua_gettable(L, index);
	return lua_type(L, -1);
}

COMPAT53_API int lua_isinteger(lua_State *L, int index) {
	if (lua_type(L, index) == LUA_TNUMBER) {
		lua_Number n = lua_tonumber(L, index);
		lua_Integer i = lua_tointeger(L, index);
		if (i == n)
			return 1;
	}
	return 0;
}

COMPAT53_API lua_Integer lua_tointegerx(lua_State *L, int i, int *isnum) {
	int ok = 0;
	lua_Number n = lua_tonumberx(L, i, &ok);
	if (ok) {
		if (n == (lua_Integer)n) {
			if (isnum)
				*isnum = 1;
			return (lua_Integer)n;
		}
	}
	if (isnum)
		*isnum = 0;
	return 0;
}

static void compat53_reverse(lua_State *L, int a, int b) {
	for (; a < b; ++a, --b) {
		lua_pushvalue(L, a);
		lua_pushvalue(L, b);
		lua_replace(L, a);
		lua_replace(L, b);
	}
}

COMPAT53_API void lua_rotate(lua_State *L, int idx, int n) {
	int n_elems = 0;
	idx = lua_absindex(L, idx);
	n_elems = lua_gettop(L) - idx + 1;
	if (n < 0)
		n += n_elems;
	if (n > 0 && n < n_elems) {
		luaL_checkstack(L, 2, "not enough stack slots available");
		n = n_elems - n;
		compat53_reverse(L, idx, idx + n - 1);
		compat53_reverse(L, idx + n, idx + n_elems - 1);
		compat53_reverse(L, idx, idx + n_elems - 1);
	}
}

COMPAT53_API void lua_seti(lua_State *L, int index, lua_Integer i) {
	luaL_checkstack(L, 1, "not enough stack slots available");
	index = lua_absindex(L, index);
	lua_pushinteger(L, i);
	lua_insert(L, -2);
	lua_settable(L, index);
}

#if !defined(lua_str2number)
#  define lua_str2number(s, p)  strtod((s), (p))
#endif

COMPAT53_API size_t lua_stringtonumber(lua_State *L, const char *s) {
	char* endptr;
	lua_Number n = lua_str2number(s, &endptr);
	if (endptr != s) {
		while (*endptr != '\0' && isspace((unsigned char)*endptr))
			++endptr;
		if (*endptr == '\0') {
			lua_pushnumber(L, n);
			return endptr - s + 1;
		}
	}
	return 0;
}

COMPAT53_API const char *luaL_tolstring(lua_State *L, int idx, size_t *len) {
	if (!luaL_callmeta(L, idx, "__tostring")) {
		int t = lua_type(L, idx), tt = 0;
		char const* name = NULL;
		switch (t) {
		case LUA_TNIL:
			lua_pushliteral(L, "nil");
			break;
		case LUA_TSTRING:
		case LUA_TNUMBER:
			lua_pushvalue(L, idx);
			break;
		case LUA_TBOOLEAN:
			if (lua_toboolean(L, idx))
				lua_pushliteral(L, "true");
			else
				lua_pushliteral(L, "false");
			break;
		default:
			tt = luaL_getmetafield(L, idx, "__name");
			name = (tt == LUA_TSTRING) ? lua_tostring(L, -1) : lua_typename(L, t);
			lua_pushfstring(L, "%s: %p", name, lua_topointer(L, idx));
			if (tt != LUA_TNIL)
				lua_replace(L, -2);
			break;
		}
	}
	else {
		if (!lua_isstring(L, -1))
			luaL_error(L, "'__tostring' must return a string");
	}
	return lua_tolstring(L, -1, len);
}

COMPAT53_API void luaL_requiref(lua_State *L, const char *modname,
	lua_CFunction openf, int glb) {
	luaL_checkstack(L, 3, "not enough stack slots available");
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
	if (lua_getfield(L, -1, modname) == LUA_TNIL) {
		lua_pop(L, 1);
		lua_pushcfunction(L, openf);
		lua_pushstring(L, modname);
		lua_call(L, 1, 1);
		lua_pushvalue(L, -1);
		lua_setfield(L, -3, modname);
	}
	if (glb) {
		lua_pushvalue(L, -1);
		lua_setglobal(L, modname);
	}
	lua_replace(L, -2);
}

#endif /* Lua 5.1 and 5.2 */

#endif /* KEPLER_PROJECT_COMPAT53_C_ */

/*********************************************************************
* This file contains parts of Lua 5.2's and Lua 5.3's source code:
*
* Copyright (C) 1994-2014 Lua.org, PUC-Rio.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

// end of sol/compatibility/compat-5.3.c

#endif

#endif /* KEPLER_PROJECT_COMPAT53_H_ */

// end of sol/compatibility/compat-5.3.h


#endif // SOL_NO_COMPAT

// end of sol/compatibility.hpp

// beginning of sol/in_place.hpp

#include <utility>

namespace sol {

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	using in_place_t = std::in_place_t;
	constexpr std::in_place_t in_place{};
	constexpr std::in_place_t in_place_of{};

	template <typename T>
	using in_place_type_t = std::in_place_type_t<T>;
	template <typename T>
	constexpr std::in_place_type_t<T> in_place_type{};

	template <size_t I>
	using in_place_index_t = std::in_place_index_t<I>;
	template <size_t I>
	constexpr in_place_index_t<I> in_place_index{};
#else
	namespace detail {
		struct in_place_of_tag {};
		template <std::size_t I>
		struct in_place_of_i {};
		template <typename T>
		struct in_place_of_t {};
	} // namespace detail

	struct in_place_tag {
		constexpr in_place_tag() = default;
	};

	constexpr inline in_place_tag in_place(detail::in_place_of_tag) {
		return in_place_tag();
	}
	template <typename T>
	constexpr inline in_place_tag in_place(detail::in_place_of_t<T>) {
		return in_place_tag();
	}
	template <std::size_t I>
	constexpr inline in_place_tag in_place(detail::in_place_of_i<I>) {
		return in_place_tag();
	}

	constexpr inline in_place_tag in_place_of(detail::in_place_of_tag) {
		return in_place_tag();
	}
	template <typename T>
	constexpr inline in_place_tag in_place_type(detail::in_place_of_t<T>) {
		return in_place_tag();
	}
	template <std::size_t I>
	constexpr inline in_place_tag in_place_index(detail::in_place_of_i<I>) {
		return in_place_tag();
	}

	using in_place_t = in_place_tag (&)(detail::in_place_of_tag);
	template <typename T>
	using in_place_type_t = in_place_tag (&)(detail::in_place_of_t<T>);
	template <std::size_t I>
	using in_place_index_t = in_place_tag (&)(detail::in_place_of_i<I>);
#endif

} // namespace sol

// end of sol/in_place.hpp

#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
#include <boost/optional.hpp>
#else
// beginning of sol/optional_implementation.hpp

#include <initializer_list>
#include <cassert>
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
#include <cstdlib>
#endif // Exceptions

#define TR2_OPTIONAL_REQUIRES(...) typename ::std::enable_if<__VA_ARGS__::value, bool>::type = false

#if defined __GNUC__ // NOTE: GNUC is also defined for Clang
#if (__GNUC__ >= 5)
#define TR2_OPTIONAL_GCC_5_0_AND_HIGHER___
#define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#elif (__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)
#define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#elif (__GNUC__ > 4)
#define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#endif
#
#if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)
#define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#elif (__GNUC__ > 4)
#define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#endif
#
#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 8) && (__GNUC_PATCHLEVEL__ >= 1)
#define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#elif (__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)
#define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#elif (__GNUC__ > 4)
#define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#endif
#endif
#
#if defined __clang_major__
#if (__clang_major__ == 3 && __clang_minor__ >= 5)
#define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#elif (__clang_major__ > 3)
#define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#endif
#if defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#elif (__clang_major__ == 3 && __clang_minor__ == 4 && __clang_patchlevel__ >= 2)
#define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#endif
#endif
#
#if defined _MSC_VER
#if (_MSC_VER >= 1900)
#define TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#endif
#endif

#if defined __clang__
#if (__clang_major__ > 2) || (__clang_major__ == 2) && (__clang_minor__ >= 9)
#define OPTIONAL_HAS_THIS_RVALUE_REFS 1
#else
#define OPTIONAL_HAS_THIS_RVALUE_REFS 0
#endif
#elif defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#define OPTIONAL_HAS_THIS_RVALUE_REFS 1
#elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#define OPTIONAL_HAS_THIS_RVALUE_REFS 1
#else
#define OPTIONAL_HAS_THIS_RVALUE_REFS 0
#endif

#if defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 1
#define OPTIONAL_CONSTEXPR_INIT_LIST constexpr
#else
#define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 0
#define OPTIONAL_CONSTEXPR_INIT_LIST
#endif

#if defined(TR2_OPTIONAL_MSVC_2015_AND_HIGHER___) || (defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_ && (defined __cplusplus) && (__cplusplus != 201103L))
#define OPTIONAL_HAS_MOVE_ACCESSORS 1
#else
#define OPTIONAL_HAS_MOVE_ACCESSORS 0
#endif

#// In C++11 constexpr implies const, so we need to make non-const members also non-constexpr
#if defined(TR2_OPTIONAL_MSVC_2015_AND_HIGHER___) || ((defined __cplusplus) && (__cplusplus == 201103L))
#define OPTIONAL_MUTABLE_CONSTEXPR
#else
#define OPTIONAL_MUTABLE_CONSTEXPR constexpr
#endif

#if defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#pragma warning(push)
#pragma warning(disable : 4814)
#endif

namespace sol {

	// BEGIN workaround for missing is_trivially_destructible
#if defined TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
	// leave it: it is already there
#elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
	// leave it: it is already there
#elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
	// leave it: it is already there
#elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
	// leave it: the user doesn't want it
#else
	template <typename T>
	using is_trivially_destructible = ::std::has_trivial_destructor<T>;
#endif
	// END workaround for missing is_trivially_destructible

#if (defined TR2_OPTIONAL_GCC_4_7_AND_HIGHER___)
	// leave it; our metafunctions are already defined.
#elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
	// leave it; our metafunctions are already defined.
#elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
	// leave it: it is already there
#elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
	// leave it: the user doesn't want it
#else

	// workaround for missing traits in GCC and CLANG
	template <class T>
	struct is_nothrow_move_constructible {
		static constexprbool value = ::std::is_nothrow_constructible<T, T&&>::value;
	};

	template <class T, class U>
	struct is_assignable {
		template <class X, class Y>
		static constexprbool has_assign(...) {
			return false;
		}

		template <class X, class Y, size_t S = sizeof((::std::declval<X>() = ::std::declval<Y>(), true))>
		// the comma operator is necessary for the cases where operator= returns void
		static constexprbool has_assign(bool) {
			return true;
		}

		static constexprbool value = has_assign<T, U>(true);
	};

	template <class T>
	struct is_nothrow_move_assignable {
		template <class X, bool has_any_move_assign>
		struct has_nothrow_move_assign {
			static constexprbool value = false;
		};

		template <class X>
		struct has_nothrow_move_assign<X, true> {
			static constexprbool value = noexcept(::std::declval<X&>() = ::std::declval<X&&>());
		};

		static constexprbool value = has_nothrow_move_assign<T, is_assignable<T&, T&&>::value>::value;
	};
	// end workaround

#endif

	// 20.5.4, optional for object types
	template <class T>
	class optional;

	// 20.5.5, optional for lvalue reference types
	template <class T>
	class optional<T&>;

	// workaround: std utility functions aren't constexpr yet
	template <class T>
	inline constexpr T&& constexpr_forward(typename ::std::remove_reference<T>::type& t) noexcept {
		return static_cast<T&&>(t);
	}

	template <class T>
	inline constexpr T&& constexpr_forward(typename ::std::remove_reference<T>::type&& t) noexcept {
		static_assert(!::std::is_lvalue_reference<T>::value, "!!");
		return static_cast<T&&>(t);
	}

	template <class T>
	inline constexpr typename ::std::remove_reference<T>::type&& constexpr_move(T&& t) noexcept {
		return static_cast<typename ::std::remove_reference<T>::type&&>(t);
	}

#if defined NDEBUG
#define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) (EXPR)
#else
#define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) ((CHECK) ? (EXPR) : ([] { assert(!#CHECK); }(), (EXPR)))
#endif

	namespace detail_ {

		// static_addressof: a constexpr version of addressof
		template <typename T>
		struct has_overloaded_addressof {
			template <class X>
			static constexpr bool has_overload(...) {
				return false;
			}

			template <class X, size_t S = sizeof(::std::declval<X&>().operator&())>
			static constexpr bool has_overload(bool) {
				return true;
			}

			static constexpr bool value = has_overload<T>(true);
		};

		template <typename T, TR2_OPTIONAL_REQUIRES(!has_overloaded_addressof<T>)>
		constexpr T* static_addressof(T& ref) {
			return &ref;
		}

		template <typename T, TR2_OPTIONAL_REQUIRES(has_overloaded_addressof<T>)>
		T* static_addressof(T& ref) {
			return ::std::addressof(ref);
		}

		// the call to convert<A>(b) has return type A and converts b to type A iff b decltype(b) is implicitly convertible to A
		template <class U>
		constexpr U convert(U v) {
			return v;
		}

	} // namespace detail_

	constexpr struct trivial_init_t {
	} trivial_init{};

	// 20.5.7, Disengaged state indicator
	struct nullopt_t {
		struct init {};
		constexpr explicit nullopt_t(init) {
		}
	};
	constexpr nullopt_t nullopt{nullopt_t::init()};

	// 20.5.8, class bad_optional_access
	class bad_optional_access : public ::std::logic_error {
	public:
		explicit bad_optional_access(const ::std::string& what_arg)
		: ::std::logic_error{what_arg} {
		}
		explicit bad_optional_access(const char* what_arg)
		: ::std::logic_error{what_arg} {
		}
	};

	template <class T>
	struct alignas(T) optional_base {
		char storage_[sizeof(T)];
		bool init_;

		constexpr optional_base() noexcept
		: storage_(), init_(false){};

		explicit optional_base(const T& v)
		: storage_(), init_(true) {
			new (&storage()) T(v);
		}

		explicit optional_base(T&& v)
		: storage_(), init_(true) {
			new (&storage()) T(constexpr_move(v));
		}

		template <class... Args>
		explicit optional_base(in_place_t, Args&&... args)
		: init_(true), storage_() {
			new (&storage()) T(constexpr_forward<Args>(args)...);
		}

		template <class U, class... Args, TR2_OPTIONAL_REQUIRES(::std::is_constructible<T, ::std::initializer_list<U>>)>
		explicit optional_base(in_place_t, ::std::initializer_list<U> il, Args&&... args)
		: init_(true), storage_() {
			new (&storage()) T(il, constexpr_forward<Args>(args)...);
		}
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
		T& storage() {
			return *reinterpret_cast<T*>(&storage_[0]);
		}

		constexpr const T& storage() const {
			return *reinterpret_cast<T const*>(&storage_[0]);
		}
#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

		~optional_base() {
			if (init_) {
				storage().T::~T();
			}
		}
	};

#if defined __GNUC__ && !defined TR2_OPTIONAL_GCC_5_0_AND_HIGHER___
	// Sorry, GCC 4.x; you're just a piece of shit
	template <typename T>
	using constexpr_optional_base = optional_base<T>;
#else
	template <class T>
	struct alignas(T) constexpr_optional_base {
		char storage_[sizeof(T)];
		bool init_;
		constexpr constexpr_optional_base() noexcept
		: storage_(), init_(false) {
		}

		explicit constexpr constexpr_optional_base(const T& v)
		: storage_(), init_(true) {
			new (&storage()) T(v);
		}

		explicit constexpr constexpr_optional_base(T&& v)
		: storage_(), init_(true) {
			new (&storage()) T(constexpr_move(v));
		}

		template <class... Args>
		explicit constexpr constexpr_optional_base(in_place_t, Args&&... args)
		: init_(true), storage_() {
			new (&storage()) T(constexpr_forward<Args>(args)...);
		}

		template <class U, class... Args, TR2_OPTIONAL_REQUIRES(::std::is_constructible<T, ::std::initializer_list<U>>)>
		OPTIONAL_CONSTEXPR_INIT_LIST explicit constexpr_optional_base(in_place_t, ::std::initializer_list<U> il, Args&&... args)
		: init_(true), storage_() {
			new (&storage()) T(il, constexpr_forward<Args>(args)...);
		}

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
		T& storage() {
			return (*reinterpret_cast<T*>(&storage_[0]));
		}

		constexpr const T& storage() const {
			return (*reinterpret_cast<T const*>(&storage_[0]));
		}
#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

		~constexpr_optional_base() = default;
	};
#endif

	template <class T>
	using OptionalBase = typename ::std::conditional<
		::std::is_trivially_destructible<T>::value,
		constexpr_optional_base<typename ::std::remove_const<T>::type>,
		optional_base<typename ::std::remove_const<T>::type>>::type;

	template <class T>
	class optional : private OptionalBase<T> {
		static_assert(!::std::is_same<typename ::std::decay<T>::type, nullopt_t>::value, "bad T");
		static_assert(!::std::is_same<typename ::std::decay<T>::type, in_place_t>::value, "bad T");

		constexpr bool initialized() const noexcept {
			return OptionalBase<T>::init_;
		}
		typename ::std::remove_const<T>::type* dataptr() {
			return ::std::addressof(OptionalBase<T>::storage());
		}
		constexpr const T* dataptr() const {
			return detail_::static_addressof(OptionalBase<T>::storage());
		}

#if OPTIONAL_HAS_THIS_RVALUE_REFS == 1
		constexpr const T& contained_val() const& {
			return OptionalBase<T>::storage();
		}
#if OPTIONAL_HAS_MOVE_ACCESSORS == 1
		OPTIONAL_MUTABLE_CONSTEXPR T&& contained_val() && {
			return ::std::move(OptionalBase<T>::storage());
		}
		OPTIONAL_MUTABLE_CONSTEXPR T& contained_val() & {
			return OptionalBase<T>::storage();
		}
#else
		T& contained_val() & {
			return OptionalBase<T>::storage();
		}
		T&& contained_val() && {
			return ::std::move(OptionalBase<T>::storage());
		}
#endif
#else
		constexpr const T& contained_val() const {
			return OptionalBase<T>::storage();
		}
		T& contained_val() {
			return OptionalBase<T>::storage();
		}
#endif

		void clear() noexcept {
			if (initialized())
				dataptr()->T::~T();
			OptionalBase<T>::init_ = false;
		}

		template <class... Args>
		void initialize(Args&&... args) noexcept(noexcept(T(::std::forward<Args>(args)...))) {
			assert(!OptionalBase<T>::init_);
			::new (static_cast<void*>(dataptr())) T(::std::forward<Args>(args)...);
			OptionalBase<T>::init_ = true;
		}

		template <class U, class... Args>
		void initialize(::std::initializer_list<U> il, Args&&... args) noexcept(noexcept(T(il, ::std::forward<Args>(args)...))) {
			assert(!OptionalBase<T>::init_);
			::new (static_cast<void*>(dataptr())) T(il, ::std::forward<Args>(args)...);
			OptionalBase<T>::init_ = true;
		}

	public:
		typedef T value_type;

		// 20.5.5.1, constructors
		constexpr optional() noexcept
		: OptionalBase<T>(){};
		constexpr optional(nullopt_t) noexcept
		: OptionalBase<T>(){};

		optional(const optional& rhs)
		: OptionalBase<T>() {
			if (rhs.initialized()) {
				::new (static_cast<void*>(dataptr())) T(*rhs);
				OptionalBase<T>::init_ = true;
			}
		}

		optional(const optional<T&>& rhs)
		: optional() {
			if (rhs) {
				::new (static_cast<void*>(dataptr())) T(*rhs);
				OptionalBase<T>::init_ = true;
			}
		}

		optional(optional&& rhs) noexcept(::std::is_nothrow_move_constructible<T>::value)
		: OptionalBase<T>() {
			if (rhs.initialized()) {
				::new (static_cast<void*>(dataptr())) T(::std::move(*rhs));
				OptionalBase<T>::init_ = true;
			}
		}

		constexpr optional(const T& v)
		: OptionalBase<T>(v) {
		}

		constexpr optional(T&& v)
		: OptionalBase<T>(constexpr_move(v)) {
		}

		template <class... Args>
		explicit constexpr optional(in_place_t, Args&&... args)
		: OptionalBase<T>(in_place, constexpr_forward<Args>(args)...) {
		}

		template <class U, class... Args, TR2_OPTIONAL_REQUIRES(::std::is_constructible<T, ::std::initializer_list<U>>)>
		OPTIONAL_CONSTEXPR_INIT_LIST explicit optional(in_place_t, ::std::initializer_list<U> il, Args&&... args)
		: OptionalBase<T>(in_place, il, constexpr_forward<Args>(args)...) {
		}

		// 20.5.4.2, Destructor
		~optional() = default;

		// 20.5.4.3, assignment
		optional& operator=(nullopt_t) noexcept {
			clear();
			return *this;
		}

		optional& operator=(const optional& rhs) {
			if (initialized() == true && rhs.initialized() == false)
				clear();
			else if (initialized() == false && rhs.initialized() == true)
				initialize(*rhs);
			else if (initialized() == true && rhs.initialized() == true)
				contained_val() = *rhs;
			return *this;
		}

		optional& operator=(optional&& rhs) noexcept(::std::is_nothrow_move_assignable<T>::value&& ::std::is_nothrow_move_constructible<T>::value) {
			if (initialized() == true && rhs.initialized() == false)
				clear();
			else if (initialized() == false && rhs.initialized() == true)
				initialize(::std::move(*rhs));
			else if (initialized() == true && rhs.initialized() == true)
				contained_val() = ::std::move(*rhs);
			return *this;
		}

		template <class U>
		auto operator=(U&& v)
			-> typename ::std::enable_if<
				::std::is_same<typename ::std::decay<U>::type, T>::value,
				optional&>::type {
			if (initialized()) {
				contained_val() = ::std::forward<U>(v);
			}
			else {
				initialize(::std::forward<U>(v));
			}
			return *this;
		}

		template <class... Args>
		void emplace(Args&&... args) {
			clear();
			initialize(::std::forward<Args>(args)...);
		}

		template <class U, class... Args>
		void emplace(::std::initializer_list<U> il, Args&&... args) {
			clear();
			initialize<U, Args...>(il, ::std::forward<Args>(args)...);
		}

		// 20.5.4.4, Swap
		void swap(optional<T>& rhs) noexcept(::std::is_nothrow_move_constructible<T>::value&& noexcept(swap(::std::declval<T&>(), ::std::declval<T&>()))) {
			if (initialized() == true && rhs.initialized() == false) {
				rhs.initialize(::std::move(**this));
				clear();
			}
			else if (initialized() == false && rhs.initialized() == true) {
				initialize(::std::move(*rhs));
				rhs.clear();
			}
			else if (initialized() == true && rhs.initialized() == true) {
				using ::std::swap;
				swap(**this, *rhs);
			}
		}

		// 20.5.4.5, Observers

		explicit constexpr operator bool() const noexcept {
			return initialized();
		}

		constexpr T const* operator->() const {
			return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), dataptr());
		}

#if OPTIONAL_HAS_MOVE_ACCESSORS == 1

		OPTIONAL_MUTABLE_CONSTEXPR T* operator->() {
			assert(initialized());
			return dataptr();
		}

		constexpr T const& operator*() const& {
			return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
		}

		OPTIONAL_MUTABLE_CONSTEXPR T& operator*() & {
			assert(initialized());
			return contained_val();
		}

		OPTIONAL_MUTABLE_CONSTEXPR T&& operator*() && {
			assert(initialized());
			return constexpr_move(contained_val());
		}

		constexpr T const& value() const& {
			return initialized() ? contained_val()
#ifdef SOL_NO_EXCEPTIONS
							 // we can't abort here
							 // because there's no constexpr abort
							 : *static_cast<T*>(nullptr);
#else
							 : (throw bad_optional_access("bad optional access"), contained_val());
#endif
		}

		OPTIONAL_MUTABLE_CONSTEXPR T& value() & {
			return initialized() ? contained_val()
#ifdef SOL_NO_EXCEPTIONS
							 : *static_cast<T*>(nullptr);
#else
							 : (throw bad_optional_access("bad optional access"), contained_val());
#endif
		}

		OPTIONAL_MUTABLE_CONSTEXPR T&& value() && {
			return initialized() ? contained_val()
#ifdef SOL_NO_EXCEPTIONS
							 // we can't abort here
							 // because there's no constexpr abort
							 : std::move(*static_cast<T*>(nullptr));
#else
							 : (throw bad_optional_access("bad optional access"), contained_val());
#endif
		}

#else

		T* operator->() {
			assert(initialized());
			return dataptr();
		}

		constexpr T const& operator*() const {
			return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
		}

		T& operator*() {
			assert(initialized());
			return contained_val();
		}

		constexpr T const& value() const {
			return initialized() ? contained_val()
#ifdef SOL_NO_EXCEPTIONS
							 // we can't abort here
							 // because there's no constexpr abort
							 : *static_cast<T*>(nullptr);
#else
							 : (throw bad_optional_access("bad optional access"), contained_val());
#endif
		}

		T& value() {
			return initialized() ? contained_val()
#ifdef SOL_NO_EXCEPTIONS
							 // we can abort here
							 // but the others are constexpr, so we can't...
							 : (std::abort(), *static_cast<T*>(nullptr));
#else
							 : (throw bad_optional_access("bad optional access"), contained_val());
#endif
		}

#endif

#if OPTIONAL_HAS_THIS_RVALUE_REFS == 1

		template <class V>
		constexpr T value_or(V&& v) const& {
			return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
		}

#if OPTIONAL_HAS_MOVE_ACCESSORS == 1

		template <class V>
		OPTIONAL_MUTABLE_CONSTEXPR T value_or(V&& v) && {
			return *this ? constexpr_move(const_cast<optional<T>&>(*this).contained_val()) : detail_::convert<T>(constexpr_forward<V>(v));
		}

#else

		template <class V>
		T value_or(V&& v) && {
			return *this ? constexpr_move(const_cast<optional<T>&>(*this).contained_val()) : detail_::convert<T>(constexpr_forward<V>(v));
		}

#endif

#else

		template <class V>
		constexpr T value_or(V&& v) const {
			return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
		}

#endif
	};

	template <class T>
	class optional<T&> {
		static_assert(!::std::is_same<T, nullopt_t>::value, "bad T");
		static_assert(!::std::is_same<T, in_place_t>::value, "bad T");
		T* ref;

	public:
		// 20.5.5.1, construction/destruction
		constexpr optional() noexcept
		: ref(nullptr) {
		}

		constexpr optional(nullopt_t) noexcept
		: ref(nullptr) {
		}

		constexpr optional(T& v) noexcept
		: ref(detail_::static_addressof(v)) {
		}

		optional(T&&) = delete;

		constexpr optional(const optional& rhs) noexcept
		: ref(rhs.ref) {
		}

		explicit constexpr optional(in_place_t, T& v) noexcept
		: ref(detail_::static_addressof(v)) {
		}

		explicit optional(in_place_t, T&&) = delete;

		~optional() = default;

		// 20.5.5.2, mutation
		optional& operator=(nullopt_t) noexcept {
			ref = nullptr;
			return *this;
		}

		// optional& operator=(const optional& rhs) noexcept {
		// ref = rhs.ref;
		// return *this;
		// }

		// optional& operator=(optional&& rhs) noexcept {
		// ref = rhs.ref;
		// return *this;
		// }

		template <typename U>
		auto operator=(U&& rhs) noexcept
			-> typename ::std::enable_if<
				::std::is_same<typename ::std::decay<U>::type, optional<T&>>::value,
				optional&>::type {
			ref = rhs.ref;
			return *this;
		}

		template <typename U>
		auto operator=(U&& rhs) noexcept
			-> typename ::std::enable_if<
				!::std::is_same<typename ::std::decay<U>::type, optional<T&>>::value,
				optional&>::type = delete;

		void emplace(T& v) noexcept {
			ref = detail_::static_addressof(v);
		}

		void emplace(T&&) = delete;

		void swap(optional<T&>& rhs) noexcept {
			::std::swap(ref, rhs.ref);
		}

		// 20.5.5.3, observers
		constexpr T* operator->() const {
			return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, ref);
		}

		constexpr T& operator*() const {
			return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, *ref);
		}

		constexpr T& value() const {
#ifdef SOL_NO_EXCEPTIONS
			return *ref;
#else
			return ref ? *ref
					 : (throw bad_optional_access("bad optional access"), *ref);
#endif // Exceptions
		}

		explicit constexpr operator bool() const noexcept {
			return ref != nullptr;
		}

		template <typename V>
		constexpr T& value_or(V&& v) const {
			return *this ? **this : detail_::convert<T&>(constexpr_forward<V>(v));
		}
	};

	template <class T>
	class optional<T&&> {
		static_assert(sizeof(T) == 0, "optional rvalue references disallowed");
	};

	// 20.5.8, Relational operators
	template <class T>
	constexpr bool operator==(const optional<T>& x, const optional<T>& y) {
		return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
	}

	template <class T>
	constexpr bool operator!=(const optional<T>& x, const optional<T>& y) {
		return !(x == y);
	}

	template <class T>
	constexpr bool operator<(const optional<T>& x, const optional<T>& y) {
		return (!y) ? false : (!x) ? true : *x < *y;
	}

	template <class T>
	constexpr bool operator>(const optional<T>& x, const optional<T>& y) {
		return (y < x);
	}

	template <class T>
	constexpr bool operator<=(const optional<T>& x, const optional<T>& y) {
		return !(y < x);
	}

	template <class T>
	constexpr bool operator>=(const optional<T>& x, const optional<T>& y) {
		return !(x < y);
	}

	// 20.5.9, Comparison with nullopt
	template <class T>
	constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept {
		return (!x);
	}

	template <class T>
	constexpr bool operator==(nullopt_t, const optional<T>& x) noexcept {
		return (!x);
	}

	template <class T>
	constexpr bool operator!=(const optional<T>& x, nullopt_t) noexcept {
		return bool(x);
	}

	template <class T>
	constexpr bool operator!=(nullopt_t, const optional<T>& x) noexcept {
		return bool(x);
	}

	template <class T>
	constexpr bool operator<(const optional<T>&, nullopt_t) noexcept {
		return false;
	}

	template <class T>
	constexpr bool operator<(nullopt_t, const optional<T>& x) noexcept {
		return bool(x);
	}

	template <class T>
	constexpr bool operator<=(const optional<T>& x, nullopt_t) noexcept {
		return (!x);
	}

	template <class T>
	constexpr bool operator<=(nullopt_t, const optional<T>&) noexcept {
		return true;
	}

	template <class T>
	constexpr bool operator>(const optional<T>& x, nullopt_t) noexcept {
		return bool(x);
	}

	template <class T>
	constexpr bool operator>(nullopt_t, const optional<T>&) noexcept {
		return false;
	}

	template <class T>
	constexpr bool operator>=(const optional<T>&, nullopt_t) noexcept {
		return true;
	}

	template <class T>
	constexpr bool operator>=(nullopt_t, const optional<T>& x) noexcept {
		return (!x);
	}

	// 20.5.10, Comparison with T
	template <class T>
	constexpr bool operator==(const optional<T>& x, const T& v) {
		return bool(x) ? *x == v : false;
	}

	template <class T>
	constexpr bool operator==(const T& v, const optional<T>& x) {
		return bool(x) ? v == *x : false;
	}

	template <class T>
	constexpr bool operator!=(const optional<T>& x, const T& v) {
		return bool(x) ? *x != v : true;
	}

	template <class T>
	constexpr bool operator!=(const T& v, const optional<T>& x) {
		return bool(x) ? v != *x : true;
	}

	template <class T>
	constexpr bool operator<(const optional<T>& x, const T& v) {
		return bool(x) ? *x < v : true;
	}

	template <class T>
	constexpr bool operator>(const T& v, const optional<T>& x) {
		return bool(x) ? v > *x : true;
	}

	template <class T>
	constexpr bool operator>(const optional<T>& x, const T& v) {
		return bool(x) ? *x > v : false;
	}

	template <class T>
	constexpr bool operator<(const T& v, const optional<T>& x) {
		return bool(x) ? v < *x : false;
	}

	template <class T>
	constexpr bool operator>=(const optional<T>& x, const T& v) {
		return bool(x) ? *x >= v : false;
	}

	template <class T>
	constexpr bool operator<=(const T& v, const optional<T>& x) {
		return bool(x) ? v <= *x : false;
	}

	template <class T>
	constexpr bool operator<=(const optional<T>& x, const T& v) {
		return bool(x) ? *x <= v : true;
	}

	template <class T>
	constexpr bool operator>=(const T& v, const optional<T>& x) {
		return bool(x) ? v >= *x : true;
	}

	// Comparison of optional<T&> with T
	template <class T>
	constexpr bool operator==(const optional<T&>& x, const T& v) {
		return bool(x) ? *x == v : false;
	}

	template <class T>
	constexpr bool operator==(const T& v, const optional<T&>& x) {
		return bool(x) ? v == *x : false;
	}

	template <class T>
	constexpr bool operator!=(const optional<T&>& x, const T& v) {
		return bool(x) ? *x != v : true;
	}

	template <class T>
	constexpr bool operator!=(const T& v, const optional<T&>& x) {
		return bool(x) ? v != *x : true;
	}

	template <class T>
	constexpr bool operator<(const optional<T&>& x, const T& v) {
		return bool(x) ? *x < v : true;
	}

	template <class T>
	constexpr bool operator>(const T& v, const optional<T&>& x) {
		return bool(x) ? v > *x : true;
	}

	template <class T>
	constexpr bool operator>(const optional<T&>& x, const T& v) {
		return bool(x) ? *x > v : false;
	}

	template <class T>
	constexpr bool operator<(const T& v, const optional<T&>& x) {
		return bool(x) ? v < *x : false;
	}

	template <class T>
	constexpr bool operator>=(const optional<T&>& x, const T& v) {
		return bool(x) ? *x >= v : false;
	}

	template <class T>
	constexpr bool operator<=(const T& v, const optional<T&>& x) {
		return bool(x) ? v <= *x : false;
	}

	template <class T>
	constexpr bool operator<=(const optional<T&>& x, const T& v) {
		return bool(x) ? *x <= v : true;
	}

	template <class T>
	constexpr bool operator>=(const T& v, const optional<T&>& x) {
		return bool(x) ? v >= *x : true;
	}

	// Comparison of optional<T const&> with T
	template <class T>
	constexpr bool operator==(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x == v : false;
	}

	template <class T>
	constexpr bool operator==(const T& v, const optional<const T&>& x) {
		return bool(x) ? v == *x : false;
	}

	template <class T>
	constexpr bool operator!=(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x != v : true;
	}

	template <class T>
	constexpr bool operator!=(const T& v, const optional<const T&>& x) {
		return bool(x) ? v != *x : true;
	}

	template <class T>
	constexpr bool operator<(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x < v : true;
	}

	template <class T>
	constexpr bool operator>(const T& v, const optional<const T&>& x) {
		return bool(x) ? v > *x : true;
	}

	template <class T>
	constexpr bool operator>(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x > v : false;
	}

	template <class T>
	constexpr bool operator<(const T& v, const optional<const T&>& x) {
		return bool(x) ? v < *x : false;
	}

	template <class T>
	constexpr bool operator>=(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x >= v : false;
	}

	template <class T>
	constexpr bool operator<=(const T& v, const optional<const T&>& x) {
		return bool(x) ? v <= *x : false;
	}

	template <class T>
	constexpr bool operator<=(const optional<const T&>& x, const T& v) {
		return bool(x) ? *x <= v : true;
	}

	template <class T>
	constexpr bool operator>=(const T& v, const optional<const T&>& x) {
		return bool(x) ? v >= *x : true;
	}

	// 20.5.12, Specialized algorithms
	template <class T>
	void swap(optional<T>& x, optional<T>& y) noexcept(noexcept(x.swap(y))) {
		x.swap(y);
	}

	template <class T>
	constexpr optional<typename ::std::decay<T>::type> make_optional(T&& v) {
		return optional<typename ::std::decay<T>::type>(constexpr_forward<T>(v));
	}

	template <class X>
	constexpr optional<X&> make_optional(::std::reference_wrapper<X> v) {
		return optional<X&>(v.get());
	}

} // namespace sol

namespace std {
	template <typename T>
	struct hash<sol::optional<T>> {
		typedef typename hash<T>::result_type result_type;
		typedef sol::optional<T> argument_type;

		constexpr result_type operator()(argument_type const& arg) const {
			return arg ? ::std::hash<T>{}(*arg) : result_type{};
		}
	};

	template <typename T>
	struct hash<sol::optional<T&>> {
		typedef typename hash<T>::result_type result_type;
		typedef sol::optional<T&> argument_type;

		constexpr result_type operator()(argument_type const& arg) const {
			return arg ? ::std::hash<T>{}(*arg) : result_type{};
		}
	};
} // namespace std

#if defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#pragma warning(pop)
#endif

#undef TR2_OPTIONAL_REQUIRES
#undef TR2_OPTIONAL_ASSERTED_EXPRESSION

// end of sol/optional_implementation.hpp

#endif // Boost vs. Better optional

namespace sol {

#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
	template <typename T>
	using optional = boost::optional<T>;
	using nullopt_t = boost::none_t;
	const nullopt_t nullopt = boost::none;
#endif // Boost vs. Better optional

	namespace meta {
		template <typename T>
		struct is_optional : std::false_type {};
		template <typename T>
		struct is_optional<optional<T>> : std::true_type {};
	} // namespace meta
} // namespace sol

// end of sol/optional.hpp

// beginning of sol/forward_detail.hpp

namespace sol {
	namespace detail {
		const bool default_safe_function_calls =
#if defined(SOL_SAFE_FUNCTION_CALLS) && SOL_SAFE_FUNCTION_CALLS
			true;
#else
			false;
#endif
	} // namespace detail

	namespace meta {
	namespace meta_detail {
	}
	} // namespace meta::meta_detail

	namespace stack {
	namespace stack_detail {
		template <typename T>
		struct undefined_metatable;
	}
	} // namespace stack::stack_detail

	namespace usertype_detail {
		template <typename T, typename Regs, typename Fx>
		void insert_default_registrations(Regs& l, int& index, Fx&& fx);

		template <typename T, typename Regs, meta::enable<meta::neg<std::is_pointer<T>>, std::is_destructible<T>> = meta::enabler>
		void make_destructor(Regs& l, int& index);
		template <typename T, typename Regs, meta::disable<meta::neg<std::is_pointer<T>>, std::is_destructible<T>> = meta::enabler>
		void make_destructor(Regs& l, int& index);
	} // namespace usertype_detail
} // namespace sol

// end of sol/forward_detail.hpp

// beginning of sol/raii.hpp

namespace sol {
	namespace detail {
		struct default_construct {
			template <typename T, typename... Args>
			static void construct(T&& obj, Args&&... args) {
				typedef meta::unqualified_t<T> Tu;
				std::allocator<Tu> alloc{};
				std::allocator_traits<std::allocator<Tu>>::construct(alloc, std::forward<T>(obj), std::forward<Args>(args)...);
			}

			template <typename T, typename... Args>
			void operator()(T&& obj, Args&&... args) const {
				construct(std::forward<T>(obj), std::forward<Args>(args)...);
			}
		};

		struct default_destruct {
			template <typename T>
			static void destroy(T&& obj) {
				std::allocator<meta::unqualified_t<T>> alloc{};
				alloc.destroy(obj);
			}

			template <typename T>
			void operator()(T&& obj) const {
				destroy(std::forward<T>(obj));
			}
		};

		struct deleter {
			template <typename T>
			void operator()(T* p) const {
				delete p;
			}
		};

		struct state_deleter {
			void operator()(lua_State* L) const {
				lua_close(L);
			}
		};

		template <typename T, typename Dx, typename... Args>
		inline std::unique_ptr<T, Dx> make_unique_deleter(Args&&... args) {
			return std::unique_ptr<T, Dx>(new T(std::forward<Args>(args)...));
		}

		template <typename Tag, typename T>
		struct tagged {
			T value;
			template <typename Arg, typename... Args, meta::disable<std::is_same<meta::unqualified_t<Arg>, tagged>> = meta::enabler>
			tagged(Arg&& arg, Args&&... args)
			: value(std::forward<Arg>(arg), std::forward<Args>(args)...) {
			}
		};
	} // namespace detail

	template <typename... Args>
	struct constructor_list {};

	template <typename... Args>
	using constructors = constructor_list<Args...>;

	const auto default_constructor = constructors<types<>>{};

	struct no_construction {};
	const auto no_constructor = no_construction{};

	struct call_construction {};
	const auto call_constructor = call_construction{};

	template <typename... Functions>
	struct constructor_wrapper {
		std::tuple<Functions...> functions;
		template <typename Arg, typename... Args, meta::disable<std::is_same<meta::unqualified_t<Arg>, constructor_wrapper>> = meta::enabler>
		constructor_wrapper(Arg&& arg, Args&&... args)
		: functions(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}
	};

	template <typename... Functions>
	inline auto initializers(Functions&&... functions) {
		return constructor_wrapper<std::decay_t<Functions>...>(std::forward<Functions>(functions)...);
	}

	template <typename... Functions>
	struct factory_wrapper {
		std::tuple<Functions...> functions;
		template <typename Arg, typename... Args, meta::disable<std::is_same<meta::unqualified_t<Arg>, factory_wrapper>> = meta::enabler>
		factory_wrapper(Arg&& arg, Args&&... args)
		: functions(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}
	};

	template <typename... Functions>
	inline auto factories(Functions&&... functions) {
		return factory_wrapper<std::decay_t<Functions>...>(std::forward<Functions>(functions)...);
	}

	template <typename Function>
	struct destructor_wrapper {
		Function fx;
		destructor_wrapper(Function f)
		: fx(std::move(f)) {
		}
	};

	template <>
	struct destructor_wrapper<void> {};

	const destructor_wrapper<void> default_destructor{};

	template <typename Fx>
	inline auto destructor(Fx&& fx) {
		return destructor_wrapper<std::decay_t<Fx>>(std::forward<Fx>(fx));
	}

} // namespace sol

// end of sol/raii.hpp

// beginning of sol/filters.hpp

namespace sol {
	namespace detail {
		struct filter_base_tag {};
	} // namespace detail

	template <int Target, int... In>
	struct static_stack_dependencies : detail::filter_base_tag {};
	typedef static_stack_dependencies<-1, 1> self_dependency;
	template <int... In>
	struct returns_self_with : detail::filter_base_tag {};
	typedef returns_self_with<> returns_self;

	struct stack_dependencies : detail::filter_base_tag {
		int target;
		std::array<int, 64> stack_indices;
		std::size_t len;

		template <typename... Args>
		stack_dependencies(int stack_target, Args&&... args)
		: target(stack_target), stack_indices(), len(sizeof...(Args)) {
			std::size_t i = 0;
			(void)detail::swallow{int(), (stack_indices[i++] = static_cast<int>(std::forward<Args>(args)), int())...};
		}

		int& operator[](std::size_t i) {
			return stack_indices[i];
		}

		const int& operator[](std::size_t i) const {
			return stack_indices[i];
		}

		std::size_t size() const {
			return len;
		}
	};

	template <typename F, typename... Filters>
	struct filter_wrapper {
		typedef std::index_sequence_for<Filters...> indices;

		F value;
		std::tuple<Filters...> filters;

		template <typename Fx, typename... Args, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Fx>, filter_wrapper>>> = meta::enabler>
		filter_wrapper(Fx&& fx, Args&&... args)
		: value(std::forward<Fx>(fx)), filters(std::forward<Args>(args)...) {
		}

		filter_wrapper(const filter_wrapper&) = default;
		filter_wrapper& operator=(const filter_wrapper&) = default;
		filter_wrapper(filter_wrapper&&) = default;
		filter_wrapper& operator=(filter_wrapper&&) = default;
	};

	template <typename F, typename... Args>
	auto filters(F&& f, Args&&... args) {
		return filter_wrapper<std::decay_t<F>, std::decay_t<Args>...>(std::forward<F>(f), std::forward<Args>(args)...);
	}
} // namespace sol

// end of sol/filters.hpp

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <optional>
#ifdef SOL_STD_VARIANT
#include <variant>
#endif
#endif // C++17
#ifdef SOL_USE_BOOST
#include <boost/unordered_map.hpp>
#else
#include <unordered_map>
#endif // Using Boost

namespace sol {
	namespace usertype_detail {
#if defined(SOL_USE_BOOST)
#if defined(SOL_CXX17_FEATURES)
		template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<>>
		using map_t = boost::unordered_map<K, V, H, E>;
#else
		template <typename K, typename V, typename H = boost::hash<K>, typename E = std::equal_to<>>
		using map_t = boost::unordered_map<K, V, H, E>;
#endif // C++17 or not, WITH boost
#else
		template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<>>
		using map_t = std::unordered_map<K, V, H, E>;
#endif // Boost map target
	} // namespace usertype_detail

	namespace detail {
#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
		typedef int (*lua_CFunction_noexcept)(lua_State* L) noexcept;
#else
		typedef int (*lua_CFunction_noexcept)(lua_State* L);
#endif // noexcept function type for lua_CFunction

		template <typename T>
		struct unique_usertype {};

		template <typename T>
		struct implicit_wrapper {
			T& item;
			implicit_wrapper(T* item)
			: item(*item) {
			}
			implicit_wrapper(T& item)
			: item(item) {
			}
			operator T&() {
				return item;
			}
			operator T*() {
				return std::addressof(item);
			}
		};

		struct unchecked_t {};
		const unchecked_t unchecked = unchecked_t{};

		struct yield_tag_t {};
		const yield_tag_t yield_tag = yield_tag_t{};
	} // namespace detail

	struct lua_nil_t {};
	const lua_nil_t lua_nil{};
	inline bool operator==(lua_nil_t, lua_nil_t) {
		return true;
	}
	inline bool operator!=(lua_nil_t, lua_nil_t) {
		return false;
	}
	typedef lua_nil_t nil_t;
#if !defined(SOL_NO_NIL) || (SOL_NO_NIL == 0)
	const nil_t nil{};
#endif

	namespace detail {
		struct non_lua_nil_t {};
	} // namespace detail

	struct metatable_t {};
	const metatable_t metatable_key = {};

	struct env_t {};
	const env_t env_key = {};

	struct no_metatable_t {};
	const no_metatable_t no_metatable = {};

	template <typename T>
	struct yielding_t {
		T func;

		yielding_t() = default;
		yielding_t(const yielding_t&) = default;
		yielding_t(yielding_t&&) = default;
		yielding_t& operator=(const yielding_t&) = default;
		yielding_t& operator=(yielding_t&&) = default;
		template <typename Arg, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Arg>, yielding_t>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<Arg>>>> = meta::enabler>
		yielding_t(Arg&& arg)
		: func(std::forward<Arg>(arg)) {
		}
		template <typename Arg0, typename Arg1, typename... Args>
		yielding_t(Arg0&& arg0, Arg1&& arg1, Args&&... args)
		: func(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...) {
		}
	};

	template <typename F>
	inline yielding_t<std::decay_t<F>> yielding(F&& f) {
		return yielding_t<std::decay_t<F>>(std::forward<F>(f));
	}

	typedef std::remove_pointer_t<lua_CFunction> lua_CFunction_ref;

	template <typename T>
	struct unique_usertype_traits {
		typedef T type;
		typedef T actual_type;
		template <typename X>
		using rebind_base = void;

		static const bool value = false;

		template <typename U>
		static bool is_null(U&&) {
			return false;
		}

		template <typename U>
		static auto get(U&& value) {
			return std::addressof(detail::deref(value));
		}
	};

	template <typename T>
	struct unique_usertype_traits<std::shared_ptr<T>> {
		typedef T type;
		typedef std::shared_ptr<T> actual_type;
		// rebind is non-void
		// if and only if unique usertype
		// is cast-capable
		template <typename X>
		using rebind_base = std::shared_ptr<X>;

		static const bool value = true;

		static bool is_null(const actual_type& p) {
			return p == nullptr;
		}

		static type* get(const actual_type& p) {
			return p.get();
		}
	};

	template <typename T, typename D>
	struct unique_usertype_traits<std::unique_ptr<T, D>> {
		typedef T type;
		typedef std::unique_ptr<T, D> actual_type;
		template <typename X>
		using rebind_base = void;

		static const bool value = true;

		static bool is_null(const actual_type& p) {
			return p == nullptr;
		}

		static type* get(const actual_type& p) {
			return p.get();
		}
	};

	template <typename T>
	struct non_null {};

	template <typename... Args>
	struct function_sig {};

	struct upvalue_index {
		int index;
		upvalue_index(int idx)
		: index(lua_upvalueindex(idx)) {
		}

		operator int() const {
			return index;
		}
	};

	struct raw_index {
		int index;
		raw_index(int i)
		: index(i) {
		}

		operator int() const {
			return index;
		}
	};

	struct absolute_index {
		int index;
		absolute_index(lua_State* L, int idx)
		: index(lua_absindex(L, idx)) {
		}

		operator int() const {
			return index;
		}
	};

	struct ref_index {
		int index;
		ref_index(int idx)
		: index(idx) {
		}

		operator int() const {
			return index;
		}
	};

	struct stack_count {
		int count;

		stack_count(int cnt)
		: count(cnt) {
		}
	};

	struct lightuserdata_value {
		void* value;
		lightuserdata_value(void* data)
		: value(data) {
		}
		operator void*() const {
			return value;
		}
	};

	struct userdata_value {
		void* value;
		userdata_value(void* data)
		: value(data) {
		}
		operator void*() const {
			return value;
		}
	};

	template <typename L>
	struct light {
		L* value;

		light(L& x)
		: value(std::addressof(x)) {
		}
		light(L* x)
		: value(x) {
		}
		light(void* x)
		: value(static_cast<L*>(x)) {
		}
		operator L*() const {
			return value;
		}
		operator L&() const {
			return *value;
		}
	};

	template <typename T>
	auto make_light(T& l) {
		typedef meta::unwrapped_t<std::remove_pointer_t<std::remove_pointer_t<T>>> L;
		return light<L>(l);
	}

	template <typename U>
	struct user {
		U value;

		user(U&& x)
		: value(std::forward<U>(x)) {
		}
		operator std::add_pointer_t<std::remove_reference_t<U>>() {
			return std::addressof(value);
		}
		operator std::add_lvalue_reference_t<U>() {
			return value;
		}
		operator std::add_const_t<std::add_lvalue_reference_t<U>>&() const {
			return value;
		}
	};

	template <typename T>
	auto make_user(T&& u) {
		typedef meta::unwrapped_t<meta::unqualified_t<T>> U;
		return user<U>(std::forward<T>(u));
	}

	template <typename T>
	struct metatable_registry_key {
		T key;

		metatable_registry_key(T key)
		: key(std::forward<T>(key)) {
		}
	};

	template <typename T>
	auto meta_registry_key(T&& key) {
		typedef meta::unqualified_t<T> K;
		return metatable_registry_key<K>(std::forward<T>(key));
	}

	template <typename... Upvalues>
	struct closure {
		lua_CFunction c_function;
		std::tuple<Upvalues...> upvalues;
		closure(lua_CFunction f, Upvalues... targetupvalues)
		: c_function(f), upvalues(std::forward<Upvalues>(targetupvalues)...) {
		}
	};

	template <>
	struct closure<> {
		lua_CFunction c_function;
		int upvalues;
		closure(lua_CFunction f, int upvalue_count = 0)
		: c_function(f), upvalues(upvalue_count) {
		}
	};

	typedef closure<> c_closure;

	template <typename... Args>
	closure<Args...> make_closure(lua_CFunction f, Args&&... args) {
		return closure<Args...>(f, std::forward<Args>(args)...);
	}

	template <typename Sig, typename... Ps>
	struct function_arguments {
		std::tuple<Ps...> arguments;
		template <typename Arg, typename... Args, meta::disable<std::is_same<meta::unqualified_t<Arg>, function_arguments>> = meta::enabler>
		function_arguments(Arg&& arg, Args&&... args)
		: arguments(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}
	};

	template <typename Sig = function_sig<>, typename... Args>
	auto as_function(Args&&... args) {
		return function_arguments<Sig, std::decay_t<Args>...>(std::forward<Args>(args)...);
	}

	template <typename Sig = function_sig<>, typename... Args>
	auto as_function_reference(Args&&... args) {
		return function_arguments<Sig, Args...>(std::forward<Args>(args)...);
	}

	template <typename T>
	struct as_table_t {
		T source;

		as_table_t() = default;
		as_table_t(const as_table_t&) = default;
		as_table_t(as_table_t&&) = default;
		as_table_t& operator=(const as_table_t&) = default;
		as_table_t& operator=(as_table_t&&) = default;
		template <typename Arg, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Arg>, as_table_t>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<Arg>>>> = meta::enabler>
		as_table_t(Arg&& arg)
		: source(std::forward<Arg>(arg)) {
		}
		template <typename Arg0, typename Arg1, typename... Args>
		as_table_t(Arg0&& arg0, Arg1&& arg1, Args&&... args)
		: source(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...) {
		}

		operator std::add_lvalue_reference_t<T>() {
			return source;
		}
	};

	template <typename T>
	struct nested {
		T source;

		nested() = default;
		nested(const nested&) = default;
		nested(nested&&) = default;
		nested& operator=(const nested&) = default;
		nested& operator=(nested&&) = default;
		template <typename Arg, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Arg>, nested>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<Arg>>>> = meta::enabler>
		nested(Arg&& arg)
		: source(std::forward<Arg>(arg)) {
		}
		template <typename Arg0, typename Arg1, typename... Args>
		nested(Arg0&& arg0, Arg1&& arg1, Args&&... args)
		: source(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...) {
		}

		operator std::add_lvalue_reference_t<T>() {
			return source;
		}
	};

	template <typename T>
	as_table_t<T> as_table_ref(T&& container) {
		return as_table_t<T>(std::forward<T>(container));
	}

	template <typename T>
	as_table_t<meta::unqualified_t<T>> as_table(T&& container) {
		return as_table_t<meta::unqualified_t<T>>(std::forward<T>(container));
	}

	template <typename T>
	nested<T> as_nested_ref(T&& container) {
		return nested<T>(std::forward<T>(container));
	}

	template <typename T>
	nested<meta::unqualified_t<T>> as_nested(T&& container) {
		return nested<meta::unqualified_t<T>>(std::forward<T>(container));
	}

	struct this_state {
		lua_State* L;

		this_state(lua_State* Ls)
		: L(Ls) {
		}

		operator lua_State*() const noexcept {
			return lua_state();
		}

		lua_State* operator->() const noexcept {
			return lua_state();
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
	};

	struct this_main_state {
		lua_State* L;

		this_main_state(lua_State* Ls)
		: L(Ls) {
		}

		operator lua_State*() const noexcept {
			return lua_state();
		}

		lua_State* operator->() const noexcept {
			return lua_state();
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
	};

	struct new_table {
		int sequence_hint = 0;
		int map_hint = 0;

		new_table() = default;
		new_table(const new_table&) = default;
		new_table(new_table&&) = default;
		new_table& operator=(const new_table&) = default;
		new_table& operator=(new_table&&) = default;

		new_table(int sequence_hint, int map_hint = 0)
		: sequence_hint(sequence_hint), map_hint(map_hint) {
		}
	};

	enum class lib : char {
		// print, assert, and other base functions
		base,
		// require and other package functions
		package,
		// coroutine functions and utilities
		coroutine,
		// string library
		string,
		// functionality from the OS
		os,
		// all things math
		math,
		// the table manipulator and observer functions
		table,
		// the debug library
		debug,
		// the bit library: different based on which you're using
		bit32,
		// input/output library
		io,
		// LuaJIT only
		ffi,
		// LuaJIT only
		jit,
		// library for handling utf8: new to Lua
		utf8,
		// do not use
		count
	};

	enum class call_syntax {
		dot = 0,
		colon = 1
	};

	enum class load_mode {
		any = 0,
		text = 1,
		binary = 2,
	};

	enum class call_status : int {
		ok = LUA_OK,
		yielded = LUA_YIELD,
		runtime = LUA_ERRRUN,
		memory = LUA_ERRMEM,
		handler = LUA_ERRERR,
		gc = LUA_ERRGCMM,
		syntax = LUA_ERRSYNTAX,
		file = LUA_ERRFILE,
	};

	enum class thread_status : int {
		ok = LUA_OK,
		yielded = LUA_YIELD,
		runtime = LUA_ERRRUN,
		memory = LUA_ERRMEM,
		gc = LUA_ERRGCMM,
		handler = LUA_ERRERR,
		dead = -1,
	};

	enum class load_status : int {
		ok = LUA_OK,
		syntax = LUA_ERRSYNTAX,
		memory = LUA_ERRMEM,
		gc = LUA_ERRGCMM,
		file = LUA_ERRFILE,
	};

	enum class type : int {
		none = LUA_TNONE,
		lua_nil = LUA_TNIL,
#if !defined(SOL_NO_NIL)
		nil = lua_nil,
#endif // Objective C/C++ Keyword that's found in OSX SDK and OBJC -- check for all forms to protect
		string = LUA_TSTRING,
		number = LUA_TNUMBER,
		thread = LUA_TTHREAD,
		boolean = LUA_TBOOLEAN,
		function = LUA_TFUNCTION,
		userdata = LUA_TUSERDATA,
		lightuserdata = LUA_TLIGHTUSERDATA,
		table = LUA_TTABLE,
		poly = -0xFFFF
	};

	inline const std::string& to_string(call_status c) {
		static const std::array<std::string, 10> names{ { "ok",
			"yielded",
			"runtime",
			"memory",
			"handler",
			"gc",
			"syntax",
			"file",
			"CRITICAL_EXCEPTION_FAILURE",
			"CRITICAL_INDETERMINATE_STATE_FAILURE" } };
		switch (c) {
		case call_status::ok:
			return names[0];
		case call_status::yielded:
			return names[1];
		case call_status::runtime:
			return names[2];
		case call_status::memory:
			return names[3];
		case call_status::handler:
			return names[4];
		case call_status::gc:
			return names[5];
		case call_status::syntax:
			return names[6];
		case call_status::file:
			return names[7];
		}
		if (static_cast<std::ptrdiff_t>(c) == -1) {
			// One of the many cases where a critical exception error has occurred
			return names[8];
		}
		return names[9];
	}

	inline bool is_indeterminate_call_failure(call_status c) {
		switch (c) {
		case call_status::ok:
		case call_status::yielded:
		case call_status::runtime:
		case call_status::memory:
		case call_status::handler:
		case call_status::gc:
		case call_status::syntax:
		case call_status::file:
			return false;
		}
		return true;
	}

	inline const std::string& to_string(load_status c) {
		static const std::array<std::string, 7> names{ { "ok",
			"memory",
			"gc",
			"syntax",
			"file",
			"CRITICAL_EXCEPTION_FAILURE",
			"CRITICAL_INDETERMINATE_STATE_FAILURE" } };
		switch (c) {
		case load_status::ok:
			return names[0];
		case load_status::memory:
			return names[1];
		case load_status::gc:
			return names[2];
		case load_status::syntax:
			return names[3];
		case load_status::file:
			return names[4];
		}
		if (static_cast<int>(c) == -1) {
			// One of the many cases where a critical exception error has occurred
			return names[5];
		}
		return names[6];
	}

	inline const std::string& to_string(load_mode c) {
		static const std::array<std::string, 3> names{ {
			"bt",
			"t",
			"b",
		} };
		return names[static_cast<std::size_t>(c)];
	}

	enum class meta_function {
		construct,
		index,
		new_index,
		mode,
		call,
		call_function = call,
		metatable,
		to_string,
		length,
		unary_minus,
		addition,
		subtraction,
		multiplication,
		division,
		modulus,
		power_of,
		involution = power_of,
		concatenation,
		equal_to,
		less_than,
		less_than_or_equal_to,
		garbage_collect,
		floor_division,
		bitwise_left_shift,
		bitwise_right_shift,
		bitwise_not,
		bitwise_and,
		bitwise_or,
		bitwise_xor,
		pairs,
		ipairs,
		next,
		type,
		type_info,
	};

	typedef meta_function meta_method;

	inline const std::array<std::string, 32>& meta_function_names() {
		static const std::array<std::string, 32> names = { { "new",
			"__index",
			"__newindex",
			"__mode",
			"__call",
			"__mt",
			"__tostring",
			"__len",
			"__unm",
			"__add",
			"__sub",
			"__mul",
			"__div",
			"__mod",
			"__pow",
			"__concat",
			"__eq",
			"__lt",
			"__le",
			"__gc",

			"__idiv",
			"__shl",
			"__shr",
			"__bnot",
			"__band",
			"__bor",
			"__bxor",

			"__pairs",
			"__ipairs",
			"next",
			"__type",
			"__typeinfo" } };
		return names;
	}

	inline const std::string& to_string(meta_function mf) {
		return meta_function_names()[static_cast<int>(mf)];
	}

	inline type type_of(lua_State* L, int index) {
		return static_cast<type>(lua_type(L, index));
	}

	inline std::string type_name(lua_State* L, type t) {
		return lua_typename(L, static_cast<int>(t));
	}

	template <typename T>
	struct is_lua_reference : std::integral_constant<bool,
							 std::is_base_of<reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<main_reference, meta::unqualified_t<T>>::value
								 || std::is_base_of<stack_reference, meta::unqualified_t<T>>::value> {};

	template <typename T>
	struct is_lua_reference_or_proxy : std::integral_constant<bool,
									is_lua_reference<meta::unqualified_t<T>>::value
										|| meta::is_specialization_of<meta::unqualified_t<T>, proxy>::value> {};

	template <typename T>
	struct is_transparent_argument : std::false_type {};
	template <>
	struct is_transparent_argument<this_state> : std::true_type {};
	template <>
	struct is_transparent_argument<this_main_state> : std::true_type {};
	template <>
	struct is_transparent_argument<this_environment> : std::true_type {};
	template <>
	struct is_transparent_argument<variadic_args> : std::true_type {};
	template <typename T>
	struct is_variadic_arguments : std::is_same<meta::unqualified_t<T>, variadic_args> {};

	namespace detail {
		template <typename T>
		struct is_initializer_list : std::false_type {};

		template <typename T>
		struct is_initializer_list<std::initializer_list<T>> : std::true_type {};

		template <typename T, typename C = void>
		struct is_container : std::false_type {};

		template <typename T>
		struct is_container<std::initializer_list<T>> : std::false_type {};

		template <typename T>
		struct is_container<T, std::enable_if_t<meta::is_string_like<meta::unqualified_t<T>>::value>> : std::false_type {};

		template <typename T>
		struct is_container<T, std::enable_if_t<meta::all<std::is_array<meta::unqualified_t<T>>, meta::neg<meta::any_same<std::remove_all_extents_t<meta::unqualified_t<T>>, char, wchar_t, char16_t, char32_t>>>::value>> : std::true_type {};

		template <typename T>
		struct is_container<T, std::enable_if_t<meta::all<meta::has_begin_end<meta::unqualified_t<T>>, meta::neg<is_initializer_list<meta::unqualified_t<T>>>, meta::neg<meta::is_string_like<meta::unqualified_t<T>>>>::value>> : std::true_type {};
	} // namespace detail

	template <typename T>
	struct is_container : detail::is_container<T> {};

	template <typename T>
	struct is_to_stringable : meta::any<meta::supports_to_string_member<meta::unqualified_t<T>>, meta::supports_adl_to_string<meta::unqualified_t<T>>, meta::supports_ostream_op<meta::unqualified_t<T>>> {};

	namespace detail {
		template <typename T, typename = void>
		struct lua_type_of : std::integral_constant<type, type::userdata> {};

		template <typename C, typename T, typename A>
		struct lua_type_of<std::basic_string<C, T, A>> : std::integral_constant<type, type::string> {};

		template <typename C, typename T>
		struct lua_type_of<basic_string_view<C, T>> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<wchar_t[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char16_t[N]> : std::integral_constant<type, type::string> {};

		template <std::size_t N>
		struct lua_type_of<char32_t[N]> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<wchar_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char16_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<char32_t> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char16_t*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<const char32_t*> : std::integral_constant<type, type::string> {};

		template <>
		struct lua_type_of<bool> : std::integral_constant<type, type::boolean> {};

		template <>
		struct lua_type_of<lua_nil_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<nullopt_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<detail::non_lua_nil_t> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<std::nullptr_t> : std::integral_constant<type, type::lua_nil> {};

		template <>
		struct lua_type_of<error> : std::integral_constant<type, type::string> {};

		template <bool b, typename Base>
		struct lua_type_of<basic_table_core<b, Base>> : std::integral_constant<type, type::table> {};

		template <>
		struct lua_type_of<metatable_t> : std::integral_constant<type, type::table> {};

		template <typename B>
		struct lua_type_of<basic_environment<B>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<env_t> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<new_table> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<as_table_t<T>> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<std::initializer_list<T>> : std::integral_constant<type, type::table> {};

		template <bool b>
		struct lua_type_of<basic_reference<b>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<stack_reference> : std::integral_constant<type, type::poly> {};

		template <typename Base>
		struct lua_type_of<basic_object<Base>> : std::integral_constant<type, type::poly> {};

		template <typename... Args>
		struct lua_type_of<std::tuple<Args...>> : std::integral_constant<type, type::poly> {};

		template <typename A, typename B>
		struct lua_type_of<std::pair<A, B>> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<void*> : std::integral_constant<type, type::lightuserdata> {};

		template <>
		struct lua_type_of<const void*> : std::integral_constant<type, type::lightuserdata> {};

		template <>
		struct lua_type_of<lightuserdata_value> : std::integral_constant<type, type::lightuserdata> {};

		template <>
		struct lua_type_of<userdata_value> : std::integral_constant<type, type::userdata> {};

		template <typename T>
		struct lua_type_of<light<T>> : std::integral_constant<type, type::lightuserdata> {};

		template <typename T>
		struct lua_type_of<user<T>> : std::integral_constant<type, type::userdata> {};

		template <typename Base>
		struct lua_type_of<basic_lightuserdata<Base>> : std::integral_constant<type, type::lightuserdata> {};

		template <typename Base>
		struct lua_type_of<basic_userdata<Base>> : std::integral_constant<type, type::userdata> {};

		template <>
		struct lua_type_of<lua_CFunction> : std::integral_constant<type, type::function> {};

		template <>
		struct lua_type_of<std::remove_pointer_t<lua_CFunction>> : std::integral_constant<type, type::function> {};

		template <typename Base, bool aligned>
		struct lua_type_of<basic_function<Base, aligned>> : std::integral_constant<type, type::function> {};

		template <typename Base, bool aligned, typename Handler>
		struct lua_type_of<basic_protected_function<Base, aligned, Handler>> : std::integral_constant<type, type::function> {};

		template <typename Base>
		struct lua_type_of<basic_coroutine<Base>> : std::integral_constant<type, type::function> {};

		template <typename Base>
		struct lua_type_of<basic_thread<Base>> : std::integral_constant<type, type::thread> {};

		template <typename Signature>
		struct lua_type_of<std::function<Signature>> : std::integral_constant<type, type::function> {};

		template <typename T>
		struct lua_type_of<optional<T>> : std::integral_constant<type, type::poly> {};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
		template <typename T>
		struct lua_type_of<std::optional<T>> : std::integral_constant<type, type::poly> {};
#endif // std::optional

		template <>
		struct lua_type_of<variadic_args> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<variadic_results> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<stack_count> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_state> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_main_state> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<this_environment> : std::integral_constant<type, type::poly> {};

		template <>
		struct lua_type_of<type> : std::integral_constant<type, type::poly> {};

		template <typename T>
		struct lua_type_of<T*> : std::integral_constant<type, type::userdata> {};

		template <typename T>
		struct lua_type_of<T, std::enable_if_t<std::is_arithmetic<T>::value>> : std::integral_constant<type, type::number> {};

		template <typename T>
		struct lua_type_of<T, std::enable_if_t<std::is_enum<T>::value>> : std::integral_constant<type, type::number> {};

		template <>
		struct lua_type_of<meta_function> : std::integral_constant<type, type::string> {};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#ifdef SOL_STD_VARIANT
		template <typename... Tn>
		struct lua_type_of<std::variant<Tn...>> : std::integral_constant<type, type::poly> {};
#endif // SOL_STD_VARIANT
#endif // SOL_CXX17_FEATURES

		template <typename T>
		struct lua_type_of<nested<T>, std::enable_if_t<::sol::is_container<T>::value>> : std::integral_constant<type, type::table> {};

		template <typename T>
		struct lua_type_of<nested<T>, std::enable_if_t<!::sol::is_container<T>::value>> : lua_type_of<T> {};

		template <typename C, C v, template <typename...> class V, typename... Args>
		struct accumulate : std::integral_constant<C, v> {};

		template <typename C, C v, template <typename...> class V, typename T, typename... Args>
		struct accumulate<C, v, V, T, Args...> : accumulate<C, v + V<T>::value, V, Args...> {};

		template <typename C, C v, template <typename...> class V, typename List>
		struct accumulate_list;

		template <typename C, C v, template <typename...> class V, typename... Args>
		struct accumulate_list<C, v, V, types<Args...>> : accumulate<C, v, V, Args...> {};
	} // namespace detail

	template <typename T>
	struct is_unique_usertype : std::integral_constant<bool, unique_usertype_traits<T>::value> {};

	template <typename T>
	struct lua_type_of : detail::lua_type_of<T> {
		typedef int SOL_INTERNAL_UNSPECIALIZED_MARKER_;
	};

	template <typename T>
	struct lua_size : std::integral_constant<int, 1> {
		typedef int SOL_INTERNAL_UNSPECIALIZED_MARKER_;
	};

	template <typename A, typename B>
	struct lua_size<std::pair<A, B>> : std::integral_constant<int, lua_size<A>::value + lua_size<B>::value> {};

	template <typename... Args>
	struct lua_size<std::tuple<Args...>> : std::integral_constant<int, detail::accumulate<int, 0, lua_size, Args...>::value> {};

	namespace detail {
		template <typename...>
		struct void_ { typedef void type; };
		template <typename T, typename = void>
		struct has_internal_marker_impl : std::false_type {};
		template <typename T>
		struct has_internal_marker_impl<T, typename void_<typename T::SOL_INTERNAL_UNSPECIALIZED_MARKER_>::type> : std::true_type {};

		template <typename T>
		struct has_internal_marker : has_internal_marker_impl<T> {};
	} // namespace detail

	template <typename T>
	struct is_lua_primitive : std::integral_constant<bool,
							 type::userdata != lua_type_of<meta::unqualified_t<T>>::value
								 || ((type::userdata == lua_type_of<meta::unqualified_t<T>>::value)
									    && detail::has_internal_marker<lua_type_of<meta::unqualified_t<T>>>::value
									    && !detail::has_internal_marker<lua_size<meta::unqualified_t<T>>>::value)
								 || is_lua_reference<meta::unqualified_t<T>>::value
								 || meta::is_specialization_of<meta::unqualified_t<T>, std::tuple>::value
								 || meta::is_specialization_of<meta::unqualified_t<T>, std::pair>::value> {};

	template <typename T>
	struct is_main_threaded : std::is_base_of<main_reference, T> {};

	template <typename T>
	struct is_stack_based : std::is_base_of<stack_reference, T> {};
	template <>
	struct is_stack_based<variadic_args> : std::true_type {};
	template <>
	struct is_stack_based<unsafe_function_result> : std::true_type {};
	template <>
	struct is_stack_based<protected_function_result> : std::true_type {};
	template <>
	struct is_stack_based<stack_proxy> : std::true_type {};
	template <>
	struct is_stack_based<stack_proxy_base> : std::true_type {};

	template <typename T>
	struct is_lua_primitive<T*> : std::true_type {};
	template <>
	struct is_lua_primitive<unsafe_function_result> : std::true_type {};
	template <>
	struct is_lua_primitive<protected_function_result> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<std::reference_wrapper<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<user<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<light<T>> : is_lua_primitive<T*> {};
	template <typename T>
	struct is_lua_primitive<optional<T>> : std::true_type {};
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	template <typename T>
	struct is_lua_primitive<std::optional<T>> : std::true_type {};
#endif
	template <typename T>
	struct is_lua_primitive<as_table_t<T>> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<nested<T>> : std::true_type {};
	template <>
	struct is_lua_primitive<userdata_value> : std::true_type {};
	template <>
	struct is_lua_primitive<lightuserdata_value> : std::true_type {};
	template <typename T>
	struct is_lua_primitive<non_null<T>> : is_lua_primitive<T*> {};

	template <typename T>
	struct is_proxy_primitive : is_lua_primitive<T> {};

	template <typename T>
	struct is_lua_index : std::is_integral<T> {};
	template <>
	struct is_lua_index<raw_index> : std::true_type {};
	template <>
	struct is_lua_index<absolute_index> : std::true_type {};
	template <>
	struct is_lua_index<ref_index> : std::true_type {};
	template <>
	struct is_lua_index<upvalue_index> : std::true_type {};

	template <typename Signature>
	struct lua_bind_traits : meta::bind_traits<Signature> {
	private:
		typedef meta::bind_traits<Signature> base_t;

	public:
		typedef std::integral_constant<bool, meta::count_for<is_variadic_arguments, typename base_t::args_list>::value != 0> runtime_variadics_t;
		static const std::size_t true_arity = base_t::arity;
		static const std::size_t arity = detail::accumulate_list<std::size_t, 0, lua_size, typename base_t::args_list>::value - meta::count_for<is_transparent_argument, typename base_t::args_list>::value;
		static const std::size_t true_free_arity = base_t::free_arity;
		static const std::size_t free_arity = detail::accumulate_list<std::size_t, 0, lua_size, typename base_t::free_args_list>::value - meta::count_for<is_transparent_argument, typename base_t::args_list>::value;
	};

	template <typename T>
	struct is_table : std::false_type {};
	template <bool x, typename T>
	struct is_table<basic_table_core<x, T>> : std::true_type {};

	template <typename T>
	struct is_function : std::false_type {};
	template <typename T, bool aligned>
	struct is_function<basic_function<T, aligned>> : std::true_type {};
	template <typename T, bool aligned, typename Handler>
	struct is_function<basic_protected_function<T, aligned, Handler>> : std::true_type {};

	template <typename T>
	struct is_lightuserdata : std::false_type {};
	template <typename T>
	struct is_lightuserdata<basic_lightuserdata<T>> : std::true_type {};

	template <typename T>
	struct is_userdata : std::false_type {};
	template <typename T>
	struct is_userdata<basic_userdata<T>> : std::true_type {};

	template <typename T>
	struct is_environment : std::integral_constant<bool, is_userdata<T>::value || is_table<T>::value> {};

	template <typename T>
	struct is_automagical : meta::neg<std::is_array<meta::unqualified_t<T>>> {};

	template <typename T>
	inline type type_of() {
		return lua_type_of<meta::unqualified_t<T>>::value;
	}

	namespace detail {
		template <typename T>
		struct is_non_factory_constructor : std::false_type {};

		template <typename... Args>
		struct is_non_factory_constructor<constructors<Args...>> : std::true_type {};

		template <typename... Args>
		struct is_non_factory_constructor<constructor_wrapper<Args...>> : std::true_type {};

		template <>
		struct is_non_factory_constructor<no_construction> : std::true_type {};

		template <typename T>
		struct is_constructor : is_non_factory_constructor<T> {};

		template <typename... Args>
		struct is_constructor<factory_wrapper<Args...>> : std::true_type {};

		template <typename T>
		struct is_constructor<protect_t<T>> : is_constructor<meta::unqualified_t<T>> {};

		template <typename F, typename... Filters>
		struct is_constructor<filter_wrapper<F, Filters...>> : is_constructor<meta::unqualified_t<F>> {};

		template <typename... Args>
		using has_constructor = meta::any<is_constructor<meta::unqualified_t<Args>>...>;

		template <typename T>
		struct is_destructor : std::false_type {};

		template <typename Fx>
		struct is_destructor<destructor_wrapper<Fx>> : std::true_type {};

		template <typename... Args>
		using has_destructor = meta::any<is_destructor<meta::unqualified_t<Args>>...>;

		struct add_destructor_tag {};
		struct check_destructor_tag {};
		struct verified_tag {
		} const verified{};
	} // namespace detail
} // namespace sol

// end of sol/types.hpp

#include <exception>
#include <cstring>

#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
#include <iostream>
#endif

namespace sol {
	// must push a single object to be the error object
	// NOTE: the VAST MAJORITY of all Lua libraries -- C or otherwise -- expect a string for the type of error
	// break this convention at your own risk
	using exception_handler_function = int(*)(lua_State*, optional<const std::exception&>, string_view);

	namespace detail {
		inline const char(&default_exception_handler_name())[11]{
			static const char name[11] = "sol.\xE2\x98\xA2\xE2\x98\xA2";
			return name;
		}

		// must push at least 1 object on the stack
		inline int default_exception_handler(lua_State* L, optional<const std::exception&>, string_view what) {
#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
			std::cerr << "[sol2] An exception occurred: ";
			std::cerr.write(what.data(), what.size());
			std::cerr << std::endl;
#endif
			lua_pushlstring(L, what.data(), what.size());
			return 1;
		}

		inline int call_exception_handler(lua_State* L, optional<const std::exception&> maybe_ex, string_view what) {
			lua_getglobal(L, default_exception_handler_name());
			type t = static_cast<type>(lua_type(L, -1));
			if (t != type::lightuserdata) {
				lua_pop(L, 1);
				return default_exception_handler(L, std::move(maybe_ex), std::move(what));
			}
			void* vfunc = lua_touserdata(L, -1);
			lua_pop(L, 1);
			if (vfunc == nullptr) {
				return default_exception_handler(L, std::move(maybe_ex), std::move(what));
			}
			exception_handler_function exfunc = reinterpret_cast<exception_handler_function>(vfunc);
			return exfunc(L, std::move(maybe_ex), std::move(what));
		}

#ifdef SOL_NO_EXCEPTIONS
		template <lua_CFunction f>
		int static_trampoline(lua_State* L) noexcept {
			return f(L);
		}

#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
		template <lua_CFunction_noexcept f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#else
		template <lua_CFunction f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif

		template <typename Fx, typename... Args>
		int trampoline(lua_State* L, Fx&& f, Args&&... args) noexcept {
			return f(L, std::forward<Args>(args)...);
		}

		inline int c_trampoline(lua_State* L, lua_CFunction f) noexcept {
			return trampoline(L, f);
		}
#else
		template <lua_CFunction f>
		int static_trampoline(lua_State* L) {
#if defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) && !defined(SOL_LUAJIT)
			return f(L);

#else
			try {
				return f(L);
			}
			catch (const char* cs) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), string_view(cs));
			}
			catch (const std::string& s) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), string_view(s.c_str(), s.size()));
			}
			catch (const std::exception& e) {
				call_exception_handler(L, optional<const std::exception&>(e), e.what());
			}
#if !defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), "caught (...) exception");
			}
#endif // LuaJIT cannot have the catchall, but we must catch std::exceps for it
			return lua_error(L);
#endif // Safe exceptions
		}

#ifdef SOL_NOEXCEPT_FUNCTION_TYPE
#if 0 
		// impossible: g++/clang++ choke as they think this function is ambiguous:
		// to fix, wait for template <auto X> and then switch on no-exceptness of the function
		template <lua_CFunction_noexcept f>
		int static_trampoline(lua_State* L) noexcept {
			return f(L);
		}
#else
		template <lua_CFunction_noexcept f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif // impossible

#else
		template <lua_CFunction f>
		int static_trampoline_noexcept(lua_State* L) noexcept {
			return f(L);
		}
#endif // noexcept lua_CFunction type

		template <typename Fx, typename... Args>
		int trampoline(lua_State* L, Fx&& f, Args&&... args) {
			if (meta::bind_traits<meta::unqualified_t<Fx>>::is_noexcept) {
				return f(L, std::forward<Args>(args)...);
			}
#if defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) && !defined(SOL_LUAJIT)
			return f(L, std::forward<Args>(args)...);
#else
			try {
				return f(L, std::forward<Args>(args)...);
			}
			catch (const char* cs) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), string_view(cs));
			}
			catch (const std::string& s) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), string_view(s.c_str(), s.size()));
			}
			catch (const std::exception& e) {
				call_exception_handler(L, optional<const std::exception&>(e), e.what());
			}
#if !defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				call_exception_handler(L, optional<const std::exception&>(nullopt), "caught (...) exception");
			}
#endif
			return lua_error(L);
#endif
		}

		inline int c_trampoline(lua_State* L, lua_CFunction f) {
			return trampoline(L, f);
		}
#endif // Exceptions vs. No Exceptions

		template <typename F, F fx>
		inline int typed_static_trampoline_raw(std::true_type, lua_State* L) {
			return static_trampoline_noexcept<fx>(L);
		}

		template <typename F, F fx>
		inline int typed_static_trampoline_raw(std::false_type, lua_State* L) {
			return static_trampoline<fx>(L);
		}

		template <typename F, F fx>
		inline int typed_static_trampoline(lua_State* L) {
			return typed_static_trampoline_raw<F, fx>(std::integral_constant<bool, meta::bind_traits<F>::is_noexcept>(), L);
		}
	} // namespace detail

	inline void set_default_exception_handler(lua_State* L, exception_handler_function exf = &detail::default_exception_handler) {
		static_assert(sizeof(void*) >= sizeof(exception_handler_function), "void* storage is too small to transport the exception handler: please file a bug on the issue tracker");
		void* storage;
		std::memcpy(&storage, &exf, sizeof(exception_handler_function));
		lua_pushlightuserdata(L, storage);
		lua_setglobal(L, detail::default_exception_handler_name());
	}
} // sol

// end of sol/trampoline.hpp

// beginning of sol/stack_core.hpp

// beginning of sol/inheritance.hpp

// beginning of sol/usertype_traits.hpp

// beginning of sol/demangle.hpp

#include <cctype>
#if defined(__GNUC__) && defined(__MINGW32__) && (__GNUC__ < 6)
extern "C" {
}
#endif // MinGW is on some stuff
#include <locale>

namespace sol {
namespace detail {
#if defined(__GNUC__) || defined(__clang__)
	template <typename T, class seperator_mark = int>
	inline std::string ctti_get_type_name() {
		// cardinal sins from MINGW
		using namespace std;
		static const std::array<std::string, 2> removals = {{"{anonymous}", "(anonymous namespace)"}};
		std::string name = __PRETTY_FUNCTION__;
		std::size_t start = name.find_first_of('[');
		start = name.find_first_of('=', start);
		std::size_t end = name.find_last_of(']');
		if (end == std::string::npos)
			end = name.size();
		if (start == std::string::npos)
			start = 0;
		if (start < name.size() - 1)
			start += 1;
		name = name.substr(start, end - start);
		start = name.rfind("seperator_mark");
		if (start != std::string::npos) {
			name.erase(start - 2, name.length());
		}
		while (!name.empty() && isblank(name.front()))
			name.erase(name.begin());
		while (!name.empty() && isblank(name.back()))
			name.pop_back();

		for (std::size_t r = 0; r < removals.size(); ++r) {
			auto found = name.find(removals[r]);
			while (found != std::string::npos) {
				name.erase(found, removals[r].size());
				found = name.find(removals[r]);
			}
		}

		return name;
	}
#elif defined(_MSC_VER)
	template <typename T>
	inline std::string ctti_get_type_name() {
		static const std::array<std::string, 7> removals = {{"public:", "private:", "protected:", "struct ", "class ", "`anonymous-namespace'", "`anonymous namespace'"}};
		std::string name = __FUNCSIG__;
		std::size_t start = name.find("get_type_name");
		if (start == std::string::npos)
			start = 0;
		else
			start += 13;
		if (start < name.size() - 1)
			start += 1;
		std::size_t end = name.find_last_of('>');
		if (end == std::string::npos)
			end = name.size();
		name = name.substr(start, end - start);
		if (name.find("struct", 0) == 0)
			name.replace(0, 6, "", 0);
		if (name.find("class", 0) == 0)
			name.replace(0, 5, "", 0);
		while (!name.empty() && isblank(name.front()))
			name.erase(name.begin());
		while (!name.empty() && isblank(name.back()))
			name.pop_back();

		for (std::size_t r = 0; r < removals.size(); ++r) {
			auto found = name.find(removals[r]);
			while (found != std::string::npos) {
				name.erase(found, removals[r].size());
				found = name.find(removals[r]);
			}
		}

		return name;
	}
#else
#error Compiler not supported for demangling
#endif // compilers

	template <typename T>
	inline std::string demangle_once() {
		std::string realname = ctti_get_type_name<T>();
		return realname;
	}

	template <typename T>
	inline std::string short_demangle_once() {
		std::string realname = ctti_get_type_name<T>();
		// This isn't the most complete but it'll do for now...?
		static const std::array<std::string, 10> ops = {{"operator<", "operator<<", "operator<<=", "operator<=", "operator>", "operator>>", "operator>>=", "operator>=", "operator->", "operator->*"}};
		int level = 0;
		std::ptrdiff_t idx = 0;
		for (idx = static_cast<std::ptrdiff_t>(realname.empty() ? 0 : realname.size() - 1); idx > 0; --idx) {
			if (level == 0 && realname[idx] == ':') {
				break;
			}
			bool isleft = realname[idx] == '<';
			bool isright = realname[idx] == '>';
			if (!isleft && !isright)
				continue;
			bool earlybreak = false;
			for (const auto& op : ops) {
				std::size_t nisop = realname.rfind(op, idx);
				if (nisop == std::string::npos)
					continue;
				std::size_t nisopidx = idx - op.size() + 1;
				if (nisop == nisopidx) {
					idx = static_cast<std::ptrdiff_t>(nisopidx);
					earlybreak = true;
				}
				break;
			}
			if (earlybreak) {
				continue;
			}
			level += isleft ? -1 : 1;
		}
		if (idx > 0) {
			realname.erase(0, realname.length() < static_cast<std::size_t>(idx) ? realname.length() : idx + 1);
		}
		return realname;
	}

	template <typename T>
	inline const std::string& demangle() {
		static const std::string d = demangle_once<T>();
		return d;
	}

	template <typename T>
	inline const std::string& short_demangle() {
		static const std::string d = short_demangle_once<T>();
		return d;
	}
}
} // namespace sol::detail

// end of sol/demangle.hpp

namespace sol {

	template <typename T>
	struct usertype_traits {
		static const std::string& name() {
			static const std::string& n = detail::short_demangle<T>();
			return n;
		}
		static const std::string& qualified_name() {
			static const std::string& q_n = detail::demangle<T>();
			return q_n;
		}
		static const std::string& metatable() {
			static const std::string m = std::string("sol.").append(detail::demangle<T>());
			return m;
		}
		static const std::string& user_metatable() {
			static const std::string u_m = std::string("sol.").append(detail::demangle<T>()).append(".user");
			return u_m;
		}
		static const std::string& user_gc_metatable() {
			static const std::string u_g_m = std::string("sol.").append(detail::demangle<T>()).append(".user\xE2\x99\xBB");
			return u_g_m;
		}
		static const std::string& gc_table() {
			static const std::string g_t = std::string("sol.").append(detail::demangle<T>()).append(".\xE2\x99\xBB");
			return g_t;
		}
	};

} // namespace sol

// end of sol/usertype_traits.hpp

namespace sol {
	template <typename... Args>
	struct base_list {};
	template <typename... Args>
	using bases = base_list<Args...>;

	typedef bases<> base_classes_tag;
	const auto base_classes = base_classes_tag();

	namespace detail {

		template <typename T>
		struct has_derived {
			static bool value;
		};

		template <typename T>
		bool has_derived<T>::value = false;

		inline decltype(auto) base_class_check_key() {
			static const auto& key = "class_check";
			return key;
		}

		inline decltype(auto) base_class_cast_key() {
			static const auto& key = "class_cast";
			return key;
		}

		inline decltype(auto) base_class_index_propogation_key() {
			static const auto& key = u8"\xF0\x9F\x8C\xB2.index";
			return key;
		}

		inline decltype(auto) base_class_new_index_propogation_key() {
			static const auto& key = u8"\xF0\x9F\x8C\xB2.new_index";
			return key;
		}

		template <typename T, typename... Bases>
		struct inheritance {
			static bool type_check_bases(types<>, const std::string&) {
				return false;
			}

			template <typename Base, typename... Args>
			static bool type_check_bases(types<Base, Args...>, const std::string& ti) {
				return ti == usertype_traits<Base>::qualified_name() || type_check_bases(types<Args...>(), ti);
			}

			static bool type_check(const std::string& ti) {
				return ti == usertype_traits<T>::qualified_name() || type_check_bases(types<Bases...>(), ti);
			}

			static void* type_cast_bases(types<>, T*, const std::string&) {
				return nullptr;
			}

			template <typename Base, typename... Args>
			static void* type_cast_bases(types<Base, Args...>, T* data, const std::string& ti) {
				// Make sure to convert to T first, and then dynamic cast to the proper type
				return ti != usertype_traits<Base>::qualified_name() ? type_cast_bases(types<Args...>(), data, ti) : static_cast<void*>(static_cast<Base*>(data));
			}

			static void* type_cast(void* voiddata, const std::string& ti) {
				T* data = static_cast<T*>(voiddata);
				return static_cast<void*>(ti != usertype_traits<T>::qualified_name() ? type_cast_bases(types<Bases...>(), data, ti) : data);
			}

			template <typename U>
			static bool type_unique_cast_bases(void*, void*, const string_view&) {
				return false;
			}

			template <typename U, typename Base, typename... Args>
			static bool type_unique_cast_bases(void* source_data, void* target_data, const string_view& ti) {
				typedef unique_usertype_traits<U> uu_traits;
				typedef typename uu_traits::template rebind_base<Base> base_ptr;
				string_view base_ti = usertype_traits<Base>::qualified_name();
				if (base_ti == ti) {
					if (target_data != nullptr) {
						U* source = static_cast<U*>(source_data);
						base_ptr* target = static_cast<base_ptr*>(target_data);
						// perform proper derived -> base conversion
						*target = *source;
					}
					return true;
				}
				return type_unique_cast_bases<U, Args...>(source_data, target_data, ti);
			}

			template <typename U>
			static bool type_unique_cast(void* source_data, void* target_data, const string_view& ti, const string_view& rebind_ti) {
				typedef unique_usertype_traits<U> uu_traits;
				typedef typename uu_traits::template rebind_base<T> rebind_t;
				string_view this_rebind_ti = usertype_traits<rebind_t>::qualified_name();
				if (rebind_ti != this_rebind_ti) {
					// this is not even of the same container type
					return false;
				}
				return type_unique_cast_bases<U, Bases...>(source_data, target_data, ti);
			}
		};

		using inheritance_check_function = decltype(&inheritance<void>::type_check);
		using inheritance_cast_function = decltype(&inheritance<void>::type_cast);
		using inheritance_unique_cast_function = decltype(&inheritance<void>::type_unique_cast<void>);
	} // namespace detail
} // namespace sol

// end of sol/inheritance.hpp

// beginning of sol/error_handler.hpp

namespace sol {

	inline std::string associated_type_name(lua_State* L, int index, type t) {
		switch (t) {
		case type::poly:
			return "anything";
		case type::userdata:
		{
			if (lua_getmetatable(L, index) == 0) {
				break;
			}
			lua_pushlstring(L, "__name", 6);
			lua_rawget(L, -2);
			size_t sz;
			const char* name = lua_tolstring(L, -1, &sz);
			std::string tn(name, static_cast<std::string::size_type>(sz));
			lua_pop(L, 2);
			return name;
		}
		default:
			break;
		}
		return lua_typename(L, static_cast<int>(t));
	}

	inline int type_panic_string(lua_State* L, int index, type expected, type actual, const std::string& message = "") noexcept(false) {
		const char* err = message.empty() ? "stack index %d, expected %s, received %s" : "stack index %d, expected %s, received %s: %s";
		std::string actualname = associated_type_name(L, index, actual);
		return luaL_error(L, err, index,
			expected == type::poly ? "anything" : lua_typename(L, static_cast<int>(expected)),
			actualname.c_str(),
			message.c_str());
	}

	inline int type_panic_c_str(lua_State* L, int index, type expected, type actual, const char* message = nullptr) noexcept(false) {
		const char* err = message == nullptr || (std::char_traits<char>::length(message) == 0) ? "stack index %d, expected %s, received %s" : "stack index %d, expected %s, received %s: %s";
		std::string actualname = associated_type_name(L, index, actual);
		return luaL_error(L, err, index,
			expected == type::poly ? "anything" : lua_typename(L, static_cast<int>(expected)),
			actualname.c_str(),
			message);
	}

	struct type_panic_t {
		int operator()(lua_State* L, int index, type expected, type actual) const noexcept(false) {
			return type_panic_c_str(L, index, expected, actual, nullptr);
		}
		int operator()(lua_State* L, int index, type expected, type actual, const char* message) const noexcept(false) {
			return type_panic_c_str(L, index, expected, actual, message);
		}
		int operator()(lua_State* L, int index, type expected, type actual, const std::string& message) const noexcept(false) {
			return type_panic_string(L, index, expected, actual, message);
		}
	};

	const type_panic_t type_panic = {};

	struct constructor_handler {
		int operator()(lua_State* L, int index, type expected, type actual, const std::string& message) const noexcept(false) {
			std::string str = "(type check failed in constructor)";
			return type_panic_string(L, index, expected, actual, message.empty() ? str : message + " " + str);
		}
	};

	template <typename F = void>
	struct argument_handler {
		int operator()(lua_State* L, int index, type expected, type actual, const std::string& message) const noexcept(false) {
			std::string str = "(bad argument to variable or function call)";
			return type_panic_string(L, index, expected, actual, message.empty() ? str : message + " " + str );
		}
	};

	template <typename R, typename... Args>
	struct argument_handler<types<R, Args...>> {
		int operator()(lua_State* L, int index, type expected, type actual, const std::string& message) const noexcept(false) {
			std::string addendum = "(bad argument into '";
			addendum += detail::demangle<R>();
			addendum += "(";
			int marker = 0;
			auto action = [&addendum, &marker](const std::string& n) {
				if (marker > 0) {
					addendum += ", ";
				}
				addendum += n;
				++marker;
			};
			(void)detail::swallow{int(), (action(detail::demangle<Args>()), int())...};
			addendum += ")')";
			return type_panic_string(L, index, expected, actual, message.empty() ? addendum : message + " " + addendum);
		}
	};

	// Specify this function as the handler for lua::check if you know there's nothing wrong
	inline int no_panic(lua_State*, int, type, type, const char* = nullptr) noexcept {
		return 0;
	}

	inline void type_error(lua_State* L, int expected, int actual) noexcept(false) {
		luaL_error(L, "expected %s, received %s", lua_typename(L, expected), lua_typename(L, actual));
	}

	inline void type_error(lua_State* L, type expected, type actual) noexcept(false) {
		type_error(L, static_cast<int>(expected), static_cast<int>(actual));
	}

	inline void type_assert(lua_State* L, int index, type expected, type actual) noexcept(false) {
		if (expected != type::poly && expected != actual) {
			type_panic_c_str(L, index, expected, actual, nullptr);
		}
	}

	inline void type_assert(lua_State* L, int index, type expected) {
		type actual = type_of(L, index);
		type_assert(L, index, expected, actual);
	}

} // namespace sol

// end of sol/error_handler.hpp

// beginning of sol/reference.hpp

// beginning of sol/stack_reference.hpp

namespace sol {
	namespace detail {
		inline bool xmovable(lua_State* leftL, lua_State* rightL) {
			if (rightL == nullptr || leftL == nullptr || leftL == rightL) {
				return false;
			}
			const void* leftregistry = lua_topointer(leftL, LUA_REGISTRYINDEX);
			const void* rightregistry = lua_topointer(rightL, LUA_REGISTRYINDEX);
			return leftregistry == rightregistry;
		}
	} // namespace detail

	class stack_reference {
	private:
		lua_State* luastate = nullptr;
		int index = 0;

	protected:
		int registry_index() const noexcept {
			return LUA_NOREF;
		}

	public:
		stack_reference() noexcept = default;
		stack_reference(lua_nil_t) noexcept
		: stack_reference(){};
		stack_reference(lua_State* L, lua_nil_t) noexcept
		: luastate(L), index(0) {
		}
		stack_reference(lua_State* L, int i) noexcept
		: stack_reference(L, absolute_index(L, i)) {
		}
		stack_reference(lua_State* L, absolute_index i) noexcept
		: luastate(L), index(i) {
		}
		stack_reference(lua_State* L, raw_index i) noexcept
		: luastate(L), index(i) {
		}
		stack_reference(lua_State* L, ref_index i) noexcept = delete;
		stack_reference(lua_State* L, const reference& r) noexcept = delete;
		stack_reference(lua_State* L, const stack_reference& r) noexcept
		: luastate(L) {
			if (!r.valid()) {
				index = 0;
				return;
			}
			int i = r.stack_index();
			if (detail::xmovable(lua_state(), r.lua_state())) {
				lua_pushvalue(r.lua_state(), r.index);
				lua_xmove(r.lua_state(), luastate, 1);
				i = absolute_index(luastate, -1);
			}
			index = i;
		}
		stack_reference(stack_reference&& o) noexcept = default;
		stack_reference& operator=(stack_reference&&) noexcept = default;
		stack_reference(const stack_reference&) noexcept = default;
		stack_reference& operator=(const stack_reference&) noexcept = default;

		int push() const noexcept {
			return push(lua_state());
		}

		int push(lua_State* Ls) const noexcept {
			if (lua_state() == nullptr) {
				lua_pushnil(Ls);
				return 1;
			}
			lua_pushvalue(lua_state(), index);
			if (Ls != lua_state()) {
				lua_xmove(lua_state(), Ls, 1);
			}
			return 1;
		}

		void pop() const noexcept {
			pop(lua_state());
		}

		void pop(lua_State* Ls, int n = 1) const noexcept {
			lua_pop(Ls, n);
		}

		int stack_index() const noexcept {
			return index;
		}

		const void* pointer() const noexcept {
			const void* vp = lua_topointer(lua_state(), stack_index());
			return vp;
		}

		type get_type() const noexcept {
			int result = lua_type(lua_state(), index);
			return static_cast<type>(result);
		}

		lua_State* lua_state() const noexcept {
			return luastate;
		}

		bool valid() const noexcept {
			type t = get_type();
			return t != type::lua_nil && t != type::none;
		}
	};

	inline bool operator==(const stack_reference& l, const stack_reference& r) {
		return lua_compare(l.lua_state(), l.stack_index(), r.stack_index(), LUA_OPEQ) == 0;
	}

	inline bool operator!=(const stack_reference& l, const stack_reference& r) {
		return !operator==(l, r);
	}

	inline bool operator==(const stack_reference& lhs, const lua_nil_t&) {
		return !lhs.valid();
	}

	inline bool operator==(const lua_nil_t&, const stack_reference& rhs) {
		return !rhs.valid();
	}

	inline bool operator!=(const stack_reference& lhs, const lua_nil_t&) {
		return lhs.valid();
	}

	inline bool operator!=(const lua_nil_t&, const stack_reference& rhs) {
		return rhs.valid();
	}
} // namespace sol

// end of sol/stack_reference.hpp

namespace sol {
	namespace detail {
		inline const char (&default_main_thread_name())[9] {
			static const char name[9] = "sol.\xF0\x9F\x93\x8C";
			return name;
		}
	} // namespace detail

	namespace stack {
		inline void remove(lua_State* L, int rawindex, int count) {
			if (count < 1)
				return;
			int top = lua_gettop(L);
			if (top < 1) {
				return;
			}
			if (rawindex == -count || top == rawindex) {
				// Slice them right off the top
				lua_pop(L, static_cast<int>(count));
				return;
			}

			// Remove each item one at a time using stack operations
			// Probably slower, maybe, haven't benchmarked,
			// but necessary
			int index = lua_absindex(L, rawindex);
			if (index < 0) {
				index = lua_gettop(L) + (index + 1);
			}
			int last = index + count;
			for (int i = index; i < last; ++i) {
				lua_remove(L, index);
			}
		}

		struct push_popper_at {
			lua_State* L;
			int index;
			int count;
			push_popper_at(lua_State* luastate, int index = -1, int count = 1)
			: L(luastate), index(index), count(count) {
			}
			~push_popper_at() {
				remove(L, index, count);
			}
		};

		template <bool top_level>
		struct push_popper_n {
			lua_State* L;
			int t;
			push_popper_n(lua_State* luastate, int x)
			: L(luastate), t(x) {
			}
			push_popper_n(const push_popper_n&) = delete;
			push_popper_n(push_popper_n&&) = default;
			push_popper_n& operator=(const push_popper_n&) = delete;
			push_popper_n& operator=(push_popper_n&&) = default;
			~push_popper_n() {
				lua_pop(L, t);
			}
		};
		template <>
		struct push_popper_n<true> {
			push_popper_n(lua_State*, int) {
			}
		};
		template <bool, typename T, typename = void>
		struct push_popper {
			T t;
			push_popper(T x)
			: t(x) {
				t.push();
			}
			~push_popper() {
				t.pop();
			}
		};
		template <typename T, typename C>
		struct push_popper<true, T, C> {
			push_popper(T) {
			}
			~push_popper() {
			}
		};
		template <typename T>
		struct push_popper<false, T, std::enable_if_t<std::is_base_of<stack_reference, meta::unqualified_t<T>>::value>> {
			push_popper(T) {
			}
			~push_popper() {
			}
		};

		template <bool top_level = false, typename T>
		push_popper<top_level, T> push_pop(T&& x) {
			return push_popper<top_level, T>(std::forward<T>(x));
		}
		template <typename T>
		push_popper_at push_pop_at(T&& x) {
			int c = x.push();
			lua_State* L = x.lua_state();
			return push_popper_at(L, lua_absindex(L, -c), c);
		}
		template <bool top_level = false>
		push_popper_n<top_level> pop_n(lua_State* L, int x) {
			return push_popper_n<top_level>(L, x);
		}
	} // namespace stack

	inline lua_State* main_thread(lua_State* L, lua_State* backup_if_unsupported = nullptr) {
#if SOL_LUA_VERSION < 502
		if (L == nullptr)
			return backup_if_unsupported;
		lua_getglobal(L, detail::default_main_thread_name());
		auto pp = stack::pop_n(L, 1);
		if (type_of(L, -1) == type::thread) {
			return lua_tothread(L, -1);
		}
		return backup_if_unsupported;
#else
		if (L == nullptr)
			return backup_if_unsupported;
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
		lua_State* Lmain = lua_tothread(L, -1);
		lua_pop(L, 1);
		return Lmain;
#endif // Lua 5.2+ has the main thread getter
	}

	namespace detail {
		struct global_tag {
		} const global_{};
		struct no_safety_tag {
		} const no_safety{};

		template <bool b>
		inline lua_State* pick_main_thread(lua_State* L, lua_State* backup_if_unsupported = nullptr) {
			(void)L;
			(void)backup_if_unsupported;
			if (b) {
				return main_thread(L, backup_if_unsupported);
			}
			return L;
		}
	} // namespace detail

	template <bool main_only = false>
	class basic_reference {
	private:
		template <bool o_main_only>
		friend class basic_reference;
		lua_State* luastate = nullptr; // non-owning
		int ref = LUA_NOREF;

		int copy() const noexcept {
			if (ref == LUA_NOREF)
				return LUA_NOREF;
			push();
			return luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}

		template <bool r_main_only>
		void copy_assign(const basic_reference<r_main_only>& r) {
			if (valid()) {
				deref();
			}
			if (r.ref == LUA_REFNIL) {
				luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF) {
				luastate = r.luastate;
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
			ref = r.copy();
		}

		template <bool r_main_only>
		void move_assign(basic_reference<r_main_only>&& r) {
			if (valid()) {
				deref();
			}
			if (r.ref == LUA_REFNIL) {
				luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF) {
				luastate = r.luastate;
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}

			luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
			ref = r.ref;
			r.ref = LUA_NOREF;
			r.luastate = nullptr;
		}

	protected:
		basic_reference(lua_State* L, detail::global_tag) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			lua_pushglobaltable(lua_state());
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}

		int stack_index() const noexcept {
			return -1;
		}

		void deref() const noexcept {
			luaL_unref(lua_state(), LUA_REGISTRYINDEX, ref);
		}

	public:
		basic_reference() noexcept = default;
		basic_reference(lua_nil_t) noexcept
		: basic_reference() {
		}
		basic_reference(const stack_reference& r) noexcept
		: basic_reference(r.lua_state(), r.stack_index()) {
		}
		basic_reference(stack_reference&& r) noexcept
		: basic_reference(r.lua_state(), r.stack_index()) {
		}
		template <bool r_main_only>
		basic_reference(lua_State* L, const basic_reference<r_main_only>& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (r.ref == LUA_REFNIL) {
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF || lua_state() == nullptr) {
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			ref = r.copy();
		}

		template <bool r_main_only>
		basic_reference(lua_State* L, basic_reference<r_main_only>&& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (r.ref == LUA_REFNIL) {
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF || lua_state() == nullptr) {
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			ref = r.ref;
			r.ref = LUA_NOREF;
			r.luastate = nullptr;
		}

		basic_reference(lua_State* L, const stack_reference& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (lua_state() == nullptr || r.lua_state() == nullptr || r.get_type() == type::none) {
				ref = LUA_NOREF;
				return;
			}
			if (r.get_type() == type::lua_nil) {
				ref = LUA_REFNIL;
				return;
			}
			if (lua_state() != r.lua_state() && !detail::xmovable(lua_state(), r.lua_state())) {
				return;
			}
			r.push(lua_state());
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, int index = -1) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			// use L to stick with that state's execution stack
			lua_pushvalue(L, index);
			ref = luaL_ref(L, LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, ref_index index) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			lua_rawgeti(lua_state(), LUA_REGISTRYINDEX, index.index);
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, lua_nil_t) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
		}

		~basic_reference() noexcept {
			if (lua_state() == nullptr || ref == LUA_NOREF)
				return;
			deref();
		}

		basic_reference(const basic_reference& o) noexcept
		: luastate(o.lua_state()), ref(o.copy()) {
		}

		basic_reference(basic_reference&& o) noexcept
		: luastate(o.lua_state()), ref(o.ref) {
			o.luastate = nullptr;
			o.ref = LUA_NOREF;
		}

		basic_reference(const basic_reference<!main_only>& o) noexcept
		: luastate(detail::pick_main_thread < main_only && !main_only > (o.lua_state(), o.lua_state())), ref(o.copy()) {
		}

		basic_reference(basic_reference<!main_only>&& o) noexcept
		: luastate(detail::pick_main_thread < main_only && !main_only > (o.lua_state(), o.lua_state())), ref(o.ref) {
			o.luastate = nullptr;
			o.ref = LUA_NOREF;
		}

		basic_reference& operator=(basic_reference&& r) noexcept {
			move_assign(std::move(r));
			return *this;
		}

		basic_reference& operator=(const basic_reference& r) noexcept {
			copy_assign(r);
			return *this;
		}

		basic_reference& operator=(basic_reference<!main_only>&& r) noexcept {
			move_assign(std::move(r));
			return *this;
		}

		basic_reference& operator=(const basic_reference<!main_only>& r) noexcept {
			copy_assign(r);
			return *this;
		}

		basic_reference& operator=(const lua_nil_t&) noexcept {
			if (valid()) {
				deref();
			}
			luastate = nullptr;
			ref = LUA_NOREF;
			return *this;
		}

		template <typename Super>
		basic_reference& operator=(proxy_base<Super>&& r);

		template <typename Super>
		basic_reference& operator=(const proxy_base<Super>& r);

		int push() const noexcept {
			return push(lua_state());
		}

		int push(lua_State* Ls) const noexcept {
			if (lua_state() == nullptr) {
				lua_pushnil(Ls);
				return 1;
			}
			lua_rawgeti(lua_state(), LUA_REGISTRYINDEX, ref);
			if (Ls != lua_state()) {
				lua_xmove(lua_state(), Ls, 1);
			}
			return 1;
		}

		void pop() const noexcept {
			pop(lua_state());
		}

		void pop(lua_State* Ls, int n = 1) const noexcept {
			lua_pop(Ls, n);
		}

		int registry_index() const noexcept {
			return ref;
		}

		bool valid() const noexcept {
			return !(ref == LUA_NOREF || ref == LUA_REFNIL);
		}

		const void* pointer() const noexcept {
			int si = push();
			const void* vp = lua_topointer(lua_state(), -si);
			lua_pop(this->lua_state(), si);
			return vp;
		}

		explicit operator bool() const noexcept {
			return valid();
		}

		type get_type() const noexcept {
			auto pp = stack::push_pop(*this);
			int result = lua_type(lua_state(), -1);
			return static_cast<type>(result);
		}

		lua_State* lua_state() const noexcept {
			return luastate;
		}
	};

	template <bool lb, bool rb>
	inline bool operator==(const basic_reference<lb>& l, const basic_reference<rb>& r) {
		auto ppl = stack::push_pop(l);
		auto ppr = stack::push_pop(r);
		return lua_compare(l.lua_state(), -1, -2, LUA_OPEQ) == 1;
	}

	template <bool lb, bool rb>
	inline bool operator!=(const basic_reference<lb>& l, const basic_reference<rb>& r) {
		return !operator==(l, r);
	}

	template <bool lb>
	inline bool operator==(const basic_reference<lb>& lhs, const lua_nil_t&) {
		return !lhs.valid();
	}

	template <bool rb>
	inline bool operator==(const lua_nil_t&, const basic_reference<rb>& rhs) {
		return !rhs.valid();
	}

	template <bool lb>
	inline bool operator!=(const basic_reference<lb>& lhs, const lua_nil_t&) {
		return lhs.valid();
	}

	template <bool rb>
	inline bool operator!=(const lua_nil_t&, const basic_reference<rb>& rhs) {
		return rhs.valid();
	}
} // namespace sol

// end of sol/reference.hpp

// beginning of sol/tie.hpp

namespace sol {

	namespace detail {
		template <typename T>
		struct is_speshul : std::false_type {};
	} // namespace detail

	template <typename T>
	struct tie_size : std::tuple_size<T> {};

	template <typename T>
	struct is_tieable : std::integral_constant<bool, (::sol::tie_size<T>::value > 0)> {};

	template <typename... Tn>
	struct tie_t : public std::tuple<std::add_lvalue_reference_t<Tn>...> {
	private:
		typedef std::tuple<std::add_lvalue_reference_t<Tn>...> base_t;

		template <typename T>
		void set(std::false_type, T&& target) {
			std::get<0>(*this) = std::forward<T>(target);
		}

		template <typename T>
		void set(std::true_type, T&& target) {
			typedef tie_size<meta::unqualified_t<T>> value_size;
			typedef tie_size<std::tuple<Tn...>> tie_size;
			typedef std::conditional_t<(value_size::value < tie_size::value), value_size, tie_size> indices_size;
			typedef std::make_index_sequence<indices_size::value> indices;
			set_extra(detail::is_speshul<meta::unqualified_t<T>>(), indices(), std::forward<T>(target));
		}

		template <std::size_t... I, typename T>
		void set_extra(std::true_type, std::index_sequence<I...>, T&& target) {
			using std::get;
			(void)detail::swallow{0,
				(get<I>(static_cast<base_t&>(*this)) = get<I>(types<Tn...>(), target), 0)..., 0};
		}

		template <std::size_t... I, typename T>
		void set_extra(std::false_type, std::index_sequence<I...>, T&& target) {
			using std::get;
			(void)detail::swallow{0,
				(get<I>(static_cast<base_t&>(*this)) = get<I>(target), 0)..., 0};
		}

	public:
		using base_t::base_t;

		template <typename T>
		tie_t& operator=(T&& value) {
			typedef is_tieable<meta::unqualified_t<T>> tieable;
			set(tieable(), std::forward<T>(value));
			return *this;
		}
	};

	template <typename... Tn>
	struct tie_size<tie_t<Tn...>> : std::tuple_size<std::tuple<Tn...>> {};

	namespace adl_barrier_detail {
		template <typename... Tn>
		inline tie_t<std::remove_reference_t<Tn>...> tie(Tn&&... argn) {
			return tie_t<std::remove_reference_t<Tn>...>(std::forward<Tn>(argn)...);
		}
	} // namespace adl_barrier_detail

	using namespace adl_barrier_detail;

} // namespace sol

// end of sol/tie.hpp

// beginning of sol/stack_guard.hpp

namespace sol {
	namespace detail {
		inline void stack_fail(int, int) {
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
			throw error(detail::direct_error, "imbalanced stack after operation finish");
#else
			// Lol, what do you want, an error printout? :3c
			// There's no sane default here. The right way would be C-style abort(), and that's not acceptable, so
			// hopefully someone will register their own stack_fail thing for the `fx` parameter of stack_guard.
#endif // No Exceptions
		}
	} // namespace detail

	struct stack_guard {
		lua_State* L;
		int top;
		std::function<void(int, int)> on_mismatch;

		stack_guard(lua_State* L)
		: stack_guard(L, lua_gettop(L)) {
		}
		stack_guard(lua_State* L, int top, std::function<void(int, int)> fx = detail::stack_fail)
		: L(L), top(top), on_mismatch(std::move(fx)) {
		}
		bool check_stack(int modification = 0) const {
			int bottom = lua_gettop(L) + modification;
			if (top == bottom) {
				return true;
			}
			on_mismatch(top, bottom);
			return false;
		}
		~stack_guard() {
			check_stack();
		}
	};
} // namespace sol

// end of sol/stack_guard.hpp

#include <vector>
#include <forward_list>
#include <algorithm>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#endif // C++17

namespace sol {
	namespace detail {
		struct as_reference_tag {};
		template <typename T>
		struct as_pointer_tag {};
		template <typename T>
		struct as_value_tag {};
		template <typename T>
		struct as_table_tag {};

		using unique_destructor = void (*)(void*);
#if 0
		using unique_tag = detail::inheritance_unique_cast_function;
#else
		using unique_tag = const char*;
#endif

		inline void* align(std::size_t alignment, std::size_t size, void*& ptr, std::size_t& space, std::size_t& required_space) {
			// this handels arbitrary alignments...
			// make this into a power-of-2-only?
			// actually can't: this is a C++14-compatible framework,
			// power of 2 alignment is C++17
			std::uintptr_t initial = reinterpret_cast<std::uintptr_t>(ptr);
			std::uintptr_t offby = static_cast<std::uintptr_t>(initial % alignment);
			std::uintptr_t padding = (alignment - offby) % alignment;
			required_space += size + padding;
			if (space < required_space) {
				return nullptr;
			}
			ptr = static_cast<void*>(static_cast<char*>(ptr) + padding);
			space -= padding;
			return ptr;
		}

		inline void* align(std::size_t alignment, std::size_t size, void*& ptr, std::size_t& space) {
			std::size_t required_space = 0;
			return align(alignment, size, ptr, space, required_space);
		}

		template <typename... Args>
		inline std::size_t aligned_space_for(void* alignment = nullptr) {
			char* start = static_cast<char*>(alignment);
			auto specific_align = [&alignment](std::size_t a, std::size_t s) {
				std::size_t space = (std::numeric_limits<std::size_t>::max)();
				alignment = align(a, s, alignment, space);
				alignment = static_cast<void*>(static_cast<char*>(alignment) + s);
			};
			(void)detail::swallow{ int{}, (specific_align(std::alignment_of<Args>::value, sizeof(Args)), int{})... };
			return static_cast<char*>(alignment) - start;
		}

		inline void* align_usertype_pointer(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<void*>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<void*>::value, sizeof(void*), ptr, space);
		}

		template <bool pre_aligned = false>
		inline void* align_usertype_unique_destructor(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<unique_destructor>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_pointer(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(void*));
			}
			if (!use_align::value) {
				return static_cast<void*>(static_cast<void**>(ptr) + 1);
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<unique_destructor>::value, sizeof(unique_destructor), ptr, space);
		}

		template <bool pre_aligned = false>
		inline void* align_usertype_unique_tag(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<unique_tag>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_unique_destructor(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(unique_destructor));
			}
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<unique_tag>::value, sizeof(unique_tag), ptr, space);
		}
		template <typename T, bool pre_aligned = false>
		inline void* align_usertype_unique(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_unique_tag(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(unique_tag));
			}
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<T>::value, sizeof(T), ptr, space);
		}

		template <typename T>
		inline void* align_user(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<T>::value, sizeof(T), ptr, space);
		}

		template <typename T>
		inline T** usertype_allocate_pointer(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T** pointerpointer = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));
				return pointerpointer;
			}
			static const std::size_t initial_size = aligned_space_for<T*>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*>(reinterpret_cast<void*>(0x1));

			std::size_t allocated_size = initial_size;
			void* unadjusted = lua_newuserdata(L, initial_size);
			void* adjusted = align(std::alignment_of<T*>::value, sizeof(T*), unadjusted, allocated_size);
			if (adjusted == nullptr) {
				lua_pop(L, 1);
				// what kind of absolute garbage trash allocator are we dealing with?
				// whatever, add some padding in the case of MAXIMAL alignment waste...
				allocated_size = misaligned_size;
				unadjusted = lua_newuserdata(L, allocated_size);
				adjusted = align(std::alignment_of<T*>::value, sizeof(T*), unadjusted, allocated_size);
				if (adjusted == nullptr) {
					// trash allocator can burn in hell
					lua_pop(L, 1);
					//luaL_error(L, "if you are the one that wrote this allocator you should feel bad for doing a worse job than malloc/realloc and should go read some books, yeah?");
					luaL_error(L, "cannot properly align memory for '%s'", detail::demangle<T*>().data());
				}
			}
			return static_cast<T**>(adjusted);
		}

		template <typename T>
		inline T* usertype_allocate(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1 || std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T** pointerpointer = static_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(T)));
				T*& pointerreference = *pointerpointer;
				T* allocationtarget = reinterpret_cast<T*>(pointerpointer + 1);
				pointerreference = allocationtarget;
				return allocationtarget;
			}

			/* the assumption is that `lua_newuserdata` -- unless someone
			passes a specific lua_Alloc that gives us bogus, un-aligned pointers
			-- uses malloc, which tends to hand out more or less aligned pointers to memory
			(most of the time, anyhow)

			but it's not guaranteed, so we have to do a post-adjustment check and increase padding

			we do this preliminarily with compile-time stuff, to see
			if we strike lucky with the allocator and alignment values

			otherwise, we have to re-allocate the userdata and
			over-allocate some space for additional padding because
			compilers are optimized for aligned reads/writes
			(and clang will barf UBsan errors on us for not being aligned)
			*/
			static const std::size_t initial_size = aligned_space_for<T*, T>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*, T>(reinterpret_cast<void*>(0x1));

			void* pointer_adjusted;
			void* data_adjusted;
			auto attempt_alloc = [](lua_State* L, std::size_t allocated_size, void*& pointer_adjusted, void*& data_adjusted) -> bool {
				void* adjusted = lua_newuserdata(L, allocated_size);
				pointer_adjusted = align(std::alignment_of<T*>::value, sizeof(T*), adjusted, allocated_size);
				if (pointer_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				// subtract size of what we're going to allocate there
				allocated_size -= sizeof(T*);
				adjusted = static_cast<void*>(static_cast<char*>(pointer_adjusted) + sizeof(T*));
				data_adjusted = align(std::alignment_of<T>::value, sizeof(T), adjusted, allocated_size);
				if (data_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				return true;
			};
			bool result = attempt_alloc(L, initial_size, pointer_adjusted, data_adjusted);
			if (!result) {
				// we're likely to get something that fails to perform the proper allocation a second time,
				// so we use the suggested_new_size bump to help us out here
				pointer_adjusted = nullptr;
				data_adjusted = nullptr;
				result = attempt_alloc(L, misaligned_size, pointer_adjusted, data_adjusted);
				if (!result) {
					if (pointer_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (pointer section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else {
						luaL_error(L, "aligned allocation of userdata block (data section) for '%s' failed", detail::demangle<T>().c_str());
					}
					return nullptr;
				}
			}

			T** pointerpointer = reinterpret_cast<T**>(pointer_adjusted);
			T*& pointerreference = *pointerpointer;
			T* allocationtarget = reinterpret_cast<T*>(data_adjusted);
			pointerreference = allocationtarget;
			return allocationtarget;
		}

		template <typename T, typename Real>
		inline Real* usertype_unique_allocate(lua_State* L, T**& pref, unique_destructor*& dx, unique_tag*& id) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1 || std::alignment_of<unique_tag>::value > 1 || std::alignment_of<unique_destructor>::value > 1 || std::alignment_of<Real>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				pref = static_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(detail::unique_destructor) + sizeof(unique_tag) + sizeof(Real)));
				dx = static_cast<detail::unique_destructor*>(static_cast<void*>(pref + 1));
				id = static_cast<unique_tag*>(static_cast<void*>(dx + 1));
				Real* mem = static_cast<Real*>(static_cast<void*>(id + 1));
				return mem;
			}

			static const std::size_t initial_size = aligned_space_for<T*, unique_destructor, unique_tag, Real>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*, unique_destructor, unique_tag, Real>(reinterpret_cast<void*>(0x1));

			void* pointer_adjusted;
			void* dx_adjusted;
			void* id_adjusted;
			void* data_adjusted;
			auto attempt_alloc = [](lua_State* L, std::size_t allocated_size, void*& pointer_adjusted, void*& dx_adjusted, void*& id_adjusted, void*& data_adjusted) -> bool {
				void* adjusted = lua_newuserdata(L, allocated_size);
				pointer_adjusted = align(std::alignment_of<T*>::value, sizeof(T*), adjusted, allocated_size);
				if (pointer_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(T*);

				adjusted = static_cast<void*>(static_cast<char*>(pointer_adjusted) + sizeof(T*));
				dx_adjusted = align(std::alignment_of<unique_destructor>::value, sizeof(unique_destructor), adjusted, allocated_size);
				if (dx_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(unique_destructor);

				adjusted = static_cast<void*>(static_cast<char*>(dx_adjusted) + sizeof(unique_destructor));

				id_adjusted = align(std::alignment_of<unique_tag>::value, sizeof(unique_tag), adjusted, allocated_size);
				if (id_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(unique_tag);

				adjusted = static_cast<void*>(static_cast<char*>(id_adjusted) + sizeof(unique_tag));
				data_adjusted = align(std::alignment_of<Real>::value, sizeof(Real), adjusted, allocated_size);
				if (data_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				return true;
			};
			bool result = attempt_alloc(L, initial_size, pointer_adjusted, dx_adjusted, id_adjusted, data_adjusted);
			if (!result) {
				// we're likely to get something that fails to perform the proper allocation a second time,
				// so we use the suggested_new_size bump to help us out here
				pointer_adjusted = nullptr;
				dx_adjusted = nullptr;
				id_adjusted = nullptr;
				data_adjusted = nullptr;
				result = attempt_alloc(L, misaligned_size, pointer_adjusted, dx_adjusted, id_adjusted, data_adjusted);
				if (!result) {
					if (pointer_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (pointer section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else if (dx_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (deleter section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else {
						luaL_error(L, "aligned allocation of userdata block (data section) for '%s' failed", detail::demangle<T>().c_str());
					}
					return nullptr;
				}
			}

			pref = static_cast<T**>(pointer_adjusted);
			dx = static_cast<detail::unique_destructor*>(dx_adjusted);
			id = static_cast<unique_tag*>(id_adjusted);
			Real* mem = static_cast<Real*>(data_adjusted);
			return mem;
		}

		template <typename T>
		inline T* user_allocate(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T* pointer = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
				return pointer;
			}

			static const std::size_t initial_size = aligned_space_for<T>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T>(reinterpret_cast<void*>(0x1));

			std::size_t allocated_size = initial_size;
			void* unadjusted = lua_newuserdata(L, allocated_size);
			void* adjusted = align(std::alignment_of<T>::value, sizeof(T), unadjusted, allocated_size);
			if (adjusted == nullptr) {
				lua_pop(L, 1);
				// try again, add extra space for alignment padding
				allocated_size = misaligned_size;
				unadjusted = lua_newuserdata(L, allocated_size);
				adjusted = align(std::alignment_of<T>::value, sizeof(T), unadjusted, allocated_size);
				if (adjusted == nullptr) {
					lua_pop(L, 1);
					luaL_error(L, "cannot properly align memory for '%s'", detail::demangle<T>().data());
				}
			}
			return static_cast<T*>(adjusted);
		}

		template <typename T>
		inline int usertype_alloc_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_usertype_pointer(memory);
			T** pdata = static_cast<T**>(memory);
			T* data = *pdata;
			std::allocator<T> alloc{};
			std::allocator_traits<std::allocator<T>>::destroy(alloc, data);
			return 0;
		}

		template <typename T>
		inline int unique_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_usertype_unique_destructor(memory);
			unique_destructor& dx = *static_cast<unique_destructor*>(memory);
			memory = static_cast<void*>(static_cast<char*>(memory) + sizeof(unique_destructor));
			memory = align_usertype_unique_tag<true>(memory);
			memory = static_cast<void*>(static_cast<char*>(memory) + sizeof(unique_tag));
			(dx)(memory);
			return 0;
		}

		template <typename T>
		inline int user_alloc_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_user<T>(memory);
			T* data = static_cast<T*>(memory);
			std::allocator<T> alloc;
			std::allocator_traits<std::allocator<T>>::destroy(alloc, data);
			return 0;
		}

		template <typename T, typename Real>
		inline void usertype_unique_alloc_destroy(void* memory) {
			memory = align_usertype_unique<Real, true>(memory);
			Real* target = static_cast<Real*>(memory);
			std::allocator<Real> alloc;
			std::allocator_traits<std::allocator<Real>>::destroy(alloc, target);
		}

		template <typename T>
		inline int cannot_destruct(lua_State* L) {
			return luaL_error(L, "cannot call the destructor for '%s': it is either hidden (protected/private) or removed with '= delete' and thusly this type is being destroyed without properly destructing, invoking undefined behavior: please bind a usertype and specify a custom destructor to define the behavior properly", detail::demangle<T>().data());
		}

		template <typename T>
		void reserve(T&, std::size_t) {
		}

		template <typename T, typename Al>
		void reserve(std::vector<T, Al>& arr, std::size_t hint) {
			arr.reserve(hint);
		}

		template <typename T, typename Tr, typename Al>
		void reserve(std::basic_string<T, Tr, Al>& arr, std::size_t hint) {
			arr.reserve(hint);
		}
	} // namespace detail

	namespace stack {

		template <typename T>
		struct extensible {};

		template <typename T, bool global = false, bool raw = false, typename = void>
		struct field_getter;
		template <typename T, typename P, bool global = false, bool raw = false, typename = void>
		struct probe_field_getter;
		template <typename T, bool global = false, bool raw = false, typename = void>
		struct field_setter;
		template <typename T, typename = void>
		struct getter;
		template <typename T, typename = void>
		struct qualified_getter;
		template <typename T, typename = void>
		struct userdata_getter;
		template <typename T, typename = void>
		struct popper;
		template <typename T, typename = void>
		struct pusher;
		template <typename T, type = lua_type_of<T>::value, typename = void>
		struct checker;
		template <typename T, type = lua_type_of<T>::value, typename = void>
		struct qualified_checker;
		template <typename T, typename = void>
		struct userdata_checker;
		template <typename T, typename = void>
		struct check_getter;
		template <typename T, typename = void>
		struct qualified_check_getter;

		struct probe {
			bool success;
			int levels;

			probe(bool s, int l)
			: success(s), levels(l) {
			}

			operator bool() const {
				return success;
			};
		};

		struct record {
			int last;
			int used;

			record()
			: last(), used() {
			}
			void use(int count) {
				last = count;
				used += count;
			}
		};

		namespace stack_detail {
			template <typename T>
			struct strip {
				typedef T type;
			};
			template <typename T>
			struct strip<std::reference_wrapper<T>> {
				typedef T& type;
			};
			template <typename T>
			struct strip<user<T>> {
				typedef T& type;
			};
			template <typename T>
			struct strip<non_null<T>> {
				typedef T type;
			};
			template <typename T>
			using strip_t = typename strip<T>::type;

			template <typename T>
			struct strip_extensible { typedef T type; };

			template <typename T>
			struct strip_extensible<extensible<T>> { typedef T type; };

			template <typename T>
			using strip_extensible_t = typename strip_extensible<T>::type;

			template <typename C>
			static int get_size_hint(const C& c) {
				return static_cast<int>(c.size());
			}

			template <typename V, typename Al>
			static int get_size_hint(const std::forward_list<V, Al>&) {
				// forward_list makes me sad
				return static_cast<int>(32);
			}

			template <typename T>
			inline decltype(auto) unchecked_unqualified_get(lua_State* L, int index, record& tracking) {
				typedef meta::unqualified_t<T> Tu;
				getter<Tu> g{};
				(void)g;
				return g.get(L, index, tracking);
			}

			template <typename T>
			inline decltype(auto) unchecked_get(lua_State* L, int index, record& tracking) {
				qualified_getter<T> g{};
				(void)g;
				return g.get(L, index, tracking);
			}

			template <typename T, typename Arg, typename... Args>
			inline int push_reference(lua_State* L, Arg&& arg, Args&&... args) {
				typedef meta::all<
					std::is_lvalue_reference<T>,
					meta::neg<std::is_const<T>>,
					meta::neg<is_lua_primitive<meta::unqualified_t<T>>>,
					meta::neg<is_unique_usertype<meta::unqualified_t<T>>>>
					use_reference_tag;
				pusher<std::conditional_t<use_reference_tag::value, detail::as_reference_tag, meta::unqualified_t<T>>> p{};
				(void)p;
				return p.push(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
			}

			template <typename T, typename Handler>
			bool check_usertype(std::false_type, lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
				typedef meta::unqualified_t<T> Tu;
				typedef detail::as_value_tag<Tu> detail_t;
				return checker<detail_t, type::userdata>{}.check(types<meta::unqualified_t<T>>(), L, index, indextype, std::forward<Handler>(handler), tracking);
			}

			template <typename T, typename Handler>
			bool check_usertype(std::true_type, lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
				typedef meta::unqualified_t<std::remove_pointer_t<meta::unqualified_t<T>>> Tu;
				typedef detail::as_pointer_tag<Tu> detail_t;
				return checker<detail_t, type::userdata>{}.check(L, index, indextype, std::forward<Handler>(handler), tracking);
			}
		} // namespace stack_detail

		inline bool maybe_indexable(lua_State* L, int index = -1) {
			type t = type_of(L, index);
			return t == type::userdata || t == type::table;
		}

		inline int top(lua_State* L) {
			return lua_gettop(L);
		}

		inline bool is_main_thread(lua_State* L) {
			int ismainthread = lua_pushthread(L);
			lua_pop(L, 1);
			return ismainthread == 1;
		}

		inline void coroutine_create_guard(lua_State* L) {
			if (is_main_thread(L)) {
				return;
			}
			int stacksize = lua_gettop(L);
			if (stacksize < 1) {
				return;
			}
			if (type_of(L, 1) != type::function) {
				return;
			}
			// well now we're screwed...
			// we can clean the stack and pray it doesn't destroy anything?
			lua_pop(L, stacksize);
		}

		template <typename T, typename... Args>
		inline int push(lua_State* L, T&& t, Args&&... args) {
			return pusher<meta::unqualified_t<T>>{}.push(L, std::forward<T>(t), std::forward<Args>(args)...);
		}

		// overload allows to use a pusher of a specific type, but pass in any kind of args
		template <typename T, typename Arg, typename... Args, typename = std::enable_if_t<!std::is_same<T, Arg>::value>>
		inline int push(lua_State* L, Arg&& arg, Args&&... args) {
			return pusher<meta::unqualified_t<T>>{}.push(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
		}

		template <typename T, typename... Args>
		inline int push_reference(lua_State* L, T&& t, Args&&... args) {
			return stack_detail::push_reference<T>(L, std::forward<T>(t), std::forward<Args>(args)...);
		}

		template <typename T, typename Arg, typename... Args>
		inline int push_reference(lua_State* L, Arg&& arg, Args&&... args) {
			return stack_detail::push_reference<T>(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
		}

		inline int multi_push(lua_State*) {
			// do nothing
			return 0;
		}

		template <typename T, typename... Args>
		inline int multi_push(lua_State* L, T&& t, Args&&... args) {
			int pushcount = push(L, std::forward<T>(t));
			void(detail::swallow{ (pushcount += stack::push(L, std::forward<Args>(args)), 0)... });
			return pushcount;
		}

		inline int multi_push_reference(lua_State*) {
			// do nothing
			return 0;
		}

		template <typename T, typename... Args>
		inline int multi_push_reference(lua_State* L, T&& t, Args&&... args) {
			int pushcount = push_reference(L, std::forward<T>(t));
			void(detail::swallow{ (pushcount += stack::push_reference(L, std::forward<Args>(args)), 0)... });
			return pushcount;
		}

		template <typename T, typename Handler>
		bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			qualified_checker<T> c;
			// VC++ has a bad warning here: shut it up
			(void)c;
			return c.check(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool check(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		bool unqualified_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef meta::unqualified_t<T> Tu;
			checker<Tu> c;
			// VC++ has a bad warning here: shut it up
			(void)c;
			return c.check(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool unqualified_check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return unqualified_check<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool unqualified_check(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return unqualified_check<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		bool check_usertype(lua_State* L, int index, Handler&& handler, record& tracking) {
			type indextype = type_of(L, index);
			return stack_detail::check_usertype<T>(std::is_pointer<T>(), L, index, indextype, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool check_usertype(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check_usertype<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool check_usertype(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check_usertype<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef meta::unqualified_t<T> Tu;
			check_getter<Tu> cg{};
			(void)cg;
			return cg.get(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return unqualified_check_get<T>(L, index, handler, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return unqualified_check_get<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		inline decltype(auto) check_get(lua_State* L, int index, Handler&& handler, record& tracking) {
			qualified_check_getter<T> cg{};
			(void)cg;
			return cg.get(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		inline decltype(auto) check_get(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check_get<T>(L, index, handler, tracking);
		}

		template <typename T>
		inline decltype(auto) check_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check_get<T>(L, index, handler);
		}

		namespace stack_detail {

#if defined(SOL_SAFE_GETTER) && SOL_SAFE_GETTER
			template <typename T>
			inline auto tagged_unqualified_get(types<T>, lua_State* L, int index, record& tracking) -> decltype(stack_detail::unchecked_unqualified_get<T>(L, index, tracking)) {
				if (is_lua_reference<T>::value) {
					return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
				}
				auto op = unqualified_check_get<T>(L, index, type_panic_c_str, tracking);
				return *std::move(op);
			}

			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<optional<T>>(L, index, tracking);
			}

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<std::optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<std::optional<T>>(L, index, tracking);
			}
#endif // shitty optional

			template <typename T>
			inline auto tagged_get(types<T>, lua_State* L, int index, record& tracking) -> decltype(stack_detail::unchecked_get<T>(L, index, tracking)) {
				if (is_lua_reference<T>::value) {
					return stack_detail::unchecked_get<T>(L, index, tracking);
				}
				auto op = check_get<T>(L, index, type_panic_c_str, tracking);
				return *std::move(op);
			}

			template <typename T>
			inline decltype(auto) tagged_get(types<optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<optional<T>>(L, index, tracking);
			}

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			template <typename T>
			inline decltype(auto) tagged_get(types<std::optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<std::optional<T>>(L, index, tracking);
			}
#endif // shitty optional

#else
			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<T>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
			}

			template <typename T>
			inline decltype(auto) tagged_get(types<T>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<T>(L, index, tracking);
			}
#endif

			template <bool b>
			struct check_types {
				template <typename T, typename... Args, typename Handler>
				static bool check(types<T, Args...>, lua_State* L, int firstargument, Handler&& handler, record& tracking) {
					if (!stack::check<T>(L, firstargument + tracking.used, handler, tracking))
						return false;
					return check(types<Args...>(), L, firstargument, std::forward<Handler>(handler), tracking);
				}

				template <typename Handler>
				static bool check(types<>, lua_State*, int, Handler&&, record&) {
					return true;
				}
			};

			template <>
			struct check_types<false> {
				template <typename... Args, typename Handler>
				static bool check(types<Args...>, lua_State*, int, Handler&&, record&) {
					return true;
				}
			};

		} // namespace stack_detail

		template <bool b, typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack_detail::check_types<b>{}.check(types<meta::unqualified_t<Args>...>(), L, index, std::forward<Handler>(handler), tracking);
		}

		template <bool b, typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return multi_check<b, Args...>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <bool b, typename... Args>
		bool multi_check(lua_State* L, int index) {
			auto handler = no_panic;
			return multi_check<b, Args...>(L, index, handler);
		}

		template <typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return multi_check<true, Args...>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler) {
			return multi_check<true, Args...>(L, index, std::forward<Handler>(handler));
		}

		template <typename... Args>
		bool multi_check(lua_State* L, int index) {
			return multi_check<true, Args...>(L, index);
		}

		template <typename T>
		inline decltype(auto) get_usertype(lua_State* L, int index, record& tracking) {
#if defined(SOL_SAFE_GETTER) && SOL_SAFE_GETTER
			return stack_detail::tagged_get(types<std::conditional_t<std::is_pointer<T>::value, detail::as_pointer_tag<std::remove_pointer_t<T>>, detail::as_value_tag<T>>>(), L, index, tracking);
#else
			return stack_detail::unchecked_get<std::conditional_t<std::is_pointer<T>::value, detail::as_pointer_tag<std::remove_pointer_t<T>>, detail::as_value_tag<T>>>(L, index, tracking);
#endif
		}

		template <typename T>
		inline decltype(auto) get_usertype(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return get_usertype<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_get(lua_State* L, int index, record& tracking) {
			return stack_detail::tagged_unqualified_get(types<T>(), L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return unqualified_get<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) get(lua_State* L, int index, record& tracking) {
			return stack_detail::tagged_get(types<T>(), L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return get<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) pop(lua_State* L) {
			return popper<meta::unqualified_t<T>>{}.pop(L);
		}

		template <bool global = false, bool raw = false, typename Key>
		void get_field(lua_State* L, Key&& key) {
			field_getter<meta::unqualified_t<Key>, global, raw>{}.get(L, std::forward<Key>(key));
		}

		template <bool global = false, bool raw = false, typename Key>
		void get_field(lua_State* L, Key&& key, int tableindex) {
			field_getter<meta::unqualified_t<Key>, global, raw>{}.get(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, typename Key>
		void raw_get_field(lua_State* L, Key&& key) {
			get_field<global, true>(L, std::forward<Key>(key));
		}

		template <bool global = false, typename Key>
		void raw_get_field(lua_State* L, Key&& key, int tableindex) {
			get_field<global, true>(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, bool raw = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_get_field(lua_State* L, Key&& key) {
			return probe_field_getter<meta::unqualified_t<Key>, C, global, raw>{}.get(L, std::forward<Key>(key));
		}

		template <bool global = false, bool raw = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_get_field(lua_State* L, Key&& key, int tableindex) {
			return probe_field_getter<meta::unqualified_t<Key>, C, global, raw>{}.get(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_raw_get_field(lua_State* L, Key&& key) {
			return probe_get_field<global, true, C>(L, std::forward<Key>(key));
		}

		template <bool global = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_raw_get_field(lua_State* L, Key&& key, int tableindex) {
			return probe_get_field<global, true, C>(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, bool raw = false, typename Key, typename Value>
		void set_field(lua_State* L, Key&& key, Value&& value) {
			field_setter<meta::unqualified_t<Key>, global, raw>{}.set(L, std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global = false, bool raw = false, typename Key, typename Value>
		void set_field(lua_State* L, Key&& key, Value&& value, int tableindex) {
			field_setter<meta::unqualified_t<Key>, global, raw>{}.set(L, std::forward<Key>(key), std::forward<Value>(value), tableindex);
		}

		template <bool global = false, typename Key, typename Value>
		void raw_set_field(lua_State* L, Key&& key, Value&& value) {
			set_field<global, true>(L, std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global = false, typename Key, typename Value>
		void raw_set_field(lua_State* L, Key&& key, Value&& value, int tableindex) {
			set_field<global, true>(L, std::forward<Key>(key), std::forward<Value>(value), tableindex);
		}

		template <typename T, typename F>
		inline void modify_unique_usertype_as(const stack_reference& obj, F&& f) {
			typedef unique_usertype_traits<T> u_traits;
			void* raw = lua_touserdata(obj.lua_state(), obj.stack_index());
			void* ptr_memory = detail::align_usertype_pointer(raw);
			void* uu_memory = detail::align_usertype_unique<T>(raw);
			T& uu = *static_cast<T*>(uu_memory);
			f(uu);
			*static_cast<void**>(ptr_memory) = static_cast<void*>(u_traits::get(uu));
		}

		template <typename F>
		inline void modify_unique_usertype(const stack_reference& obj, F&& f) {
			typedef meta::bind_traits<meta::unqualified_t<F>> bt;
			typedef typename bt::template arg_at<0> T;
			modify_unique_usertype_as<meta::unqualified_t<T>>(obj, std::forward<F>(f));
		}
	} // namespace stack
} // namespace sol

// end of sol/stack_core.hpp

// beginning of sol/stack_check.hpp

// beginning of sol/stack_check_unqualified.hpp

#include <cmath>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
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

// end of sol/stack_check_unqualified.hpp

// beginning of sol/stack_check_qualified.hpp

namespace sol {
namespace stack {

#if 0
	template <typename X>
	struct qualified_checker<X, type::userdata, std::enable_if_t<is_unique_usertype<X>::value && !std::is_reference<X>::value>> {
		typedef unique_usertype_traits<meta::unqualified_t<X>> u_traits;
		typedef typename u_traits::type T;
		
		template <typename Handler>
		static bool check(std::false_type, lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack::unqualified_check<X>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename Handler>
		static bool check(std::true_type, lua_State* L, int index, Handler&& handler, record& tracking) {
			// we have a unique pointer type that can be 
			// rebound to a base/derived type
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
			void* basecastdata = lua_touserdata(L, index);
			void* memory = detail::align_usertype_unique_destructor(basecastdata);
			detail::unique_destructor& pdx = *static_cast<detail::unique_destructor*>(memory);
			if (&detail::usertype_unique_alloc_destroy<T, X> == pdx) {
				return true;
			}
			if (detail::has_derived<T>::value) {
				memory = detail::align_usertype_unique_cast<true>(memory);
				detail::inheritance_unique_cast_function ic = reinterpret_cast<detail::inheritance_unique_cast_function>(memory);
				string_view ti = usertype_traits<T>::qualified_name();
				string_view rebind_ti = usertype_traits<base_id>::qualified_name();
				if (ic(nullptr, basecastdata, ti, rebind_ti)) {
					lua_pop(L, 1);
				}
			}
			handler(L, index, type::userdata, indextype, "value is a userdata but is not the correct unique usertype");
			return false;
		}
		
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return check(meta::neg<std::is_void<typename u_traits::base_id>>(), L, index, std::forward<Handler>(handler), tracking);
		}
	};

#endif // Not implemented right now...

	template <typename X>
	struct qualified_checker<X, type::userdata, std::enable_if_t<is_container<meta::unqualified_t<X>>::value && !std::is_reference<X>::value>> {
		template <typename Handler>
		static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			if (type_of(L, index) == type::userdata) {
				return stack::unqualified_check<X>(L, index, std::forward<Handler>(handler), tracking);
			}
			else {
				return stack::unqualified_check<nested<X>>(L, index, std::forward<Handler>(handler), tracking);
			}
		}
	};
}
} // namespace sol::stack

// end of sol/stack_check_qualified.hpp

// end of sol/stack_check.hpp

// beginning of sol/stack_get.hpp

// beginning of sol/stack_get_unqualified.hpp

// beginning of sol/overload.hpp

namespace sol {
	template <typename... Functions>
	struct overload_set {
		std::tuple<Functions...> functions;
		template <typename Arg, typename... Args, meta::disable<std::is_same<overload_set, meta::unqualified_t<Arg>>> = meta::enabler>
		overload_set(Arg&& arg, Args&&... args)
		: functions(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}
		overload_set(const overload_set&) = default;
		overload_set(overload_set&&) = default;
		overload_set& operator=(const overload_set&) = default;
		overload_set& operator=(overload_set&&) = default;
	};

	template <typename... Args>
	decltype(auto) overload(Args&&... args) {
		return overload_set<std::decay_t<Args>...>(std::forward<Args>(args)...);
	}
} // namespace sol

// end of sol/overload.hpp

// beginning of sol/unicode.hpp

namespace sol {
	// Everything here was lifted pretty much straight out of
	// ogonek, because fuck figuring it out=
	namespace unicode {
		enum class error_code {
			ok = 0,
			invalid_code_point,
			invalid_code_unit,
			invalid_leading_surrogate,
			invalid_trailing_surrogate,
			sequence_too_short,
			overlong_sequence,
		};

		inline const string_view& to_string(error_code ec) {
			static const string_view arr[4] = {
				"ok",
				"invalid code points",
				"invalid code unit",
				"overlong sequence"
			};
			return arr[static_cast<std::size_t>(ec)];
		}

		template <typename It>
		struct decoded_result {
			error_code error;
			char32_t codepoint;
			It next;
		};

		template <typename C>
		struct encoded_result {
			error_code error;
			std::size_t code_units_size;
			std::array<C, 4> code_units;
		};

		struct unicode_detail {
			// codepoint related
			static constexpr char32_t last_code_point = 0x10FFFF;

			static constexpr char32_t first_lead_surrogate = 0xD800;
			static constexpr char32_t last_lead_surrogate = 0xDBFF;

			static constexpr char32_t first_trail_surrogate = 0xDC00;
			static constexpr char32_t last_trail_surrogate = 0xDFFF;

			static constexpr char32_t first_surrogate = first_lead_surrogate;
			static constexpr char32_t last_surrogate = last_trail_surrogate;

			static constexpr bool is_lead_surrogate(char32_t u) {
				return u >= first_lead_surrogate && u <= last_lead_surrogate;
			}
			static constexpr bool is_trail_surrogate(char32_t u) {
				return u >= first_trail_surrogate && u <= last_trail_surrogate;
			}
			static constexpr bool is_surrogate(char32_t u) {
				return u >= first_surrogate && u <= last_surrogate;
			}

			// utf8 related
			static constexpr auto last_1byte_value = 0x7Fu;
			static constexpr auto last_2byte_value = 0x7FFu;
			static constexpr auto last_3byte_value = 0xFFFFu;

			static constexpr auto start_2byte_mask = 0x80u;
			static constexpr auto start_3byte_mask = 0xE0u;
			static constexpr auto start_4byte_mask = 0xF0u;

			static constexpr auto continuation_mask = 0xC0u;
			static constexpr auto continuation_signature = 0x80u;

			static constexpr int sequence_length(unsigned char b) {
				return (b & start_2byte_mask) == 0 ? 1
					: (b & start_3byte_mask) != start_3byte_mask ? 2
					: (b & start_4byte_mask) != start_4byte_mask ? 3
					: 4;
			}

			static constexpr char32_t decode(unsigned char b0, unsigned char b1) {
				return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
			}
			static constexpr char32_t decode(unsigned char b0, unsigned char b1, unsigned char b2) {
				return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
			}
			static constexpr char32_t decode(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3) {
				return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			}

			// utf16 related
			static constexpr char32_t last_bmp_value = 0xFFFF;
			static constexpr char32_t normalizing_value = 0x10000;
			static constexpr int lead_surrogate_bitmask = 0xFFC00;
			static constexpr int trail_surrogate_bitmask = 0x3FF;
			static constexpr int lead_shifted_bits = 10;
			static constexpr char32_t replacement = 0xFFFD;

			static char32_t combine_surrogates(char16_t lead, char16_t trail) {
				auto hi = lead - first_lead_surrogate;
				auto lo = trail - first_trail_surrogate;
				return normalizing_value + ((hi << lead_shifted_bits) | lo);
			}
		};

		inline encoded_result<char> code_point_to_utf8(char32_t codepoint) {
			encoded_result<char> er;
			er.error = error_code::ok;
			if (codepoint <= unicode_detail::last_1byte_value) {
				er.code_units_size = 1;
				er.code_units = std::array<char, 4>{ { static_cast<char>(codepoint) } };
			}
			else if (codepoint <= unicode_detail::last_2byte_value) {
				er.code_units_size = 2;
				er.code_units = std::array<char, 4>{{
					static_cast<char>(0xC0 | ((codepoint & 0x7C0) >> 6)),
					static_cast<char>(0x80 | (codepoint & 0x3F)),
				}};
			}
			else if (codepoint <= unicode_detail::last_3byte_value) {
				er.code_units_size = 3;
				er.code_units = std::array<char, 4>{{
					static_cast<char>(0xE0 | ((codepoint & 0xF000) >> 12)),
					static_cast<char>(0x80 | ((codepoint & 0xFC0) >> 6)),
					static_cast<char>(0x80 | (codepoint & 0x3F)),
				}};
			}
			else {
				er.code_units_size = 4;
				er.code_units = std::array<char, 4>{ {
					static_cast<char>(0xF0 | ((codepoint & 0x1C0000) >> 18)),
						static_cast<char>(0x80 | ((codepoint & 0x3F000) >> 12)),
						static_cast<char>(0x80 | ((codepoint & 0xFC0) >> 6)),
						static_cast<char>(0x80 | (codepoint & 0x3F)),
				} };
			}
			return er;
		}

		inline encoded_result<char16_t> code_point_to_utf16(char32_t codepoint) {
			encoded_result<char16_t> er;

			if (codepoint <= unicode_detail::last_bmp_value) {
				er.code_units_size = 1;
				er.code_units = std::array<char16_t, 4>{ { static_cast<char16_t>(codepoint) } };
				er.error = error_code::ok;
			}
			else {
				auto normal = codepoint - unicode_detail::normalizing_value;
				auto lead = unicode_detail::first_lead_surrogate + ((normal & unicode_detail::lead_surrogate_bitmask) >> unicode_detail::lead_shifted_bits);
				auto trail = unicode_detail::first_trail_surrogate + (normal & unicode_detail::trail_surrogate_bitmask);
				er.code_units = std::array<char16_t, 4>{ {
					static_cast<char16_t>(lead),
					static_cast<char16_t>(trail)
				} };
				er.code_units_size = 2;
				er.error = error_code::ok;
			}
			return er;
		}

		inline encoded_result<char32_t> code_point_to_utf32(char32_t codepoint) {
			encoded_result<char32_t> er;
			er.code_units_size = 1;
			er.code_units[0] = codepoint;
			er.error = error_code::ok;
			return er;
		}

		template <typename It>
		inline decoded_result<It> utf8_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}

			unsigned char b0 = *it;
			std::size_t length = unicode_detail::sequence_length(b0);

			if (length == 1) {
				dr.codepoint = static_cast<char32_t>(b0);
				dr.error = error_code::ok;
				++it;
				dr.next = it;
				return dr;
			}

			auto is_invalid = [](unsigned char b) { return b == 0xC0 || b == 0xC1 || b > 0xF4; };
			auto is_continuation = [](unsigned char b) {
				return (b & unicode_detail::continuation_mask) == unicode_detail::continuation_signature;
			};

			if (is_invalid(b0) || is_continuation(b0)) {
				dr.error = error_code::invalid_code_unit;
				dr.next = it;
				return dr;
			}

			++it;
			std::array<unsigned char, 4> b;
			b[0] = b0;
			for (std::size_t i = 1; i < length; ++i) {
				b[i] = *it;
				if (!is_continuation(b[i])) {
					dr.error = error_code::invalid_code_unit;
					dr.next = it;
					return dr;
				}
				++it;
			}

			char32_t decoded;
			switch (length) {
			case 2:
				decoded = unicode_detail::decode(b[0], b[1]);
				break;
			case 3:
				decoded = unicode_detail::decode(b[0], b[1], b[2]);
				break;
			default:
				decoded = unicode_detail::decode(b[0], b[1], b[2], b[3]);
				break;
			}

			auto is_overlong = [](char32_t u, std::size_t bytes) {
				return u <= unicode_detail::last_1byte_value
					|| (u <= unicode_detail::last_2byte_value && bytes > 2)
					|| (u <= unicode_detail::last_3byte_value && bytes > 3);
			};
			if (is_overlong(decoded, length)) {
				dr.error = error_code::overlong_sequence;
				return dr;
			}
			if (unicode_detail::is_surrogate(decoded) || decoded > unicode_detail::last_code_point) {
				dr.error = error_code::invalid_code_point;
				return dr;
			}
			
			// then everything is fine
			dr.codepoint = decoded;
			dr.error = error_code::ok;
			dr.next = it;
			return dr;
		}

		template <typename It>
		inline decoded_result<It> utf16_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}

			char16_t lead = static_cast<char16_t>(*it);
			
			if (!unicode_detail::is_surrogate(lead)) {
				++it;
				dr.codepoint = static_cast<char32_t>(lead);
				dr.next = it;
				dr.error = error_code::ok;
				return dr;
			}
			if (!unicode_detail::is_lead_surrogate(lead)) {
				dr.error = error_code::invalid_leading_surrogate;
				dr.next = it;
				return dr;
			}

			++it;
			auto trail = *it;
			if (!unicode_detail::is_trail_surrogate(trail)) {
				dr.error = error_code::invalid_trailing_surrogate;
				dr.next = it;
				return dr;
			}
			
			dr.codepoint = unicode_detail::combine_surrogates(lead, trail);
			dr.next = ++it;
			dr.error = error_code::ok;
			return dr;
		}

		template <typename It>
		inline decoded_result<It> utf32_to_code_point(It it, It last) {
			decoded_result<It> dr;
			if (it == last) {
				dr.next = it;
				dr.error = error_code::sequence_too_short;
				return dr;
			}
			dr.codepoint = static_cast<char32_t>(*it);
			dr.next = ++it;
			dr.error = error_code::ok;
			return dr;
		}
	}
}
// end of sol/unicode.hpp

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
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

// end of sol/stack_get_unqualified.hpp

// beginning of sol/stack_get_qualified.hpp

namespace sol {
namespace stack {

#if 0 // need static reflection / DERIVED_CLASS macros...
	template <typename X>
	struct qualified_getter<X, std::enable_if_t<
		!std::is_reference<X>::value && is_unique_usertype<meta::unqualified_t<X>>::value
	>> {
		typedef typename unique_usertype_traits<meta::unqualified_t<X>>::type P;
		typedef typename unique_usertype_traits<meta::unqualified_t<X>>::actual_type Real;

		static Real& get(lua_State* L, int index, record& tracking) {
			tracking.use(1);
			void* memory = lua_touserdata(L, index);
			void* del = detail::align_usertype_unique_destructor(memory);
			memory = detail::align_usertype_unique<Real>(memory);
			Real* mem = static_cast<Real*>(memory);
			return *mem;
		}
	};
#endif // need static reflection

	template <typename T>
	struct qualified_getter<T, std::enable_if_t<
		!std::is_reference<T>::value 
		&& is_container<meta::unqualified_t<T>>::value 
		&& std::is_default_constructible<meta::unqualified_t<T>>::value
		&& !is_lua_primitive<T>::value 
		&& !is_transparent_argument<T>::value 
	>> {
		static T get(lua_State* L, int index, record& tracking) {
			if (type_of(L, index) == type::userdata) {
				return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
			}
			else {
				return stack_detail::unchecked_unqualified_get<sol::nested<T>>(L, index, tracking);
			}
		}
	};
}
} // namespace sol::stack

// end of sol/stack_get_qualified.hpp

// end of sol/stack_get.hpp

// beginning of sol/stack_check_get.hpp

// beginning of sol/stack_check_get_unqualified.hpp

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#endif // C++17

namespace sol {
namespace stack {
	template <typename T, typename>
	struct check_getter {
		typedef decltype(stack_detail::unchecked_unqualified_get<T>(nullptr, 0, std::declval<record&>())) R;

		template <typename Handler>
		static optional<R> get(lua_State* L, int index, Handler&& handler, record& tracking) {
			if (!unqualified_check<T>(L, index, std::forward<Handler>(handler))) {
				tracking.use(static_cast<int>(!lua_isnone(L, index)));
				return nullopt;
			}
			return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
		}
	};

	template <typename T>
	struct check_getter<T, std::enable_if_t<is_lua_reference<T>::value>> {
		template <typename Handler>
		static optional<T> get(lua_State* L, int index, Handler&& handler, record& tracking) {
			// actually check if it's none here, otherwise
			// we'll have a none object inside an optional!
			bool success = lua_isnoneornil(L, index) == 0 && stack::check<T>(L, index, no_panic);
			if (!success) {
				// expected type, actual type
				tracking.use(static_cast<int>(success));
				handler(L, index, type::poly, type_of(L, index), "");
				return nullopt;
			}
			return stack_detail::unchecked_get<T>(L, index, tracking);
		}
	};

	template <typename T>
	struct check_getter<T, std::enable_if_t<std::is_integral<T>::value && lua_type_of<T>::value == type::number>> {
		template <typename Handler>
		static optional<T> get(lua_State* L, int index, Handler&& handler, record& tracking) {
#if SOL_LUA_VERSION >= 503
			if (lua_isinteger(L, index) != 0) {
				tracking.use(1);
				return static_cast<T>(lua_tointeger(L, index));
			}
#endif
			int isnum = 0;
			const lua_Number value = lua_tonumberx(L, index, &isnum);
			if (isnum != 0) {
#if (defined(SOL_SAFE_NUMERICS) && SOL_SAFE_NUMERICS) && !(defined(SOL_NO_CHECK_NUMBER_PRECISION) && SOL_NO_CHECK_NUMBER_PRECISION)
				const auto integer_value = llround(value);
				if (static_cast<lua_Number>(integer_value) == value) {
					tracking.use(1);
					return static_cast<T>(integer_value);
				}
#else
				tracking.use(1);
				return static_cast<T>(value);
#endif
			}
			const type t = type_of(L, index);
			tracking.use(static_cast<int>(t != type::none));
			handler(L, index, type::number, t, "not an integer");
			return nullopt;
		}
	};

	template <typename T>
	struct check_getter<T, std::enable_if_t<std::is_enum<T>::value && !meta::any_same<T, meta_function, type>::value>> {
		template <typename Handler>
		static optional<T> get(lua_State* L, int index, Handler&& handler, record& tracking) {
			int isnum = 0;
			lua_Integer value = lua_tointegerx(L, index, &isnum);
			if (isnum == 0) {
				type t = type_of(L, index);
				tracking.use(static_cast<int>(t != type::none));
				handler(L, index, type::number, t, "not a valid enumeration value");
				return nullopt;
			}
			tracking.use(1);
			return static_cast<T>(value);
		}
	};

	template <typename T>
	struct check_getter<T, std::enable_if_t<std::is_floating_point<T>::value>> {
		template <typename Handler>
		static optional<T> get(lua_State* L, int index, Handler&& handler, record& tracking) {
			int isnum = 0;
			lua_Number value = lua_tonumberx(L, index, &isnum);
			if (isnum == 0) {
				type t = type_of(L, index);
				tracking.use(static_cast<int>(t != type::none));
				handler(L, index, type::number, t, "not a valid floating point number");
				return nullopt;
			}
			tracking.use(1);
			return static_cast<T>(value);
		}
	};

	template <typename T>
	struct getter<optional<T>> {
		static decltype(auto) get(lua_State* L, int index, record& tracking) {
			return check_get<T>(L, index, no_panic, tracking);
		}
	};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	template <typename T>
	struct getter<std::optional<T>> {
		static std::optional<T> get(lua_State* L, int index, record& tracking) {
			if (!unqualified_check<T>(L, index, no_panic)) {
				tracking.use(static_cast<int>(!lua_isnone(L, index)));
				return std::nullopt;
			}
			return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
		}
	};

#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
	template <typename... Tn>
	struct check_getter<std::variant<Tn...>> {
		typedef std::variant<Tn...> V;
		typedef std::variant_size<V> V_size;
		typedef std::integral_constant<bool, V_size::value == 0> V_is_empty;

		template <typename Handler>
		static optional<V> get_empty(std::true_type, lua_State*, int, Handler&&, record&) {
			return nullopt;
		}

		template <typename Handler>
		static optional<V> get_empty(std::false_type, lua_State* L, int index, Handler&& handler, record&) {
			// This should never be reached...
			// please check your code and understand what you did to bring yourself here
			// maybe file a bug report, or 5
			handler(L, index, type::poly, type_of(L, index), "this variant code should never be reached: if it has, you have done something so terribly wrong");
			return nullopt;
		}

		template <typename Handler>
		static optional<V> get_one(std::integral_constant<std::size_t, 0>, lua_State* L, int index, Handler&& handler, record& tracking) {
			return get_empty(V_is_empty(), L, index, std::forward<Handler>(handler), tracking);
		}

		template <std::size_t I, typename Handler>
		static optional<V> get_one(std::integral_constant<std::size_t, I>, lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef std::variant_alternative_t<I - 1, V> T;
			if (stack::check<T>(L, index, no_panic, tracking)) {
				return V(std::in_place_index<I - 1>, stack::get<T>(L, index));
			}
			return get_one(std::integral_constant<std::size_t, I - 1>(), L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename Handler>
		static optional<V> get(lua_State* L, int index, Handler&& handler, record& tracking) {
			return get_one(std::integral_constant<std::size_t, V_size::value>(), L, index, std::forward<Handler>(handler), tracking);
		}
	};
#endif // SOL_STD_VARIANT
#endif // SOL_CXX17_FEATURES
}
} // namespace sol::stack

// end of sol/stack_check_get_unqualified.hpp

// beginning of sol/stack_check_get_qualified.hpp

namespace sol {
namespace stack {
	template <typename T, typename C>
	struct qualified_check_getter : check_getter<meta::unqualified_t<T>, C> {};
}
} // namespace sol::stack

// end of sol/stack_check_get_qualified.hpp

// end of sol/stack_check_get.hpp

// beginning of sol/stack_push.hpp

#include <limits>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
#endif // Can use variant
#endif // C++17

namespace sol {
namespace stack {
	inline int push_environment_of(lua_State* L, int index = -1) {
#if SOL_LUA_VERSION < 502
		// Use lua_getfenv
		lua_getfenv(L, index);
		return 1;
#else
		// Use upvalues as explained in Lua 5.2 and beyond's manual
		if (lua_getupvalue(L, index, 1) == nullptr) {
			push(L, lua_nil);
			return 1;
		}
#endif
		return 1;
	}

	template <typename T>
	int push_environment_of(const T& target) {
		target.push();
		return push_environment_of(target.lua_state(), -1) + 1;
	}

	template <typename T>
	struct pusher<detail::as_value_tag<T>> {
		template <typename F, typename... Args>
		static int push_fx(lua_State* L, F&& f, Args&&... args) {
			// Basically, we store all user-data like this:
			// If it's a movable/copyable value (no std::ref(x)), then we store the pointer to the new
			// data in the first sizeof(T*) bytes, and then however many bytes it takes to
			// do the actual object. Things that are std::ref or plain T* are stored as
			// just the sizeof(T*), and nothing else.
			T* obj = detail::usertype_allocate<T>(L);
			std::allocator<T> alloc{};
			std::allocator_traits<std::allocator<T>>::construct(alloc, obj, std::forward<Args>(args)...);
			f();
			return 1;
		}

		template <typename K, typename... Args>
		static int push_keyed(lua_State* L, K&& k, Args&&... args) {
			stack_detail::undefined_metatable<T> fx(L, &k[0]);
			return push_fx(L, fx, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static int push(lua_State* L, Args&&... args) {
			return push_keyed(L, usertype_traits<T>::metatable(), std::forward<Args>(args)...);
		}
	};

	template <typename T>
	struct pusher<detail::as_pointer_tag<T>> {
		typedef meta::unqualified_t<T> U;

		template <typename F>
		static int push_fx(lua_State* L, F&& f, T* obj) {
			if (obj == nullptr)
				return stack::push(L, lua_nil);
			T** pref = detail::usertype_allocate_pointer<T>(L);
			*pref = obj;
			f();
			return 1;
		}

		template <typename K>
		static int push_keyed(lua_State* L, K&& k, T* obj) {
			stack_detail::undefined_metatable<U*> fx(L, &k[0]);
			return push_fx(L, fx, obj);
		}

		static int push(lua_State* L, T* obj) {
			return push_keyed(L, usertype_traits<U*>::metatable(), obj);
		}
	};

	template <>
	struct pusher<detail::as_reference_tag> {
		template <typename T>
		static int push(lua_State* L, T&& obj) {
			return stack::push(L, detail::ptr(obj));
		}
	};

	template <typename T, typename>
	struct pusher {
		template <typename... Args>
		static int push(lua_State* L, Args&&... args) {
			return pusher<detail::as_value_tag<T>>{}.push(L, std::forward<Args>(args)...);
		}
	};

	template <typename T>
	struct pusher<T*, meta::disable_if_t<meta::any<is_container<meta::unqualified_t<T>>, std::is_function<meta::unqualified_t<T>>, is_lua_reference<meta::unqualified_t<T>>>::value>> {
		template <typename... Args>
		static int push(lua_State* L, Args&&... args) {
			return pusher<detail::as_pointer_tag<T>>{}.push(L, std::forward<Args>(args)...);
		}
	};

	template <typename T>
	struct pusher<T, std::enable_if_t<is_unique_usertype<T>::value>> {
		typedef typename unique_usertype_traits<T>::type P;
		typedef typename unique_usertype_traits<T>::actual_type Real;

		template <typename Arg, meta::enable<std::is_base_of<Real, meta::unqualified_t<Arg>>> = meta::enabler>
		static int push(lua_State* L, Arg&& arg) {
			if (unique_usertype_traits<T>::is_null(arg)) {
				return stack::push(L, lua_nil);
			}
			return push_deep(L, std::forward<Arg>(arg));
		}

		template <typename Arg0, typename Arg1, typename... Args>
		static int push(lua_State* L, Arg0&& arg0, Arg0&& arg1, Args&&... args) {
			return push_deep(L, std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...);
		}

		template <typename... Args>
		static int push_deep(lua_State* L, Args&&... args) {
			P** pref = nullptr;
			detail::unique_destructor* fx = nullptr;
			detail::unique_tag* id = nullptr;
			Real* mem = detail::usertype_unique_allocate<P, Real>(L, pref, fx, id);
			*fx = detail::usertype_unique_alloc_destroy<P, Real>;
#if 0
			*id = &detail::inheritance<P>::type_unique_cast_bases<Real>;
#else
			*id = &usertype_traits<Real>::qualified_name()[0];
#endif
			detail::default_construct::construct(mem, std::forward<Args>(args)...);
			*pref = unique_usertype_traits<T>::get(*mem);
			if (luaL_newmetatable(L, &usertype_traits<detail::unique_usertype<std::remove_cv_t<P>>>::metatable()[0]) == 1) {
				luaL_Reg l[32]{};
				int index = 0;
				auto prop_fx = [](meta_function) { return true; };
				usertype_detail::insert_default_registrations<P>(l, index, prop_fx);
				usertype_detail::make_destructor<T>(l, index);
				luaL_setfuncs(L, l, 0);
			}
			lua_setmetatable(L, -2);
			return 1;
		}
	};

	template <typename T>
	struct pusher<std::reference_wrapper<T>> {
		static int push(lua_State* L, const std::reference_wrapper<T>& t) {
			return stack::push(L, std::addressof(detail::deref(t.get())));
		}
	};

	template <typename T>
	struct pusher<T, std::enable_if_t<std::is_floating_point<T>::value>> {
		static int push(lua_State* L, const T& value) {
			lua_pushnumber(L, value);
			return 1;
		}
	};

	template <typename T>
	struct pusher<T, std::enable_if_t<std::is_integral<T>::value>> {
		static int push(lua_State* L, const T& value) {
#if SOL_LUA_VERSION >= 503
			static auto integer_value_fits = [](T const& value) {
				if (sizeof(T) < sizeof(lua_Integer) || (std::is_signed<T>::value && sizeof(T) == sizeof(lua_Integer))) {
					return true;
				}
				auto u_min = static_cast<std::intmax_t>((std::numeric_limits<lua_Integer>::min)());
				auto u_max = static_cast<std::uintmax_t>((std::numeric_limits<lua_Integer>::max)());
				auto t_min = static_cast<std::intmax_t>((std::numeric_limits<T>::min)());
				auto t_max = static_cast<std::uintmax_t>((std::numeric_limits<T>::max)());
				return (u_min <= t_min || value >= static_cast<T>(u_min)) && (u_max >= t_max || value <= static_cast<T>(u_max));
			};
			if (integer_value_fits(value)) {
				lua_pushinteger(L, static_cast<lua_Integer>(value));
				return 1;
			}
#endif // Lua 5.3 and above
#if (defined(SOL_SAFE_NUMERICS) && SOL_SAFE_NUMERICS) && !(defined(SOL_NO_CHECK_NUMBER_PRECISION) && SOL_NO_CHECK_NUMBER_PRECISION)
			if (static_cast<T>(llround(static_cast<lua_Number>(value))) != value) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
				// Is this really worth it?
				assert(false && "integer value will be misrepresented in lua");
				lua_pushnumber(L, static_cast<lua_Number>(value));
				return 1;
#else
				throw error(detail::direct_error, "integer value will be misrepresented in lua");
#endif // No Exceptions
			}
#endif // Safe Numerics and Number Precision Check
			lua_pushnumber(L, static_cast<lua_Number>(value));
			return 1;
		}
	};

	template <typename T>
	struct pusher<T, std::enable_if_t<std::is_enum<T>::value>> {
		static int push(lua_State* L, const T& value) {
			if (std::is_same<char, std::underlying_type_t<T>>::value) {
				return stack::push(L, static_cast<int>(value));
			}
			return stack::push(L, static_cast<std::underlying_type_t<T>>(value));
		}
	};

	template <typename T>
	struct pusher<detail::as_table_tag<T>> {
		static int push(lua_State* L, const T& tablecont) {
			typedef meta::has_key_value_pair<meta::unqualified_t<std::remove_pointer_t<T>>> has_kvp;
			return push(has_kvp(), std::false_type(), L, tablecont);
		}

		static int push(std::true_type, lua_State* L, const T& tablecont) {
			typedef meta::has_key_value_pair<meta::unqualified_t<std::remove_pointer_t<T>>> has_kvp;
			return push(has_kvp(), std::true_type(), L, tablecont);
		}

		static int push(std::false_type, lua_State* L, const T& tablecont) {
			typedef meta::has_key_value_pair<meta::unqualified_t<std::remove_pointer_t<T>>> has_kvp;
			return push(has_kvp(), std::false_type(), L, tablecont);
		}

		template <bool is_nested>
		static int push(std::true_type, std::integral_constant<bool, is_nested>, lua_State* L, const T& tablecont) {
			auto& cont = detail::deref(detail::unwrap(tablecont));
			lua_createtable(L, static_cast<int>(cont.size()), 0);
			int tableindex = lua_gettop(L);
			for (const auto& pair : cont) {
				if (is_nested) {
					set_field(L, pair.first, as_nested_ref(pair.second), tableindex);
				}
				else {
					set_field(L, pair.first, pair.second, tableindex);
				}
			}
			return 1;
		}

		template <bool is_nested>
		static int push(std::false_type, std::integral_constant<bool, is_nested>, lua_State* L, const T& tablecont) {
			auto& cont = detail::deref(detail::unwrap(tablecont));
			lua_createtable(L, stack_detail::get_size_hint(cont), 0);
			int tableindex = lua_gettop(L);
			std::size_t index = 1;
			for (const auto& i : cont) {
#if SOL_LUA_VERSION >= 503
				int p = is_nested ? stack::push(L, as_nested_ref(i)) : stack::push(L, i);
				for (int pi = 0; pi < p; ++pi) {
					lua_seti(L, tableindex, static_cast<lua_Integer>(index++));
				}
#else
				lua_pushinteger(L, static_cast<lua_Integer>(index));
				int p = is_nested ? stack::push(L, as_nested_ref(i)) : stack::push(L, i);
				if (p == 1) {
					++index;
					lua_settable(L, tableindex);
				}
				else {
					int firstindex = tableindex + 1 + 1;
					for (int pi = 0; pi < p; ++pi) {
						stack::push(L, index);
						lua_pushvalue(L, firstindex);
						lua_settable(L, tableindex);
						++index;
						++firstindex;
					}
					lua_pop(L, 1 + p);
				}
#endif // Lua Version 5.3 and others
			}
			// TODO: figure out a better way to do this...?
			//set_field(L, -1, cont.size());
			return 1;
		}
	};

	template <typename T>
	struct pusher<as_table_t<T>, std::enable_if_t<is_container<std::remove_pointer_t<meta::unwrap_unqualified_t<T>>>::value>> {
		static int push(lua_State* L, const T& tablecont) {
			return stack::push<detail::as_table_tag<T>>(L, tablecont);
		}
	};

	template <typename T>
	struct pusher<as_table_t<T>, std::enable_if_t<!is_container<std::remove_pointer_t<meta::unwrap_unqualified_t<T>>>::value>> {
		static int push(lua_State* L, const T& v) {
			return stack::push(L, v);
		}
	};

	template <typename T>
	struct pusher<nested<T>, std::enable_if_t<is_container<std::remove_pointer_t<meta::unwrap_unqualified_t<T>>>::value>> {
		static int push(lua_State* L, const T& tablecont) {
			pusher<detail::as_table_tag<T>> p{};
			// silence annoying VC++ warning
			(void)p;
			return p.push(std::true_type(), L, tablecont);
		}
	};

	template <typename T>
	struct pusher<nested<T>, std::enable_if_t<!is_container<std::remove_pointer_t<meta::unwrap_unqualified_t<T>>>::value>> {
		static int push(lua_State* L, const T& tablecont) {
			pusher<meta::unqualified_t<T>> p{};
			// silence annoying VC++ warning
			(void)p;
			return p.push(L, tablecont);
		}
	};

	template <typename T>
	struct pusher<std::initializer_list<T>> {
		static int push(lua_State* L, const std::initializer_list<T>& il) {
			pusher<detail::as_table_tag<std::initializer_list<T>>> p{};
			// silence annoying VC++ warning
			(void)p;
			return p.push(L, il);
		}
	};

	template <typename T>
	struct pusher<T, std::enable_if_t<is_lua_reference<T>::value>> {
		static int push(lua_State* L, const T& ref) {
			return ref.push(L);
		}

		static int push(lua_State* L, T&& ref) {
			return ref.push(L);
		}
	};

	template <>
	struct pusher<bool> {
		static int push(lua_State* L, bool b) {
			lua_pushboolean(L, b);
			return 1;
		}
	};

	template <>
	struct pusher<lua_nil_t> {
		static int push(lua_State* L, lua_nil_t) {
			lua_pushnil(L);
			return 1;
		}
	};

	template <>
	struct pusher<stack_count> {
		static int push(lua_State*, stack_count st) {
			return st.count;
		}
	};

	template <>
	struct pusher<metatable_t> {
		static int push(lua_State* L, metatable_t) {
			lua_pushlstring(L, "__mt", 4);
			return 1;
		}
	};

	template <>
	struct pusher<std::remove_pointer_t<lua_CFunction>> {
		static int push(lua_State* L, lua_CFunction func, int n = 0) {
			lua_pushcclosure(L, func, n);
			return 1;
		}
	};

	template <>
	struct pusher<lua_CFunction> {
		static int push(lua_State* L, lua_CFunction func, int n = 0) {
			lua_pushcclosure(L, func, n);
			return 1;
		}
	};

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
	template <>
	struct pusher<std::remove_pointer_t<detail::lua_CFunction_noexcept>> {
		static int push(lua_State* L, detail::lua_CFunction_noexcept func, int n = 0) {
			lua_pushcclosure(L, func, n);
			return 1;
		}
	};

	template <>
	struct pusher<detail::lua_CFunction_noexcept> {
		static int push(lua_State* L, detail::lua_CFunction_noexcept func, int n = 0) {
			lua_pushcclosure(L, func, n);
			return 1;
		}
	};
#endif // noexcept function type

	template <>
	struct pusher<c_closure> {
		static int push(lua_State* L, c_closure cc) {
			lua_pushcclosure(L, cc.c_function, cc.upvalues);
			return 1;
		}
	};

	template <typename Arg, typename... Args>
	struct pusher<closure<Arg, Args...>> {
		template <std::size_t... I, typename T>
		static int push(std::index_sequence<I...>, lua_State* L, T&& c) {
			int pushcount = multi_push(L, detail::forward_get<I>(c.upvalues)...);
			return stack::push(L, c_closure(c.c_function, pushcount));
		}

		template <typename T>
		static int push(lua_State* L, T&& c) {
			return push(std::make_index_sequence<1 + sizeof...(Args)>(), L, std::forward<T>(c));
		}
	};

	template <>
	struct pusher<void*> {
		static int push(lua_State* L, void* userdata) {
			lua_pushlightuserdata(L, userdata);
			return 1;
		}
	};

	template <>
	struct pusher<const void*> {
		static int push(lua_State* L, const void* userdata) {
			lua_pushlightuserdata(L, const_cast<void*>(userdata));
			return 1;
		}
	};

	template <>
	struct pusher<lightuserdata_value> {
		static int push(lua_State* L, lightuserdata_value userdata) {
			lua_pushlightuserdata(L, userdata);
			return 1;
		}
	};

	template <typename T>
	struct pusher<light<T>> {
		static int push(lua_State* L, light<T> l) {
			lua_pushlightuserdata(L, static_cast<void*>(l.value));
			return 1;
		}
	};

	template <typename T>
	struct pusher<user<T>> {
		template <bool with_meta = true, typename Key, typename... Args>
		static int push_with(lua_State* L, Key&& name, Args&&... args) {
			// A dumb pusher
			T* data = detail::user_allocate<T>(L);
			std::allocator<T> alloc{};
			std::allocator_traits<std::allocator<T>>::construct(alloc, data, std::forward<Args>(args)...);
			if (with_meta) {
				// Make sure we have a plain GC set for this data
				if (luaL_newmetatable(L, name) != 0) {
					lua_CFunction cdel = detail::user_alloc_destruct<T>;
					lua_pushcclosure(L, cdel, 0);
					lua_setfield(L, -2, "__gc");
				}
				lua_setmetatable(L, -2);
			}
			return 1;
		}

		template <typename Arg, typename... Args, meta::disable<meta::any_same<meta::unqualified_t<Arg>, no_metatable_t, metatable_t>> = meta::enabler>
		static int push(lua_State* L, Arg&& arg, Args&&... args) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with(L, name, std::forward<Arg>(arg), std::forward<Args>(args)...);
		}

		template <typename... Args>
		static int push(lua_State* L, no_metatable_t, Args&&... args) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with<false>(L, name, std::forward<Args>(args)...);
		}

		template <typename Key, typename... Args>
		static int push(lua_State* L, metatable_t, Key&& key, Args&&... args) {
			const auto name = &key[0];
			return push_with<true>(L, name, std::forward<Args>(args)...);
		}

		static int push(lua_State* L, const user<T>& u) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with(L, name, u.value);
		}

		static int push(lua_State* L, user<T>&& u) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with(L, name, std::move(u.value));
		}

		static int push(lua_State* L, no_metatable_t, const user<T>& u) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with<false>(L, name, u.value);
		}

		static int push(lua_State* L, no_metatable_t, user<T>&& u) {
			const auto name = &usertype_traits<meta::unqualified_t<T>>::user_gc_metatable()[0];
			return push_with<false>(L, name, std::move(u.value));
		}
	};

	template <>
	struct pusher<userdata_value> {
		static int push(lua_State* L, userdata_value data) {
			void** ud = detail::usertype_allocate_pointer<void>(L);
			*ud = data.value;
			return 1;
		}
	};

	template <>
	struct pusher<const char*> {
		static int push_sized(lua_State* L, const char* str, std::size_t len) {
			lua_pushlstring(L, str, len);
			return 1;
		}

		static int push(lua_State* L, const char* str) {
			if (str == nullptr)
				return stack::push(L, lua_nil);
			return push_sized(L, str, std::char_traits<char>::length(str));
		}

		static int push(lua_State* L, const char* strb, const char* stre) {
			return push_sized(L, strb, stre - strb);
		}

		static int push(lua_State* L, const char* str, std::size_t len) {
			return push_sized(L, str, len);
		}
	};

	template <>
	struct pusher<char*> {
		static int push_sized(lua_State* L, const char* str, std::size_t len) {
			pusher<const char*> p{};
			(void)p;
			return p.push_sized(L, str, len);
		}

		static int push(lua_State* L, const char* str) {
			pusher<const char*> p{};
			(void)p;
			return p.push(L, str);
		}

		static int push(lua_State* L, const char* strb, const char* stre) {
			pusher<const char*> p{};
			(void)p;
			return p.push(L, strb, stre);
		}

		static int push(lua_State* L, const char* str, std::size_t len) {
			pusher<const char*> p{};
			(void)p;
			return p.push(L, str, len);
		}
	};

	template <size_t N>
	struct pusher<char[N]> {
		static int push(lua_State* L, const char (&str)[N]) {
			lua_pushlstring(L, str, std::char_traits<char>::length(str));
			return 1;
		}

		static int push(lua_State* L, const char (&str)[N], std::size_t sz) {
			lua_pushlstring(L, str, sz);
			return 1;
		}
	};

	template <>
	struct pusher<char> {
		static int push(lua_State* L, char c) {
			const char str[2] = { c, '\0' };
			return stack::push(L, str, 1);
		}
	};

	template <typename Traits, typename Al>
	struct pusher<std::basic_string<char, Traits, Al>> {
		static int push(lua_State* L, const std::basic_string<char, Traits, Al>& str) {
			lua_pushlstring(L, str.c_str(), str.size());
			return 1;
		}

		static int push(lua_State* L, const std::basic_string<char, Traits, Al>& str, std::size_t sz) {
			lua_pushlstring(L, str.c_str(), sz);
			return 1;
		}
	};

	template <typename Ch, typename Traits>
	struct pusher<basic_string_view<Ch, Traits>> {
		static int push(lua_State* L, const basic_string_view<Ch, Traits>& sv) {
			return stack::push(L, sv.data(), sv.length());
		}

		static int push(lua_State* L, const basic_string_view<Ch, Traits>& sv, std::size_t n) {
			return stack::push(L, sv.data(), n);
		}
	};

	template <>
	struct pusher<meta_function> {
		static int push(lua_State* L, meta_function m) {
			const std::string& str = to_string(m);
			lua_pushlstring(L, str.c_str(), str.size());
			return 1;
		}
	};

	template <>
	struct pusher<absolute_index> {
		static int push(lua_State* L, absolute_index ai) {
			lua_pushvalue(L, ai);
			return 1;
		}
	};

	template <>
	struct pusher<raw_index> {
		static int push(lua_State* L, raw_index ri) {
			lua_pushvalue(L, ri);
			return 1;
		}
	};

	template <>
	struct pusher<ref_index> {
		static int push(lua_State* L, ref_index ri) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ri);
			return 1;
		}
	};

	template <>
	struct pusher<const wchar_t*> {
		static int push(lua_State* L, const wchar_t* wstr) {
			return push(L, wstr, std::char_traits<wchar_t>::length(wstr));
		}

		static int push(lua_State* L, const wchar_t* wstr, std::size_t sz) {
			return push(L, wstr, wstr + sz);
		}

		static int push(lua_State* L, const wchar_t* strb, const wchar_t* stre) {
			if (sizeof(wchar_t) == 2) {
				const char16_t* sb = reinterpret_cast<const char16_t*>(strb);
				const char16_t* se = reinterpret_cast<const char16_t*>(stre);
				return stack::push(L, sb, se);
			}
			const char32_t* sb = reinterpret_cast<const char32_t*>(strb);
			const char32_t* se = reinterpret_cast<const char32_t*>(stre);
			return stack::push(L, sb, se);
		}
	};

	template <>
	struct pusher<wchar_t*> {
		static int push(lua_State* L, const wchar_t* str) {
			pusher<const wchar_t*> p{};
			(void)p;
			return p.push(L, str);
		}

		static int push(lua_State* L, const wchar_t* strb, const wchar_t* stre) {
			pusher<const wchar_t*> p{};
			(void)p;
			return p.push(L, strb, stre);
		}

		static int push(lua_State* L, const wchar_t* str, std::size_t len) {
			pusher<const wchar_t*> p{};
			(void)p;
			return p.push(L, str, len);
		}
	};

	template <>
	struct pusher<const char16_t*> {
		static int convert_into(lua_State* L, char* start, std::size_t, const char16_t* strb, const char16_t* stre) {
			char* target = start;
			char32_t cp = 0;
			for (const char16_t* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf16_to_code_point(strtarget, stre);
				if (dr.error != unicode::error_code::ok) {
					cp = unicode::unicode_detail::replacement;
				}
				else {
					cp = dr.codepoint;
				}
				auto er = unicode::code_point_to_utf8(cp);
				const char* utf8data = er.code_units.data();
				std::memcpy(target, utf8data, er.code_units_size);
				target += er.code_units_size;
				strtarget = dr.next;
			}

			return stack::push(L, start, target);
		}

		static int push(lua_State* L, const char16_t* u16str) {
			return push(L, u16str, std::char_traits<char16_t>::length(u16str));
		}

		static int push(lua_State* L, const char16_t* u16str, std::size_t sz) {
			return push(L, u16str, u16str + sz);
		}

		static int push(lua_State* L, const char16_t* strb, const char16_t* stre) {
			// TODO: use new unicode methods
			// TODO: use new unicode methods
			char sbo[SOL_STACK_STRING_OPTIMIZATION_SIZE];
			// if our max string space is small enough, use SBO
			// right off the bat
			std::size_t max_possible_code_units = (stre - strb) * 4;
			if (max_possible_code_units <= SOL_STACK_STRING_OPTIMIZATION_SIZE) {
				return convert_into(L, sbo, max_possible_code_units, strb, stre);
			}
			// otherwise, we must manually count/check size
			std::size_t needed_size = 0;
			for (const char16_t* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf16_to_code_point(strtarget, stre);
				auto er = unicode::code_point_to_utf8(dr.codepoint);
				needed_size += er.code_units_size;
				strtarget = dr.next;
			}
			if (needed_size < SOL_STACK_STRING_OPTIMIZATION_SIZE) {
				return convert_into(L, sbo, needed_size, strb, stre);
			}
			std::string u8str("", 0);
			u8str.resize(needed_size);
			char* target = &u8str[0];
			return convert_into(L, target, needed_size, strb, stre);
		}
	};

	template <>
	struct pusher<char16_t*> {
		static int push(lua_State* L, const char16_t* str) {
			pusher<const char16_t*> p{};
			(void)p;
			return p.push(L, str);
		}

		static int push(lua_State* L, const char16_t* strb, const char16_t* stre) {
			pusher<const char16_t*> p{};
			(void)p;
			return p.push(L, strb, stre);
		}

		static int push(lua_State* L, const char16_t* str, std::size_t len) {
			pusher<const char16_t*> p{};
			(void)p;
			return p.push(L, str, len);
		}
	};

	template <>
	struct pusher<const char32_t*> {
		static int convert_into(lua_State* L, char* start, std::size_t, const char32_t* strb, const char32_t* stre) {
			char* target = start;
			char32_t cp = 0;
			for (const char32_t* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf32_to_code_point(strtarget, stre);
				if (dr.error != unicode::error_code::ok) {
					cp = unicode::unicode_detail::replacement;
				}
				else {
					cp = dr.codepoint;
				}
				auto er = unicode::code_point_to_utf8(cp);
				const char* data = er.code_units.data();
				std::memcpy(target, data, er.code_units_size);
				target += er.code_units_size;
				strtarget = dr.next;
			}
			return stack::push(L, start, target);
		}

		static int push(lua_State* L, const char32_t* u32str) {
			return push(L, u32str, u32str + std::char_traits<char32_t>::length(u32str));
		}

		static int push(lua_State* L, const char32_t* u32str, std::size_t sz) {
			return push(L, u32str, u32str + sz);
		}

		static int push(lua_State* L, const char32_t* strb, const char32_t* stre) {
			// TODO: use new unicode methods
			char sbo[SOL_STACK_STRING_OPTIMIZATION_SIZE];
			// if our max string space is small enough, use SBO
			// right off the bat
			std::size_t max_possible_code_units = (stre - strb) * 4;
			if (max_possible_code_units <= SOL_STACK_STRING_OPTIMIZATION_SIZE) {
				return convert_into(L, sbo, max_possible_code_units, strb, stre);
			}
			// otherwise, we must manually count/check size
			std::size_t needed_size = 0;
			for (const char32_t* strtarget = strb; strtarget < stre;) {
				auto dr = unicode::utf32_to_code_point(strtarget, stre);
				auto er = unicode::code_point_to_utf8(dr.codepoint);
				needed_size += er.code_units_size;
				strtarget = dr.next;
			}
			if (needed_size < SOL_STACK_STRING_OPTIMIZATION_SIZE) {
				return convert_into(L, sbo, needed_size, strb, stre);
			}
			std::string u8str("", 0);
			u8str.resize(needed_size);
			char* target = &u8str[0];
			return convert_into(L, target, needed_size, strb, stre);
		}
	};

	template <>
	struct pusher<char32_t*> {
		static int push(lua_State* L, const char32_t* str) {
			pusher<const char32_t*> p{};
			(void)p;
			return p.push(L, str);
		}

		static int push(lua_State* L, const char32_t* strb, const char32_t* stre) {
			pusher<const char32_t*> p{};
			(void)p;
			return p.push(L, strb, stre);
		}

		static int push(lua_State* L, const char32_t* str, std::size_t len) {
			pusher<const char32_t*> p{};
			(void)p;
			return p.push(L, str, len);
		}
	};

	template <size_t N>
	struct pusher<wchar_t[N]> {
		static int push(lua_State* L, const wchar_t (&str)[N]) {
			return push(L, str, std::char_traits<wchar_t>::length(str));
		}

		static int push(lua_State* L, const wchar_t (&str)[N], std::size_t sz) {
			return stack::push<const wchar_t*>(L, str, str + sz);
		}
	};

	template <size_t N>
	struct pusher<char16_t[N]> {
		static int push(lua_State* L, const char16_t (&str)[N]) {
			return push(L, str, std::char_traits<char16_t>::length(str));
		}

		static int push(lua_State* L, const char16_t (&str)[N], std::size_t sz) {
			return stack::push<const char16_t*>(L, str, str + sz);
		}
	};

	template <size_t N>
	struct pusher<char32_t[N]> {
		static int push(lua_State* L, const char32_t (&str)[N]) {
			return push(L, str, std::char_traits<char32_t>::length(str));
		}

		static int push(lua_State* L, const char32_t (&str)[N], std::size_t sz) {
			return stack::push<const char32_t*>(L, str, str + sz);
		}
	};

	template <>
	struct pusher<wchar_t> {
		static int push(lua_State* L, wchar_t c) {
			const wchar_t str[2] = { c, '\0' };
			return stack::push(L, &str[0], 1);
		}
	};

	template <>
	struct pusher<char16_t> {
		static int push(lua_State* L, char16_t c) {
			const char16_t str[2] = { c, '\0' };
			return stack::push(L, &str[0], 1);
		}
	};

	template <>
	struct pusher<char32_t> {
		static int push(lua_State* L, char32_t c) {
			const char32_t str[2] = { c, '\0' };
			return stack::push(L, &str[0], 1);
		}
	};

	template <typename Ch, typename Traits, typename Al>
	struct pusher<std::basic_string<Ch, Traits, Al>, std::enable_if_t<!std::is_same<Ch, char>::value>> {
		static int push(lua_State* L, const std::basic_string<Ch, Traits, Al>& wstr) {
			return push(L, wstr, wstr.size());
		}

		static int push(lua_State* L, const std::basic_string<Ch, Traits, Al>& wstr, std::size_t sz) {
			return stack::push(L, wstr.data(), wstr.data() + sz);
		}
	};

	template <typename... Args>
	struct pusher<std::tuple<Args...>> {
		template <std::size_t... I, typename T>
		static int push(std::index_sequence<I...>, lua_State* L, T&& t) {
			int pushcount = 0;
			(void)detail::swallow{ 0, (pushcount += stack::push(L, detail::forward_get<I>(t)), 0)... };
			return pushcount;
		}

		template <typename T>
		static int push(lua_State* L, T&& t) {
			return push(std::index_sequence_for<Args...>(), L, std::forward<T>(t));
		}
	};

	template <typename A, typename B>
	struct pusher<std::pair<A, B>> {
		template <typename T>
		static int push(lua_State* L, T&& t) {
			int pushcount = stack::push(L, detail::forward_get<0>(t));
			pushcount += stack::push(L, detail::forward_get<1>(t));
			return pushcount;
		}
	};

	template <typename O>
	struct pusher<optional<O>> {
		template <typename T>
		static int push(lua_State* L, T&& t) {
			if (t == nullopt) {
				return stack::push(L, nullopt);
			}
			return stack::push(L, static_cast<std::conditional_t<std::is_lvalue_reference<T>::value, O&, O&&>>(t.value()));
		}
	};

	template <>
	struct pusher<nullopt_t> {
		static int push(lua_State* L, nullopt_t) {
			return stack::push(L, lua_nil);
		}
	};

	template <>
	struct pusher<std::nullptr_t> {
		static int push(lua_State* L, std::nullptr_t) {
			return stack::push(L, lua_nil);
		}
	};

	template <>
	struct pusher<this_state> {
		static int push(lua_State*, const this_state&) {
			return 0;
		}
	};

	template <>
	struct pusher<this_main_state> {
		static int push(lua_State*, const this_main_state&) {
			return 0;
		}
	};

	template <>
	struct pusher<new_table> {
		static int push(lua_State* L, const new_table& nt) {
			lua_createtable(L, nt.sequence_hint, nt.map_hint);
			return 1;
		}
	};

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	template <typename O>
	struct pusher<std::optional<O>> {
		template <typename T>
		static int push(lua_State* L, T&& t) {
			if (t == std::nullopt) {
				return stack::push(L, nullopt);
			}
			return stack::push(L, static_cast<std::conditional_t<std::is_lvalue_reference<T>::value, O&, O&&>>(t.value()));
		}
	};

#if defined(SOL_STD_VARIANT) && SOL_STD_VARIANT
	namespace stack_detail {

		struct push_function {
			lua_State* L;

			push_function(lua_State* L)
			: L(L) {
			}

			template <typename T>
			int operator()(T&& value) const {
				return stack::push<T>(L, std::forward<T>(value));
			}
		};

	} // namespace stack_detail

	template <typename... Tn>
	struct pusher<std::variant<Tn...>> {
		static int push(lua_State* L, const std::variant<Tn...>& v) {
			return std::visit(stack_detail::push_function(L), v);
		}

		static int push(lua_State* L, std::variant<Tn...>&& v) {
			return std::visit(stack_detail::push_function(L), std::move(v));
		}
	};
#endif // Variant because Clang is terrible
#endif // C++17 Support
}
} // namespace sol::stack

// end of sol/stack_push.hpp

// beginning of sol/stack_pop.hpp

namespace sol {
namespace stack {
	template <typename T, typename>
	struct popper {
		inline static decltype(auto) pop(lua_State* L) {
			record tracking{};
#ifdef __INTEL_COMPILER
			auto&& r = get<T>(L, -lua_size<T>::value, tracking);
#else
			decltype(auto) r = get<T>(L, -lua_size<T>::value, tracking);
#endif
			lua_pop(L, tracking.used);
			return r;
		}
	};

	template <typename T>
	struct popper<T, std::enable_if_t<is_stack_based<meta::unqualified_t<T>>::value>> {
		static_assert(meta::neg<is_stack_based<meta::unqualified_t<T>>>::value, "You cannot pop something that lives solely on the stack: it will not remain on the stack when popped and thusly will go out of scope!");
	};
}
} // namespace sol::stack

// end of sol/stack_pop.hpp

// beginning of sol/stack_field.hpp

namespace sol {
namespace stack {
	template <typename T, bool, bool, typename>
	struct field_getter {
		template <typename Key>
		void get(lua_State* L, Key&& key, int tableindex = -2) {
			push(L, std::forward<Key>(key));
			lua_gettable(L, tableindex);
		}
	};

	template <typename T, bool global, typename C>
	struct field_getter<T, global, true, C> {
		template <typename Key>
		void get(lua_State* L, Key&& key, int tableindex = -2) {
			push(L, std::forward<Key>(key));
			lua_rawget(L, tableindex);
		}
	};

	template <bool b, bool raw, typename C>
	struct field_getter<metatable_t, b, raw, C> {
		void get(lua_State* L, metatable_t, int tableindex = -1) {
			if (lua_getmetatable(L, tableindex) == 0)
				push(L, lua_nil);
		}
	};

	template <bool b, bool raw, typename C>
	struct field_getter<env_t, b, raw, C> {
		void get(lua_State* L, env_t, int tableindex = -1) {
#if SOL_LUA_VERSION < 502
			// Use lua_setfenv
			lua_getfenv(L, tableindex);
#else
			// Use upvalues as explained in Lua 5.2 and beyond's manual
			if (lua_getupvalue(L, tableindex, 1) == nullptr) {
				push(L, lua_nil);
			}
#endif
		}
	};

	template <typename T, bool raw>
	struct field_getter<T, true, raw, std::enable_if_t<meta::is_c_str<T>::value>> {
		template <typename Key>
		void get(lua_State* L, Key&& key, int = -1) {
			lua_getglobal(L, &key[0]);
		}
	};

	template <typename T>
	struct field_getter<T, false, false, std::enable_if_t<meta::is_c_str<T>::value>> {
		template <typename Key>
		void get(lua_State* L, Key&& key, int tableindex = -1) {
			lua_getfield(L, tableindex, &key[0]);
		}
	};

#if SOL_LUA_VERSION >= 503
	template <typename T>
	struct field_getter<T, false, false, std::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value>> {
		template <typename Key>
		void get(lua_State* L, Key&& key, int tableindex = -1) {
			lua_geti(L, tableindex, static_cast<lua_Integer>(key));
		}
	};
#endif // Lua 5.3.x

#if SOL_LUA_VERSION >= 502
	template <typename C>
	struct field_getter<void*, false, true, C> {
		void get(lua_State* L, void* key, int tableindex = -1) {
			lua_rawgetp(L, tableindex, key);
		}
	};
#endif // Lua 5.3.x

	template <typename T>
	struct field_getter<T, false, true, std::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value>> {
		template <typename Key>
		void get(lua_State* L, Key&& key, int tableindex = -1) {
			lua_rawgeti(L, tableindex, static_cast<lua_Integer>(key));
		}
	};

	template <typename... Args, bool b, bool raw, typename C>
	struct field_getter<std::tuple<Args...>, b, raw, C> {
		template <std::size_t... I, typename Keys>
		void apply(std::index_sequence<0, I...>, lua_State* L, Keys&& keys, int tableindex) {
			get_field<b, raw>(L, detail::forward_get<0>(keys), tableindex);
			void(detail::swallow{(get_field<false, raw>(L, detail::forward_get<I>(keys)), 0)...});
			reference saved(L, -1);
			lua_pop(L, static_cast<int>(sizeof...(I)));
			saved.push();
		}

		template <typename Keys>
		void get(lua_State* L, Keys&& keys) {
			apply(std::make_index_sequence<sizeof...(Args)>(), L, std::forward<Keys>(keys), lua_absindex(L, -1));
		}

		template <typename Keys>
		void get(lua_State* L, Keys&& keys, int tableindex) {
			apply(std::make_index_sequence<sizeof...(Args)>(), L, std::forward<Keys>(keys), tableindex);
		}
	};

	template <typename A, typename B, bool b, bool raw, typename C>
	struct field_getter<std::pair<A, B>, b, raw, C> {
		template <typename Keys>
		void get(lua_State* L, Keys&& keys, int tableindex) {
			get_field<b, raw>(L, detail::forward_get<0>(keys), tableindex);
			get_field<false, raw>(L, detail::forward_get<1>(keys));
			reference saved(L, -1);
			lua_pop(L, static_cast<int>(2));
			saved.push();
		}

		template <typename Keys>
		void get(lua_State* L, Keys&& keys) {
			get_field<b, raw>(L, detail::forward_get<0>(keys));
			get_field<false, raw>(L, detail::forward_get<1>(keys));
			reference saved(L, -1);
			lua_pop(L, static_cast<int>(2));
			saved.push();
		}
	};

	template <typename T, bool, bool, typename>
	struct field_setter {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int tableindex = -3) {
			push(L, std::forward<Key>(key));
			push(L, std::forward<Value>(value));
			lua_settable(L, tableindex);
		}
	};

	template <typename T, bool b, typename C>
	struct field_setter<T, b, true, C> {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int tableindex = -3) {
			push(L, std::forward<Key>(key));
			push(L, std::forward<Value>(value));
			lua_rawset(L, tableindex);
		}
	};

	template <bool b, bool raw, typename C>
	struct field_setter<metatable_t, b, raw, C> {
		template <typename Value>
		void set(lua_State* L, metatable_t, Value&& value, int tableindex = -2) {
			push(L, std::forward<Value>(value));
			lua_setmetatable(L, tableindex);
		}
	};

	template <typename T, bool raw>
	struct field_setter<T, true, raw, std::enable_if_t<meta::is_c_str<T>::value>> {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int = -2) {
			push(L, std::forward<Value>(value));
			lua_setglobal(L, &key[0]);
		}
	};

	template <typename T>
	struct field_setter<T, false, false, std::enable_if_t<meta::is_c_str<T>::value>> {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int tableindex = -2) {
			push(L, std::forward<Value>(value));
			lua_setfield(L, tableindex, &key[0]);
		}
	};

#if SOL_LUA_VERSION >= 503
	template <typename T>
	struct field_setter<T, false, false, std::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value>> {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int tableindex = -2) {
			push(L, std::forward<Value>(value));
			lua_seti(L, tableindex, static_cast<lua_Integer>(key));
		}
	};
#endif // Lua 5.3.x

	template <typename T>
	struct field_setter<T, false, true, std::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value>> {
		template <typename Key, typename Value>
		void set(lua_State* L, Key&& key, Value&& value, int tableindex = -2) {
			push(L, std::forward<Value>(value));
			lua_rawseti(L, tableindex, static_cast<lua_Integer>(key));
		}
	};

#if SOL_LUA_VERSION >= 502
	template <typename C>
	struct field_setter<void*, false, true, C> {
		template <typename Key, typename Value>
		void set(lua_State* L, void* key, Value&& value, int tableindex = -2) {
			push(L, std::forward<Value>(value));
			lua_rawsetp(L, tableindex, key);
		}
	};
#endif // Lua 5.2.x

	template <typename... Args, bool b, bool raw, typename C>
	struct field_setter<std::tuple<Args...>, b, raw, C> {
		template <bool g, std::size_t I, typename Key, typename Value>
		void apply(std::index_sequence<I>, lua_State* L, Key&& keys, Value&& value, int tableindex) {
			I < 1 ? set_field<g, raw>(L, detail::forward_get<I>(keys), std::forward<Value>(value), tableindex) : set_field<g, raw>(L, detail::forward_get<I>(keys), std::forward<Value>(value));
		}

		template <bool g, std::size_t I0, std::size_t I1, std::size_t... I, typename Keys, typename Value>
		void apply(std::index_sequence<I0, I1, I...>, lua_State* L, Keys&& keys, Value&& value, int tableindex) {
			I0 < 1 ? get_field<g, raw>(L, detail::forward_get<I0>(keys), tableindex) : get_field<g, raw>(L, detail::forward_get<I0>(keys), -1);
			apply<false>(std::index_sequence<I1, I...>(), L, std::forward<Keys>(keys), std::forward<Value>(value), -1);
		}

		template <bool g, std::size_t I0, std::size_t... I, typename Keys, typename Value>
		void top_apply(std::index_sequence<I0, I...>, lua_State* L, Keys&& keys, Value&& value, int tableindex) {
			apply<g>(std::index_sequence<I0, I...>(), L, std::forward<Keys>(keys), std::forward<Value>(value), tableindex);
			lua_pop(L, static_cast<int>(sizeof...(I)));
		}

		template <typename Keys, typename Value>
		void set(lua_State* L, Keys&& keys, Value&& value, int tableindex = -3) {
			top_apply<b>(std::make_index_sequence<sizeof...(Args)>(), L, std::forward<Keys>(keys), std::forward<Value>(value), tableindex);
		}
	};

	template <typename A, typename B, bool b, bool raw, typename C>
	struct field_setter<std::pair<A, B>, b, raw, C> {
		template <typename Keys, typename Value>
		void set(lua_State* L, Keys&& keys, Value&& value, int tableindex = -1) {
			get_field<b, raw>(L, detail::forward_get<0>(keys), tableindex);
			set_field<false, raw>(L, detail::forward_get<1>(keys), std::forward<Value>(value));
			lua_pop(L, 1);
		}
	};
}
} // namespace sol::stack

// end of sol/stack_field.hpp

// beginning of sol/stack_probe.hpp

namespace sol {
namespace stack {
	template <typename T, typename P, bool b, bool raw, typename>
	struct probe_field_getter {
		template <typename Key>
		probe get(lua_State* L, Key&& key, int tableindex = -2) {
			if (!b && !maybe_indexable(L, tableindex)) {
				return probe(false, 0);
			}
			get_field<b, raw>(L, std::forward<Key>(key), tableindex);
			return probe(check<P>(L), 1);
		}
	};

	template <typename A, typename B, typename P, bool b, bool raw, typename C>
	struct probe_field_getter<std::pair<A, B>, P, b, raw, C> {
		template <typename Keys>
		probe get(lua_State* L, Keys&& keys, int tableindex = -2) {
			if (!b && !maybe_indexable(L, tableindex)) {
				return probe(false, 0);
			}
			get_field<b, raw>(L, std::get<0>(keys), tableindex);
			if (!maybe_indexable(L)) {
				return probe(false, 1);
			}
			get_field<false, raw>(L, std::get<1>(keys), tableindex);
			return probe(check<P>(L), 2);
		}
	};

	template <typename... Args, typename P, bool b, bool raw, typename C>
	struct probe_field_getter<std::tuple<Args...>, P, b, raw, C> {
		template <std::size_t I, typename Keys>
		probe apply(std::index_sequence<I>, int sofar, lua_State* L, Keys&& keys, int tableindex) {
			get_field < I<1 && b, raw>(L, std::get<I>(keys), tableindex);
			return probe(check<P>(L), sofar);
		}

		template <std::size_t I, std::size_t I1, std::size_t... In, typename Keys>
		probe apply(std::index_sequence<I, I1, In...>, int sofar, lua_State* L, Keys&& keys, int tableindex) {
			get_field < I<1 && b, raw>(L, std::get<I>(keys), tableindex);
			if (!maybe_indexable(L)) {
				return probe(false, sofar);
			}
			return apply(std::index_sequence<I1, In...>(), sofar + 1, L, std::forward<Keys>(keys), -1);
		}

		template <typename Keys>
		probe get(lua_State* L, Keys&& keys, int tableindex = -2) {
			if (!b && !maybe_indexable(L, tableindex)) {
				return probe(false, 0);
			}
			return apply(std::index_sequence_for<Args...>(), 1, L, std::forward<Keys>(keys), tableindex);
		}
	};
}
} // namespace sol::stack

// end of sol/stack_probe.hpp

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

// end of sol/stack.hpp

// beginning of sol/unsafe_function.hpp

// beginning of sol/function_result.hpp

// beginning of sol/protected_function_result.hpp

// beginning of sol/proxy_base.hpp

namespace sol {
	struct proxy_base_tag {};

	template <typename Super>
	struct proxy_base : proxy_base_tag {
		operator std::string() const {
			const Super& super = *static_cast<const Super*>(static_cast<const void*>(this));
			return super.template get<std::string>();
		}

		template <typename T, meta::enable<meta::neg<meta::is_string_constructible<T>>, is_proxy_primitive<meta::unqualified_t<T>>> = meta::enabler>
		operator T() const {
			const Super& super = *static_cast<const Super*>(static_cast<const void*>(this));
			return super.template get<T>();
		}

		template <typename T, meta::enable<meta::neg<meta::is_string_constructible<T>>, meta::neg<is_proxy_primitive<meta::unqualified_t<T>>>> = meta::enabler>
		operator T&() const {
			const Super& super = *static_cast<const Super*>(static_cast<const void*>(this));
			return super.template get<T&>();
		}

		lua_State* lua_state() const {
			const Super& super = *static_cast<const Super*>(static_cast<const void*>(this));
			return super.lua_state();
		}
	};
} // namespace sol

// end of sol/proxy_base.hpp

// beginning of sol/stack_iterator.hpp

namespace sol {
	template <typename proxy_t, bool is_const>
	struct stack_iterator {
		typedef std::conditional_t<is_const, const proxy_t, proxy_t> reference;
		typedef std::conditional_t<is_const, const proxy_t*, proxy_t*> pointer;
		typedef proxy_t value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::random_access_iterator_tag iterator_category;
		lua_State* L;
		int index;
		int stacktop;
		proxy_t sp;

		stack_iterator()
			: L(nullptr), index((std::numeric_limits<int>::max)()), stacktop((std::numeric_limits<int>::max)()), sp() {
		}
		stack_iterator(const stack_iterator<proxy_t, true>& r)
			: L(r.L), index(r.index), stacktop(r.stacktop), sp(r.sp) {
		}
		stack_iterator(lua_State* luastate, int idx, int topidx)
			: L(luastate), index(idx), stacktop(topidx), sp(luastate, idx) {
		}

		reference operator*() {
			return proxy_t(L, index);
		}

		reference operator*() const {
			return proxy_t(L, index);
		}

		pointer operator->() {
			sp = proxy_t(L, index);
			return &sp;
		}

		pointer operator->() const {
			const_cast<proxy_t&>(sp) = proxy_t(L, index);
			return &sp;
		}

		stack_iterator& operator++() {
			++index;
			return *this;
		}

		stack_iterator operator++(int) {
			auto r = *this;
			this->operator++();
			return r;
		}

		stack_iterator& operator--() {
			--index;
			return *this;
		}

		stack_iterator operator--(int) {
			auto r = *this;
			this->operator--();
			return r;
		}

		stack_iterator& operator+=(difference_type idx) {
			index += static_cast<int>(idx);
			return *this;
		}

		stack_iterator& operator-=(difference_type idx) {
			index -= static_cast<int>(idx);
			return *this;
		}

		difference_type operator-(const stack_iterator& r) const {
			return index - r.index;
		}

		stack_iterator operator+(difference_type idx) const {
			stack_iterator r = *this;
			r += idx;
			return r;
		}

		reference operator[](difference_type idx) const {
			return proxy_t(L, index + static_cast<int>(idx));
		}

		bool operator==(const stack_iterator& r) const {
			if (stacktop == (std::numeric_limits<int>::max)()) {
				return r.index == r.stacktop;
			}
			else if (r.stacktop == (std::numeric_limits<int>::max)()) {
				return index == stacktop;
			}
			return index == r.index;
		}

		bool operator!=(const stack_iterator& r) const {
			return !(this->operator==(r));
		}

		bool operator<(const stack_iterator& r) const {
			return index < r.index;
		}

		bool operator>(const stack_iterator& r) const {
			return index > r.index;
		}

		bool operator<=(const stack_iterator& r) const {
			return index <= r.index;
		}

		bool operator>=(const stack_iterator& r) const {
			return index >= r.index;
		}
	};

	template <typename proxy_t, bool is_const>
	inline stack_iterator<proxy_t, is_const> operator+(typename stack_iterator<proxy_t, is_const>::difference_type n, const stack_iterator<proxy_t, is_const>& r) {
		return r + n;
	}
} // namespace sol

// end of sol/stack_iterator.hpp

// beginning of sol/stack_proxy.hpp

// beginning of sol/stack_proxy_base.hpp

namespace sol {
	struct stack_proxy_base : public proxy_base<stack_proxy_base> {
	private:
		lua_State* L;
		int index;

	public:
		stack_proxy_base()
			: L(nullptr), index(0) {
		}
		stack_proxy_base(lua_State* L, int index)
			: L(L), index(index) {
		}

		template <typename T>
		decltype(auto) get() const {
			return stack::get<T>(L, stack_index());
		}

		template <typename T>
		bool is() const {
			return stack::check<T>(L, stack_index());
		}

		template <typename T>
		decltype(auto) as() const {
			return get<T>();
		}

		type get_type() const noexcept {
			return type_of(lua_state(), stack_index());
		}

		int push() const {
			return push(L);
		}

		int push(lua_State* Ls) const {
			lua_pushvalue(Ls, index);
			return 1;
		}

		lua_State* lua_state() const {
			return L;
		}
		int stack_index() const {
			return index;
		}
	};

	namespace stack {
		template <>
		struct getter<stack_proxy_base> {
			static stack_proxy_base get(lua_State* L, int index = -1) {
				return stack_proxy_base(L, index);
			}
		};

		template <>
		struct pusher<stack_proxy_base> {
			static int push(lua_State*, const stack_proxy_base& ref) {
				return ref.push();
			}
		};
	} // namespace stack

} // namespace sol

// end of sol/stack_proxy_base.hpp

namespace sol {
	struct stack_proxy : public stack_proxy_base {
	public:
		stack_proxy()
		: stack_proxy_base() {
		}
		stack_proxy(lua_State* L, int index)
		: stack_proxy_base(L, index) {
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args);

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}
	};

	namespace stack {
		template <>
		struct getter<stack_proxy> {
			static stack_proxy get(lua_State* L, int index = -1) {
				return stack_proxy(L, index);
			}
		};

		template <>
		struct pusher<stack_proxy> {
			static int push(lua_State*, const stack_proxy& ref) {
				return ref.push();
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/stack_proxy.hpp

namespace sol {
	struct protected_function_result : public proxy_base<protected_function_result> {
	private:
		lua_State* L;
		int index;
		int returncount;
		int popcount;
		call_status err;

		template <typename T>
		decltype(auto) tagged_get(types<optional<T>>, int index_offset) const {
			typedef decltype(stack::get<optional<T>>(L, index)) ret_t;
			int target = index + index_offset;
			if (!valid()) {
				return ret_t(nullopt);
			}
			return stack::get<optional<T>>(L, target);
		}

		template <typename T>
		decltype(auto) tagged_get(types<T>, int index_offset) const {
			int target = index + index_offset;
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES
			if (!valid()) {
				type t = type_of(L, target);
				type_panic_c_str(L, target, t, type::none, "bad get from protected_function_result (is not an error)");
			}
#endif // Check Argument Safety
			return stack::get<T>(L, target);
		}

		optional<error> tagged_get(types<optional<error>>, int index_offset) const {
			int target = index + index_offset;
			if (valid()) {
				return nullopt;
			}
			return error(detail::direct_error, stack::get<std::string>(L, target));
		}

		error tagged_get(types<error>, int index_offset) const {
			int target = index + index_offset;
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES
			if (valid()) {
				type t = type_of(L, target);
				type_panic_c_str(L, target, t, type::none, "bad get from protected_function_result (is an error)");
			}
#endif // Check Argument Safety
			return error(detail::direct_error, stack::get<std::string>(L, target));
		}

	public:
		typedef stack_proxy reference_type;
		typedef stack_proxy value_type;
		typedef stack_proxy* pointer;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef stack_iterator<stack_proxy, false> iterator;
		typedef stack_iterator<stack_proxy, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		protected_function_result() = default;
		protected_function_result(lua_State* Ls, int idx = -1, int retnum = 0, int popped = 0, call_status pferr = call_status::ok) noexcept
		: L(Ls), index(idx), returncount(retnum), popcount(popped), err(pferr) {
		}
		protected_function_result(const protected_function_result&) = default;
		protected_function_result& operator=(const protected_function_result&) = default;
		protected_function_result(protected_function_result&& o) noexcept
		: L(o.L), index(o.index), returncount(o.returncount), popcount(o.popcount), err(o.err) {
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.abandon();
		}
		protected_function_result& operator=(protected_function_result&& o) noexcept {
			L = o.L;
			index = o.index;
			returncount = o.returncount;
			popcount = o.popcount;
			err = o.err;
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.abandon();
			return *this;
		}

		protected_function_result(const unsafe_function_result& o) = delete;
		protected_function_result& operator=(const unsafe_function_result& o) = delete;
		protected_function_result(unsafe_function_result&& o) noexcept;
		protected_function_result& operator=(unsafe_function_result&& o) noexcept;

		call_status status() const noexcept {
			return err;
		}

		bool valid() const noexcept {
			return status() == call_status::ok || status() == call_status::yielded;
		}

		template <typename T>
		decltype(auto) get(int index_offset = 0) const {
			return tagged_get(types<meta::unqualified_t<T>>(), index_offset);
		}

		type get_type(difference_type index_offset = 0) const noexcept {
			return type_of(L, index + static_cast<int>(index_offset));
		}

		stack_proxy operator[](difference_type index_offset) const {
			return stack_proxy(L, index + static_cast<int>(index_offset));
		}

		iterator begin() {
			return iterator(L, index, stack_index() + return_count());
		}
		iterator end() {
			return iterator(L, stack_index() + return_count(), stack_index() + return_count());
		}
		const_iterator begin() const {
			return const_iterator(L, index, stack_index() + return_count());
		}
		const_iterator end() const {
			return const_iterator(L, stack_index() + return_count(), stack_index() + return_count());
		}
		const_iterator cbegin() const {
			return begin();
		}
		const_iterator cend() const {
			return end();
		}

		reverse_iterator rbegin() {
			return std::reverse_iterator<iterator>(begin());
		}
		reverse_iterator rend() {
			return std::reverse_iterator<iterator>(end());
		}
		const_reverse_iterator rbegin() const {
			return std::reverse_iterator<const_iterator>(begin());
		}
		const_reverse_iterator rend() const {
			return std::reverse_iterator<const_iterator>(end());
		}
		const_reverse_iterator crbegin() const {
			return std::reverse_iterator<const_iterator>(cbegin());
		}
		const_reverse_iterator crend() const {
			return std::reverse_iterator<const_iterator>(cend());
		}

		lua_State* lua_state() const noexcept {
			return L;
		};
		int stack_index() const noexcept {
			return index;
		};
		int return_count() const noexcept {
			return returncount;
		};
		int pop_count() const noexcept {
			return popcount;
		};
		void abandon() noexcept {
			//L = nullptr;
			index = 0;
			returncount = 0;
			popcount = 0;
			err = call_status::runtime;
		}
		~protected_function_result() {
			stack::remove(L, index, popcount);
		}
	};

	namespace stack {
		template <>
		struct pusher<protected_function_result> {
			static int push(lua_State* L, const protected_function_result& pfr) {
				int p = 0;
				for (int i = 0; i < pfr.pop_count(); ++i) {
					lua_pushvalue(L, i + pfr.stack_index());
					++p;
				}
				return p;
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/protected_function_result.hpp

// beginning of sol/unsafe_function_result.hpp

namespace sol {
	struct unsafe_function_result : public proxy_base<unsafe_function_result> {
	private:
		lua_State* L;
		int index;
		int returncount;

	public:
		typedef stack_proxy reference_type;
		typedef stack_proxy value_type;
		typedef stack_proxy* pointer;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef stack_iterator<stack_proxy, false> iterator;
		typedef stack_iterator<stack_proxy, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		unsafe_function_result() = default;
		unsafe_function_result(lua_State* Ls, int idx = -1, int retnum = 0)
			: L(Ls), index(idx), returncount(retnum) {
		}
		unsafe_function_result(const unsafe_function_result&) = default;
		unsafe_function_result& operator=(const unsafe_function_result&) = default;
		unsafe_function_result(unsafe_function_result&& o)
			: L(o.L), index(o.index), returncount(o.returncount) {
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but will be thorough
			o.abandon();
		}
		unsafe_function_result& operator=(unsafe_function_result&& o) {
			L = o.L;
			index = o.index;
			returncount = o.returncount;
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but will be thorough
			o.abandon();
			return *this;
		}

		unsafe_function_result(const protected_function_result& o) = delete;
		unsafe_function_result& operator=(const protected_function_result& o) = delete;
		unsafe_function_result(protected_function_result&& o) noexcept;
		unsafe_function_result& operator=(protected_function_result&& o) noexcept;

		template <typename T>
		decltype(auto) get(difference_type index_offset = 0) const {
			return stack::get<T>(L, index + static_cast<int>(index_offset));
		}

		type get_type(difference_type index_offset = 0) const noexcept {
			return type_of(L, index + static_cast<int>(index_offset));
		}

		stack_proxy operator[](difference_type index_offset) const {
			return stack_proxy(L, index + static_cast<int>(index_offset));
		}

		iterator begin() {
			return iterator(L, index, stack_index() + return_count());
		}
		iterator end() {
			return iterator(L, stack_index() + return_count(), stack_index() + return_count());
		}
		const_iterator begin() const {
			return const_iterator(L, index, stack_index() + return_count());
		}
		const_iterator end() const {
			return const_iterator(L, stack_index() + return_count(), stack_index() + return_count());
		}
		const_iterator cbegin() const {
			return begin();
		}
		const_iterator cend() const {
			return end();
		}

		reverse_iterator rbegin() {
			return std::reverse_iterator<iterator>(begin());
		}
		reverse_iterator rend() {
			return std::reverse_iterator<iterator>(end());
		}
		const_reverse_iterator rbegin() const {
			return std::reverse_iterator<const_iterator>(begin());
		}
		const_reverse_iterator rend() const {
			return std::reverse_iterator<const_iterator>(end());
		}
		const_reverse_iterator crbegin() const {
			return std::reverse_iterator<const_iterator>(cbegin());
		}
		const_reverse_iterator crend() const {
			return std::reverse_iterator<const_iterator>(cend());
		}

		call_status status() const noexcept {
			return call_status::ok;
		}

		bool valid() const noexcept {
			return status() == call_status::ok || status() == call_status::yielded;
		}

		lua_State* lua_state() const {
			return L;
		};
		int stack_index() const {
			return index;
		};
		int return_count() const {
			return returncount;
		};
		void abandon() noexcept {
			//L = nullptr;
			index = 0;
			returncount = 0;
		}
		~unsafe_function_result() {
			lua_pop(L, returncount);
		}
	};

	namespace stack {
		template <>
		struct pusher<unsafe_function_result> {
			static int push(lua_State* L, const unsafe_function_result& fr) {
				int p = 0;
				for (int i = 0; i < fr.return_count(); ++i) {
					lua_pushvalue(L, i + fr.stack_index());
					++p;
				}
				return p;
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/unsafe_function_result.hpp

namespace sol {

	namespace detail {
		template <>
		struct is_speshul<unsafe_function_result> : std::true_type {};
		template <>
		struct is_speshul<protected_function_result> : std::true_type {};

		template <std::size_t I, typename... Args, typename T>
		stack_proxy get(types<Args...>, index_value<0>, index_value<I>, const T& fr) {
			return stack_proxy(fr.lua_state(), static_cast<int>(fr.stack_index() + I));
		}

		template <std::size_t I, std::size_t N, typename Arg, typename... Args, typename T, meta::enable<meta::boolean<(N > 0)>> = meta::enabler>
		stack_proxy get(types<Arg, Args...>, index_value<N>, index_value<I>, const T& fr) {
			return get(types<Args...>(), index_value<N - 1>(), index_value<I + lua_size<Arg>::value>(), fr);
		}
	} // namespace detail

	template <>
	struct tie_size<unsafe_function_result> : std::integral_constant<std::size_t, SIZE_MAX> {};

	template <>
	struct tie_size<protected_function_result> : std::integral_constant<std::size_t, SIZE_MAX> {};

	template <std::size_t I>
	stack_proxy get(const unsafe_function_result& fr) {
		return stack_proxy(fr.lua_state(), static_cast<int>(fr.stack_index() + I));
	}

	template <std::size_t I, typename... Args>
	stack_proxy get(types<Args...> t, const unsafe_function_result& fr) {
		return detail::get(t, index_value<I>(), index_value<0>(), fr);
	}

	template <std::size_t I>
	stack_proxy get(const protected_function_result& fr) {
		return stack_proxy(fr.lua_state(), static_cast<int>(fr.stack_index() + I));
	}

	template <std::size_t I, typename... Args>
	stack_proxy get(types<Args...> t, const protected_function_result& fr) {
		return detail::get(t, index_value<I>(), index_value<0>(), fr);
	}
} // namespace sol

// end of sol/function_result.hpp

// beginning of sol/function_types.hpp

// beginning of sol/function_types_core.hpp

// beginning of sol/wrapper.hpp

namespace sol {

	namespace detail {
		template <typename T>
		using array_return_type = std::conditional_t<std::is_array<T>::value, std::add_lvalue_reference_t<T>, T>;
	}

	template <typename F, typename = void>
	struct wrapper {
		typedef lua_bind_traits<meta::unqualified_t<F>> traits_type;
		typedef typename traits_type::args_list args_list;
		typedef typename traits_type::args_list free_args_list;
		typedef typename traits_type::returns_list returns_list;

		template <typename... Args>
		static decltype(auto) call(F& f, Args&&... args) {
			return f(std::forward<Args>(args)...);
		}

		struct caller {
			template <typename... Args>
			decltype(auto) operator()(F& fx, Args&&... args) const {
				return call(fx, std::forward<Args>(args)...);
			}
		};
	};

	template <typename F>
	struct wrapper<F, std::enable_if_t<std::is_function<std::remove_pointer_t<meta::unqualified_t<F>>>::value>> {
		typedef lua_bind_traits<std::remove_pointer_t<meta::unqualified_t<F>>> traits_type;
		typedef typename traits_type::args_list args_list;
		typedef typename traits_type::args_list free_args_list;
		typedef typename traits_type::returns_list returns_list;

		template <F fx, typename... Args>
		static decltype(auto) invoke(Args&&... args) {
			return fx(std::forward<Args>(args)...);
		}

		template <typename... Args>
		static decltype(auto) call(F& fx, Args&&... args) {
			return fx(std::forward<Args>(args)...);
		}

		struct caller {
			template <typename... Args>
			decltype(auto) operator()(F& fx, Args&&... args) const {
				return call(fx, std::forward<Args>(args)...);
			}
		};

		template <F fx>
		struct invoker {
			template <typename... Args>
			decltype(auto) operator()(Args&&... args) const {
				return invoke<fx>(std::forward<Args>(args)...);
			}
		};
	};

	template <typename F>
	struct wrapper<F, std::enable_if_t<std::is_member_object_pointer<meta::unqualified_t<F>>::value>> {
		typedef lua_bind_traits<meta::unqualified_t<F>> traits_type;
		typedef typename traits_type::object_type object_type;
		typedef typename traits_type::return_type return_type;
		typedef typename traits_type::args_list args_list;
		typedef types<object_type&, return_type> free_args_list;
		typedef typename traits_type::returns_list returns_list;

		template <F fx>
		static auto call(object_type& mem) -> detail::array_return_type<decltype(mem.*fx)> {
			return mem.*fx;
		}

		template <F fx, typename Arg, typename... Args>
		static decltype(auto) invoke(object_type& mem, Arg&& arg, Args&&...) {
			return mem.*fx = std::forward<Arg>(arg);
		}

		template <typename Fx>
		static auto call(Fx&& fx, object_type& mem) -> detail::array_return_type<decltype(mem.*fx)> {
			return mem.*fx;
		}

		template <typename Fx, typename Arg, typename... Args>
		static void call(Fx&& fx, object_type& mem, Arg&& arg, Args&&...) {
			(mem.*fx) = std::forward<Arg>(arg);
		}

		struct caller {
			template <typename Fx, typename... Args>
			decltype(auto) operator()(Fx&& fx, object_type& mem, Args&&... args) const {
				return call(std::forward<Fx>(fx), mem, std::forward<Args>(args)...);
			}
		};

		template <F fx>
		struct invoker {
			template <typename... Args>
			decltype(auto) operator()(Args&&... args) const {
				return invoke<fx>(std::forward<Args>(args)...);
			}
		};
	};

	template <typename F, typename R, typename O, typename... FArgs>
	struct member_function_wrapper {
		typedef O object_type;
		typedef lua_bind_traits<F> traits_type;
		typedef typename traits_type::args_list args_list;
		typedef types<object_type&, FArgs...> free_args_list;
		typedef meta::tuple_types<R> returns_list;

		template <F fx, typename... Args>
		static R invoke(O& mem, Args&&... args) {
			return (mem.*fx)(std::forward<Args>(args)...);
		}

		template <typename Fx, typename... Args>
		static R call(Fx&& fx, O& mem, Args&&... args) {
			return (mem.*fx)(std::forward<Args>(args)...);
		}

		struct caller {
			template <typename Fx, typename... Args>
			decltype(auto) operator()(Fx&& fx, O& mem, Args&&... args) const {
				return call(std::forward<Fx>(fx), mem, std::forward<Args>(args)...);
			}
		};

		template <F fx>
		struct invoker {
			template <typename... Args>
			decltype(auto) operator()(O& mem, Args&&... args) const {
				return invoke<fx>(mem, std::forward<Args>(args)...);
			}
		};
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...)> : public member_function_wrapper<R (O::*)(Args...), R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const> : public member_function_wrapper<R (O::*)(Args...) const, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile> : public member_function_wrapper<R (O::*)(Args...) const volatile, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...)&> : public member_function_wrapper<R (O::*)(Args...)&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const&> : public member_function_wrapper<R (O::*)(Args...) const&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile&> : public member_function_wrapper<R (O::*)(Args...) const volatile&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...)&> : public member_function_wrapper<R (O::*)(Args..., ...)&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const&> : public member_function_wrapper<R (O::*)(Args..., ...) const&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const volatile&> : public member_function_wrapper<R (O::*)(Args..., ...) const volatile&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) &&> : public member_function_wrapper<R (O::*)(Args...)&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const&&> : public member_function_wrapper<R (O::*)(Args...) const&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile&&> : public member_function_wrapper<R (O::*)(Args...) const volatile&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) &&> : public member_function_wrapper<R (O::*)(Args..., ...)&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const&&> : public member_function_wrapper<R (O::*)(Args..., ...) const&, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const volatile&&> : public member_function_wrapper<R (O::*)(Args..., ...) const volatile&, R, O, Args...> {
	};

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
	//noexcept has become a part of a function's type

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) noexcept> : public member_function_wrapper<R (O::*)(Args...) noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const noexcept> : public member_function_wrapper<R (O::*)(Args...) const noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile noexcept> : public member_function_wrapper<R (O::*)(Args...) const volatile noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) & noexcept> : public member_function_wrapper<R (O::*)(Args...) & noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const& noexcept> : public member_function_wrapper<R (O::*)(Args...) const& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile& noexcept> : public member_function_wrapper<R (O::*)(Args...) const volatile& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) & noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) & noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const& noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) const& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const volatile& noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) const volatile& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) && noexcept> : public member_function_wrapper<R (O::*)(Args...) & noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const&& noexcept> : public member_function_wrapper<R (O::*)(Args...) const& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args...) const volatile&& noexcept> : public member_function_wrapper<R (O::*)(Args...) const volatile& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) && noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) & noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const&& noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) const& noexcept, R, O, Args...> {
	};

	template <typename R, typename O, typename... Args>
	struct wrapper<R (O::*)(Args..., ...) const volatile&& noexcept> : public member_function_wrapper<R (O::*)(Args..., ...) const volatile& noexcept, R, O, Args...> {
	};

#endif // noexcept is part of a function's type

} // namespace sol

// end of sol/wrapper.hpp

namespace sol {
namespace function_detail {
	template <typename Fx, int start = 1, bool is_yielding = false>
	inline int call(lua_State* L) {
		Fx& fx = stack::get<user<Fx>>(L, upvalue_index(start));
		int nr = fx(L);
		if (is_yielding) {
			return lua_yield(L, nr);
		}
		else {
			return nr;
		}
	}
}
} // namespace sol::function_detail

// end of sol/function_types_core.hpp

// beginning of sol/function_types_templated.hpp

// beginning of sol/call.hpp

// beginning of sol/protect.hpp

namespace sol {

	template <typename T>
	struct protect_t {
		T value;

		template <typename Arg, typename... Args, meta::disable<std::is_same<protect_t, meta::unqualified_t<Arg>>> = meta::enabler>
		protect_t(Arg&& arg, Args&&... args)
		: value(std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}

		protect_t(const protect_t&) = default;
		protect_t(protect_t&&) = default;
		protect_t& operator=(const protect_t&) = default;
		protect_t& operator=(protect_t&&) = default;
	};

	template <typename T>
	auto protect(T&& value) {
		return protect_t<std::decay_t<T>>(std::forward<T>(value));
	}

} // namespace sol

// end of sol/protect.hpp

// beginning of sol/property.hpp

namespace sol {

	struct no_prop {};

	template <typename R, typename W>
	struct property_wrapper {
		typedef std::integral_constant<bool, !std::is_void<R>::value> can_read;
		typedef std::integral_constant<bool, !std::is_void<W>::value> can_write;
		typedef std::conditional_t<can_read::value, R, no_prop> Read;
		typedef std::conditional_t<can_write::value, W, no_prop> Write;
		Read read;
		Write write;

		template <typename Rx, typename Wx>
		property_wrapper(Rx&& r, Wx&& w)
		: read(std::forward<Rx>(r)), write(std::forward<Wx>(w)) {
		}
	};

	namespace property_detail {
		template <typename R, typename W>
		inline decltype(auto) property(std::true_type, R&& read, W&& write) {
			return property_wrapper<std::decay_t<R>, std::decay_t<W>>(std::forward<R>(read), std::forward<W>(write));
		}
		template <typename W, typename R>
		inline decltype(auto) property(std::false_type, W&& write, R&& read) {
			return property_wrapper<std::decay_t<R>, std::decay_t<W>>(std::forward<R>(read), std::forward<W>(write));
		}
		template <typename R>
		inline decltype(auto) property(std::true_type, R&& read) {
			return property_wrapper<std::decay_t<R>, void>(std::forward<R>(read), no_prop());
		}
		template <typename W>
		inline decltype(auto) property(std::false_type, W&& write) {
			return property_wrapper<void, std::decay_t<W>>(no_prop(), std::forward<W>(write));
		}
	} // namespace property_detail

	template <typename F, typename G>
	inline decltype(auto) property(F&& f, G&& g) {
		typedef lua_bind_traits<meta::unqualified_t<F>> left_traits;
		typedef lua_bind_traits<meta::unqualified_t<G>> right_traits;
		return property_detail::property(meta::boolean<(left_traits::free_arity < right_traits::free_arity)>(), std::forward<F>(f), std::forward<G>(g));
	}

	template <typename F>
	inline decltype(auto) property(F&& f) {
		typedef lua_bind_traits<meta::unqualified_t<F>> left_traits;
		return property_detail::property(meta::boolean<(left_traits::free_arity < 2)>(), std::forward<F>(f));
	}

	template <typename F>
	inline decltype(auto) readonly_property(F&& f) {
		return property_detail::property(std::true_type(), std::forward<F>(f));
	}

	template <typename F>
	inline decltype(auto) writeonly_property(F&& f) {
		return property_detail::property(std::false_type(), std::forward<F>(f));
	}

	template <typename T>
	struct readonly_wrapper {
		T v;

		readonly_wrapper(T v)
		: v(std::move(v)) {
		}

		operator T&() {
			return v;
		}
		operator const T&() const {
			return v;
		}
	};

	// Allow someone to make a member variable readonly (const)
	template <typename R, typename T>
	inline auto readonly(R T::*v) {
		return readonly_wrapper<meta::unqualified_t<decltype(v)>>(v);
	}

	template <typename T>
	struct var_wrapper {
		T value;
		template <typename... Args>
		var_wrapper(Args&&... args)
		: value(std::forward<Args>(args)...) {
		}
		var_wrapper(const var_wrapper&) = default;
		var_wrapper(var_wrapper&&) = default;
		var_wrapper& operator=(const var_wrapper&) = default;
		var_wrapper& operator=(var_wrapper&&) = default;
	};

	template <typename V>
	inline auto var(V&& v) {
		typedef meta::unqualified_t<V> T;
		return var_wrapper<T>(std::forward<V>(v));
	}

	namespace meta {
		template <typename T>
		struct is_member_object : std::is_member_object_pointer<T> {};

		template <typename T>
		struct is_member_object<readonly_wrapper<T>> : std::true_type {};
	} // namespace meta

} // namespace sol

// end of sol/property.hpp

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

// end of sol/call.hpp

namespace sol {
	namespace function_detail {
		template <typename F, F fx>
		inline int call_wrapper_variable(std::false_type, lua_State* L) {
			typedef meta::bind_traits<meta::unqualified_t<F>> traits_type;
			typedef typename traits_type::args_list args_list;
			typedef meta::tuple_types<typename traits_type::return_type> return_type;
			return stack::call_into_lua(return_type(), args_list(), L, 1, fx);
		}

		template <typename R, typename V, V, typename T>
		inline int call_set_assignable(std::false_type, T&&, lua_State* L) {
			return luaL_error(L, "cannot write to this type: copy assignment/constructor not available");
		}

		template <typename R, typename V, V variable, typename T>
		inline int call_set_assignable(std::true_type, lua_State* L, T&& mem) {
			(mem.*variable) = stack::get<R>(L, 2);
			return 0;
		}

		template <typename R, typename V, V, typename T>
		inline int call_set_variable(std::false_type, lua_State* L, T&&) {
			return luaL_error(L, "cannot write to a const variable");
		}

		template <typename R, typename V, V variable, typename T>
		inline int call_set_variable(std::true_type, lua_State* L, T&& mem) {
			return call_set_assignable<R, V, variable>(std::is_assignable<std::add_lvalue_reference_t<R>, R>(), L, std::forward<T>(mem));
		}

		template <typename V, V variable>
		inline int call_wrapper_variable(std::true_type, lua_State* L) {
			typedef meta::bind_traits<meta::unqualified_t<V>> traits_type;
			typedef typename traits_type::object_type T;
			typedef typename traits_type::return_type R;
			auto& mem = stack::get<T>(L, 1);
			switch (lua_gettop(L)) {
			case 1: {
				decltype(auto) r = (mem.*variable);
				stack::push_reference(L, std::forward<decltype(r)>(r));
				return 1;
			}
			case 2:
				return call_set_variable<R, V, variable>(meta::neg<std::is_const<R>>(), L, mem);
			default:
				return luaL_error(L, "incorrect number of arguments to member variable function call");
			}
		}

		template <typename F, F fx>
		inline int call_wrapper_function(std::false_type, lua_State* L) {
			return call_wrapper_variable<F, fx>(std::is_member_object_pointer<F>(), L);
		}

		template <typename F, F fx>
		inline int call_wrapper_function(std::true_type, lua_State* L) {
			return call_detail::call_wrapped<void, false, false>(L, fx);
		}

		template <typename F, F fx>
		int call_wrapper_entry(lua_State* L) noexcept(meta::bind_traits<F>::is_noexcept) {
			return call_wrapper_function<F, fx>(std::is_member_function_pointer<meta::unqualified_t<F>>(), L);
		}

		template <typename... Fxs>
		struct c_call_matcher {
			template <typename Fx, std::size_t I, typename R, typename... Args>
			int operator()(types<Fx>, index_value<I>, types<R>, types<Args...>, lua_State* L, int, int) const {
				typedef meta::at_in_pack_t<I, Fxs...> target;
				return target::call(L);
			}
		};

		template <typename F, F fx>
		inline int c_call_raw(std::true_type, lua_State* L) {
			return fx(L);
		}

		template <typename F, F fx>
		inline int c_call_raw(std::false_type, lua_State* L) {
#ifdef __clang__
			return detail::trampoline(L, function_detail::call_wrapper_entry<F, fx>);
#else
			return detail::typed_static_trampoline<decltype(&function_detail::call_wrapper_entry<F, fx>), (&function_detail::call_wrapper_entry<F, fx>)>(L);
#endif // fuck you clang :c
		}

	} // namespace function_detail

	template <typename F, F fx>
	inline int c_call(lua_State* L) {
		typedef meta::unqualified_t<F> Fu;
		typedef std::integral_constant<bool, std::is_same<Fu, lua_CFunction>::value
#if defined(SOL_NOEXCEPT_FUNCTION_TYPE) && SOL_NOEXCEPT_FUNCTION_TYPE
			|| std::is_same<Fu, detail::lua_CFunction_noexcept>::value
#endif
		> is_raw;
		return function_detail::c_call_raw<F, fx>(is_raw(), L);
	}

	template <typename F, F f>
	struct wrap {
		typedef F type;

		static int call(lua_State* L) {
			return c_call<type, f>(L);
		}
	};

	template <typename... Fxs>
	inline int c_call(lua_State* L) {
		if (sizeof...(Fxs) < 2) {
			return meta::at_in_pack_t<0, Fxs...>::call(L);
		}
		else {
			return call_detail::overload_match_arity<typename Fxs::type...>(function_detail::c_call_matcher<Fxs...>(), L, lua_gettop(L), 1);
		}
	}

} // namespace sol

// end of sol/function_types_templated.hpp

// beginning of sol/function_types_stateless.hpp

namespace sol {
namespace function_detail {
	template <typename Function, bool is_yielding>
	struct upvalue_free_function {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef meta::bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(traits_type::is_noexcept) {
			auto udata = stack::stack_detail::get_as_upvalues<function_type*>(L);
			function_type* fx = udata.first;
			return call_detail::call_wrapped<void, true, false>(L, fx);
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_member_function {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef lua_bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(traits_type::is_noexcept) {
			// Layout:
			// idx 1...n: verbatim data of member function pointer
			// idx n + 1: is the object's void pointer
			// We don't need to store the size, because the other side is templated
			// with the same member function pointer type
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			auto objdata = stack::stack_detail::get_as_upvalues<T*>(L, memberdata.second);
			function_type& memfx = memberdata.first;
			auto& item = *objdata.first;
			return call_detail::call_wrapped<T, true, false, -1>(L, memfx, item);
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_member_variable {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef lua_bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(traits_type::is_noexcept) {
			// Layout:
			// idx 1...n: verbatim data of member variable pointer
			// idx n + 1: is the object's void pointer
			// We don't need to store the size, because the other side is templated
			// with the same member function pointer type
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			auto objdata = stack::stack_detail::get_as_upvalues<T*>(L, memberdata.second);
			auto& mem = *objdata.first;
			function_type& var = memberdata.first;
			switch (lua_gettop(L)) {
			case 0:
				return call_detail::call_wrapped<T, true, false, -1>(L, var, mem);
			case 1:
				return call_detail::call_wrapped<T, false, false, -1>(L, var, mem);
			default:
				return luaL_error(L, "sol: incorrect number of arguments to member variable function");
			}
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_member_variable<T, readonly_wrapper<Function>, is_yielding> {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef lua_bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(traits_type::is_noexcept) {
			// Layout:
			// idx 1...n: verbatim data of member variable pointer
			// idx n + 1: is the object's void pointer
			// We don't need to store the size, because the other side is templated
			// with the same member function pointer type
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			auto objdata = stack::stack_detail::get_as_upvalues<T*>(L, memberdata.second);
			auto& mem = *objdata.first;
			function_type& var = memberdata.first;
			switch (lua_gettop(L)) {
			case 0:
				return call_detail::call_wrapped<T, true, false, -1>(L, var, mem);
			default:
				return luaL_error(L, "sol: incorrect number of arguments to member variable function");
			}
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_this_member_function {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef lua_bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(traits_type::is_noexcept) {
			// Layout:
			// idx 1...n: verbatim data of member variable pointer
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			function_type& memfx = memberdata.first;
			return call_detail::call_wrapped<T, false, false>(L, memfx);
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_this_member_variable {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;

		static int real_call(lua_State* L) noexcept(false) {
			// Layout:
			// idx 1...n: verbatim data of member variable pointer
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			function_type& var = memberdata.first;
			switch (lua_gettop(L)) {
			case 1:
				return call_detail::call_wrapped<T, true, false>(L, var);
			case 2:
				return call_detail::call_wrapped<T, false, false>(L, var);
			default:
				return luaL_error(L, "sol: incorrect number of arguments to member variable function");
			}
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};

	template <typename T, typename Function, bool is_yielding>
	struct upvalue_this_member_variable<T, readonly_wrapper<Function>, is_yielding> {
		typedef std::remove_pointer_t<std::decay_t<Function>> function_type;
		typedef lua_bind_traits<function_type> traits_type;

		static int real_call(lua_State* L) noexcept(false) {
			// Layout:
			// idx 1...n: verbatim data of member variable pointer
			auto memberdata = stack::stack_detail::get_as_upvalues<function_type>(L);
			function_type& var = memberdata.first;
			switch (lua_gettop(L)) {
			case 1:
				return call_detail::call_wrapped<T, true, false>(L, var);
			default:
				return luaL_error(L, "sol: incorrect number of arguments to member variable function");
			}
		}

		static int call(lua_State* L) {
			int nr = detail::typed_static_trampoline<decltype(&real_call), (&real_call)>(L);
			if (is_yielding) {
				return lua_yield(L, nr);
			}
			else {
				return nr;
			}
		}

		int operator()(lua_State* L) {
			return call(L);
		}
	};
}
} // namespace sol::function_detail

// end of sol/function_types_stateless.hpp

// beginning of sol/function_types_stateful.hpp

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

// end of sol/function_types_stateful.hpp

// beginning of sol/function_types_overloaded.hpp

namespace sol {
namespace function_detail {
	template <int start_skew = 0, typename... Functions>
	struct overloaded_function {
		typedef std::tuple<Functions...> overload_list;
		typedef std::make_index_sequence<sizeof...(Functions)> indices;
		overload_list overloads;

		overloaded_function(overload_list set)
		: overloads(std::move(set)) {
		}

		overloaded_function(Functions... fxs)
		: overloads(fxs...) {
		}

		template <typename Fx, std::size_t I, typename... R, typename... Args>
		int call(types<Fx>, index_value<I>, types<R...>, types<Args...>, lua_State* L, int, int) {
			auto& func = std::get<I>(overloads);
			return call_detail::call_wrapped<void, true, false, start_skew>(L, func);
		}

		int operator()(lua_State* L) {
			auto mfx = [&](auto&&... args) { return this->call(std::forward<decltype(args)>(args)...); };
			return call_detail::overload_match<Functions...>(mfx, L, 1 + start_skew);
		}
	};
}
} // namespace sol::function_detail

// end of sol/function_types_overloaded.hpp

// beginning of sol/resolve.hpp

namespace sol {

#ifndef __clang__
	// constexpr is fine for not-clang

	namespace detail {
		template <typename R, typename... Args, typename F, typename = std::result_of_t<meta::unqualified_t<F>(Args...)>>
		inline constexpr auto resolve_i(types<R(Args...)>, F &&) -> R (meta::unqualified_t<F>::*)(Args...) {
			using Sig = R(Args...);
			typedef meta::unqualified_t<F> Fu;
			return static_cast<Sig Fu::*>(&Fu::operator());
		}

		template <typename F, typename U = meta::unqualified_t<F>>
		inline constexpr auto resolve_f(std::true_type, F&& f)
			-> decltype(resolve_i(types<meta::function_signature_t<decltype(&U::operator())>>(), std::forward<F>(f))) {
			return resolve_i(types<meta::function_signature_t<decltype(&U::operator())>>(), std::forward<F>(f));
		}

		template <typename F>
		inline constexpr void resolve_f(std::false_type, F&&) {
			static_assert(meta::has_deducible_signature<F>::value,
				"Cannot use no-template-parameter call with an overloaded functor: specify the signature");
		}

		template <typename F, typename U = meta::unqualified_t<F>>
		inline constexpr auto resolve_i(types<>, F&& f) -> decltype(resolve_f(meta::has_deducible_signature<U>(), std::forward<F>(f))) {
			return resolve_f(meta::has_deducible_signature<U>{}, std::forward<F>(f));
		}

		template <typename... Args, typename F, typename R = std::result_of_t<F&(Args...)>>
		inline constexpr auto resolve_i(types<Args...>, F&& f) -> decltype(resolve_i(types<R(Args...)>(), std::forward<F>(f))) {
			return resolve_i(types<R(Args...)>(), std::forward<F>(f));
		}

		template <typename Sig, typename C>
		inline constexpr Sig C::*resolve_v(std::false_type, Sig C::*mem_func_ptr) {
			return mem_func_ptr;
		}

		template <typename Sig, typename C>
		inline constexpr Sig C::*resolve_v(std::true_type, Sig C::*mem_variable_ptr) {
			return mem_variable_ptr;
		}
	} // namespace detail

	template <typename... Args, typename R>
	inline constexpr auto resolve(R fun_ptr(Args...)) -> R (*)(Args...) {
		return fun_ptr;
	}

	template <typename Sig>
	inline constexpr Sig* resolve(Sig* fun_ptr) {
		return fun_ptr;
	}

	template <typename... Args, typename R, typename C>
	inline constexpr auto resolve(R (C::*mem_ptr)(Args...)) -> R (C::*)(Args...) {
		return mem_ptr;
	}

	template <typename Sig, typename C>
	inline constexpr Sig C::*resolve(Sig C::*mem_ptr) {
		return detail::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
	}

	template <typename... Sig, typename F, meta::disable<std::is_function<meta::unqualified_t<F>>> = meta::enabler>
	inline constexpr auto resolve(F&& f) -> decltype(detail::resolve_i(types<Sig...>(), std::forward<F>(f))) {
		return detail::resolve_i(types<Sig...>(), std::forward<F>(f));
	}
#else

	// Clang has distinct problems with constexpr arguments,
	// so don't use the constexpr versions inside of clang.

	namespace detail {
		template <typename R, typename... Args, typename F, typename = std::result_of_t<meta::unqualified_t<F>(Args...)>>
		inline auto resolve_i(types<R(Args...)>, F &&) -> R (meta::unqualified_t<F>::*)(Args...) {
			using Sig = R(Args...);
			typedef meta::unqualified_t<F> Fu;
			return static_cast<Sig Fu::*>(&Fu::operator());
		}

		template <typename F, typename U = meta::unqualified_t<F>>
		inline auto resolve_f(std::true_type, F&& f)
			-> decltype(resolve_i(types<meta::function_signature_t<decltype(&U::operator())>>(), std::forward<F>(f))) {
			return resolve_i(types<meta::function_signature_t<decltype(&U::operator())>>(), std::forward<F>(f));
		}

		template <typename F>
		inline void resolve_f(std::false_type, F&&) {
			static_assert(meta::has_deducible_signature<F>::value,
				"Cannot use no-template-parameter call with an overloaded functor: specify the signature");
		}

		template <typename F, typename U = meta::unqualified_t<F>>
		inline auto resolve_i(types<>, F&& f) -> decltype(resolve_f(meta::has_deducible_signature<U>(), std::forward<F>(f))) {
			return resolve_f(meta::has_deducible_signature<U>{}, std::forward<F>(f));
		}

		template <typename... Args, typename F, typename R = std::result_of_t<F&(Args...)>>
		inline auto resolve_i(types<Args...>, F&& f) -> decltype(resolve_i(types<R(Args...)>(), std::forward<F>(f))) {
			return resolve_i(types<R(Args...)>(), std::forward<F>(f));
		}

		template <typename Sig, typename C>
		inline Sig C::*resolve_v(std::false_type, Sig C::*mem_func_ptr) {
			return mem_func_ptr;
		}

		template <typename Sig, typename C>
		inline Sig C::*resolve_v(std::true_type, Sig C::*mem_variable_ptr) {
			return mem_variable_ptr;
		}
	} // namespace detail

	template <typename... Args, typename R>
	inline auto resolve(R fun_ptr(Args...)) -> R (*)(Args...) {
		return fun_ptr;
	}

	template <typename Sig>
	inline Sig* resolve(Sig* fun_ptr) {
		return fun_ptr;
	}

	template <typename... Args, typename R, typename C>
	inline auto resolve(R (C::*mem_ptr)(Args...)) -> R (C::*)(Args...) {
		return mem_ptr;
	}

	template <typename Sig, typename C>
	inline Sig C::*resolve(Sig C::*mem_ptr) {
		return detail::resolve_v(std::is_member_object_pointer<Sig C::*>(), mem_ptr);
	}

	template <typename... Sig, typename F>
	inline auto resolve(F&& f) -> decltype(detail::resolve_i(types<Sig...>(), std::forward<F>(f))) {
		return detail::resolve_i(types<Sig...>(), std::forward<F>(f));
	}

#endif

} // namespace sol

// end of sol/resolve.hpp

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

// end of sol/function_types.hpp

namespace sol {
	template <typename base_t, bool aligned = false>
	class basic_function : public base_t {
	private:
		void luacall(std::ptrdiff_t argcount, std::ptrdiff_t resultcount) const {
			lua_call(lua_state(), static_cast<int>(argcount), static_cast<int>(resultcount));
		}

		template <std::size_t... I, typename... Ret>
		auto invoke(types<Ret...>, std::index_sequence<I...>, std::ptrdiff_t n) const {
			luacall(n, lua_size<std::tuple<Ret...>>::value);
			return stack::pop<std::tuple<Ret...>>(lua_state());
		}

		template <std::size_t I, typename Ret>
		Ret invoke(types<Ret>, std::index_sequence<I>, std::ptrdiff_t n) const {
			luacall(n, lua_size<Ret>::value);
			return stack::pop<Ret>(lua_state());
		}

		template <std::size_t I>
		void invoke(types<void>, std::index_sequence<I>, std::ptrdiff_t n) const {
			luacall(n, 0);
		}

		unsafe_function_result invoke(types<>, std::index_sequence<>, std::ptrdiff_t n) const {
			int stacksize = lua_gettop(lua_state());
			int firstreturn = (std::max)(1, stacksize - static_cast<int>(n));
			luacall(n, LUA_MULTRET);
			int poststacksize = lua_gettop(lua_state());
			int returncount = poststacksize - (firstreturn - 1);
			return unsafe_function_result(lua_state(), firstreturn, returncount);
		}

	public:
		using base_t::lua_state;

		basic_function() = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_function>>, meta::neg<std::is_same<base_t, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_function(T&& r) noexcept
		: base_t(std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_function<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_function>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_function(const basic_function&) = default;
		basic_function& operator=(const basic_function&) = default;
		basic_function(basic_function&&) = default;
		basic_function& operator=(basic_function&&) = default;
		basic_function(const stack_reference& r)
		: basic_function(r.lua_state(), r.stack_index()) {
		}
		basic_function(stack_reference&& r)
		: basic_function(r.lua_state(), r.stack_index()) {
		}
		basic_function(lua_nil_t n)
		: base_t(n) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_function(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_function>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_function(lua_State* L, int index = -1)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_function>(L, index, handler);
#endif // Safety
		}
		basic_function(lua_State* L, ref_index index)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_function>(lua_state(), -1, handler);
#endif // Safety
		}

		template <typename... Args>
		unsafe_function_result operator()(Args&&... args) const {
			return call<>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) operator()(types<Ret...>, Args&&... args) const {
			return call<Ret...>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) const {
			if (!aligned) {
				base_t::push();
			}
			int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
			return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount);
		}
	};
} // namespace sol

// end of sol/unsafe_function.hpp

// beginning of sol/protected_function.hpp

// beginning of sol/protected_handler.hpp

namespace sol {
	namespace detail {
		inline const char(&default_handler_name())[9]{
			static const char name[9] = "sol.\xF0\x9F\x94\xA9";
			return name;
		}

		template <bool b, typename target_t = reference>
		struct protected_handler {
			typedef is_stack_based<target_t> is_stack;
			const target_t& target;
			int stackindex;

			protected_handler(std::false_type, const target_t& target)
				: target(target), stackindex(0) {
				if (b) {
					stackindex = lua_gettop(target.lua_state()) + 1;
					target.push();
				}
			}

			protected_handler(std::true_type, const target_t& target)
				: target(target), stackindex(0) {
				if (b) {
					stackindex = target.stack_index();
				}
			}

			protected_handler(const target_t& target)
				: protected_handler(is_stack(), target) {
			}

			bool valid() const noexcept {
				return b;
			}

			~protected_handler() {
				if (!is_stack::value && stackindex != 0) {
					lua_remove(target.lua_state(), stackindex);
				}
			}
		};

		template <typename base_t, typename T>
		basic_function<base_t> force_cast(T& p) {
			return p;
		}

		template <typename Reference, bool is_main_ref = false>
		static Reference get_default_handler(lua_State* L) {
			if (is_stack_based<Reference>::value || L == nullptr)
				return Reference(L, lua_nil);
			L = is_main_ref ? main_thread(L, L) : L;
			lua_getglobal(L, default_handler_name());
			auto pp = stack::pop_n(L, 1);
			return Reference(L, -1);
		}

		template <typename T>
		static void set_default_handler(lua_State* L, const T& ref) {
			if (L == nullptr) {
				return;
			}
			if (!ref.valid()) {
				lua_pushnil(L);
				lua_setglobal(L, default_handler_name());
			}
			else {
				ref.push(L);
				lua_setglobal(L, default_handler_name());
			}
		}
	} // namespace detail
} // namespace sol

// end of sol/protected_handler.hpp

namespace sol {
	template <typename base_t, bool aligned = false, typename handler_t = reference>
	class basic_protected_function : public base_t {
	public:
		typedef is_stack_based<handler_t> is_stack_handler;

		static handler_t get_default_handler(lua_State* L) {
			return detail::get_default_handler<handler_t, is_main_threaded<base_t>::value>(L);
		}

		template <typename T>
		static void set_default_handler(const T& ref) {
			detail::set_default_handler(ref.lua_state(), ref);
		}

	private:
		template <bool b>
		call_status luacall(std::ptrdiff_t argcount, std::ptrdiff_t resultcount, detail::protected_handler<b, handler_t>& h) const {
			return static_cast<call_status>(lua_pcall(lua_state(), static_cast<int>(argcount), static_cast<int>(resultcount), h.stackindex));
		}

		template <std::size_t... I, bool b, typename... Ret>
		auto invoke(types<Ret...>, std::index_sequence<I...>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, sizeof...(Ret), h);
			return stack::pop<std::tuple<Ret...>>(lua_state());
		}

		template <std::size_t I, bool b, typename Ret>
		Ret invoke(types<Ret>, std::index_sequence<I>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, 1, h);
			return stack::pop<Ret>(lua_state());
		}

		template <std::size_t I, bool b>
		void invoke(types<void>, std::index_sequence<I>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, 0, h);
		}

		template <bool b>
		protected_function_result invoke(types<>, std::index_sequence<>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			int stacksize = lua_gettop(lua_state());
			int poststacksize = stacksize;
			int firstreturn = 1;
			int returncount = 0;
			call_status code = call_status::ok;
#if !defined(SOL_NO_EXCEPTIONS) || !SOL_NO_EXCEPTIONS
			auto onexcept = [&](optional<const std::exception&> maybe_ex, const char* error) {
				h.stackindex = 0;
				if (b) {
					h.target.push();
					detail::call_exception_handler(lua_state(), maybe_ex, error);
					lua_call(lua_state(), 1, 1);
				}
				else {
					detail::call_exception_handler(lua_state(), maybe_ex, error);
				}
			};
			(void)onexcept;
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION) || (defined(SOL_LUAJIT) && SOL_LUAJIT)
			try {
#endif // Safe Exception Propagation
#endif // No Exceptions
				firstreturn = (std::max)(1, static_cast<int>(stacksize - n - static_cast<int>(h.valid())));
				code = luacall(n, LUA_MULTRET, h);
				poststacksize = lua_gettop(lua_state()) - static_cast<int>(h.valid());
				returncount = poststacksize - (firstreturn - 1);
#ifndef SOL_NO_EXCEPTIONS
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION) || (defined(SOL_LUAJIT) && SOL_LUAJIT)
			}
			// Handle C++ errors thrown from C++ functions bound inside of lua
			catch (const char* error) {
				onexcept(optional<const std::exception&>(nullopt), error);
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
			catch (const std::string& error) {
				onexcept(optional<const std::exception&>(nullopt), error.c_str());
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
			catch (const std::exception& error) {
				onexcept(optional<const std::exception&>(error), error.what());
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				onexcept(optional<const std::exception&>(nullopt), "caught (...) unknown error during protected_function call");
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
#endif // LuaJIT
#else
			// do not handle exceptions: they can be propogated into C++ and keep all type information / rich information
#endif // Safe Exception Propagation
#endif // Exceptions vs. No Exceptions
			return protected_function_result(lua_state(), firstreturn, returncount, returncount, code);
		}

	public:
		using base_t::lua_state;

		handler_t error_handler;

		basic_protected_function() = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_protected_function>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<T>>>, meta::neg<std::is_same<base_t, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(T&& r) noexcept
		: base_t(std::forward<T>(r)), error_handler(get_default_handler(r.lua_state())) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_function<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_protected_function>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_protected_function(const basic_protected_function&) = default;
		basic_protected_function& operator=(const basic_protected_function&) = default;
		basic_protected_function(basic_protected_function&&) = default;
		basic_protected_function& operator=(basic_protected_function&&) = default;
		basic_protected_function(const basic_function<base_t>& b)
		: basic_protected_function(b, get_default_handler(b.lua_state())) {
		}
		basic_protected_function(basic_function<base_t>&& b)
		: basic_protected_function(std::move(b), get_default_handler(b.lua_state())) {
		}
		basic_protected_function(const basic_function<base_t>& b, handler_t eh)
		: base_t(b), error_handler(std::move(eh)) {
		}
		basic_protected_function(basic_function<base_t>&& b, handler_t eh)
		: base_t(std::move(b)), error_handler(std::move(eh)) {
		}
		basic_protected_function(const stack_reference& r)
		: basic_protected_function(r.lua_state(), r.stack_index(), get_default_handler(r.lua_state())) {
		}
		basic_protected_function(stack_reference&& r)
		: basic_protected_function(r.lua_state(), r.stack_index(), get_default_handler(r.lua_state())) {
		}
		basic_protected_function(const stack_reference& r, handler_t eh)
		: basic_protected_function(r.lua_state(), r.stack_index(), std::move(eh)) {
		}
		basic_protected_function(stack_reference&& r, handler_t eh)
		: basic_protected_function(r.lua_state(), r.stack_index(), std::move(eh)) {
		}

		template <typename Super>
		basic_protected_function(const proxy_base<Super>& p)
		: basic_protected_function(p, get_default_handler(p.lua_state())) {
		}
		template <typename Super>
		basic_protected_function(proxy_base<Super>&& p)
		: basic_protected_function(std::move(p), get_default_handler(p.lua_state())) {
		}
		template <typename Proxy, typename Handler, meta::enable<std::is_base_of<proxy_base_tag, meta::unqualified_t<Proxy>>, meta::neg<is_lua_index<meta::unqualified_t<Handler>>>> = meta::enabler>
		basic_protected_function(Proxy&& p, Handler&& eh)
		: basic_protected_function(detail::force_cast<base_t>(p), std::forward<Handler>(eh)) {
		}

		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(lua_State* L, T&& r)
		: basic_protected_function(L, std::forward<T>(r), get_default_handler(L)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(lua_State* L, T&& r, handler_t eh)
		: base_t(L, std::forward<T>(r)), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_protected_function>(lua_state(), -1, handler);
#endif // Safety
		}
		
		basic_protected_function(lua_nil_t n)
			: base_t(n), error_handler(n) {
		}

		basic_protected_function(lua_State* L, int index = -1)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, int index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, absolute_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, absolute_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, raw_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, raw_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, ref_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, ref_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_protected_function>(lua_state(), -1, handler);
#endif // Safety
		}

		template <typename... Args>
		protected_function_result operator()(Args&&... args) const {
			return call<>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) operator()(types<Ret...>, Args&&... args) const {
			return call<Ret...>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) const {
			if (!aligned) {
				// we do not expect the function to already be on the stack: push it
				if (error_handler.valid()) {
					detail::protected_handler<true, handler_t> h(error_handler);
					base_t::push();
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
				else {
					detail::protected_handler<false, handler_t> h(error_handler);
					base_t::push();
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
			}
			else {
				// the function is already on the stack at the right location
				if (error_handler.valid()) {
					// the handler will be pushed onto the stack manually,
					// since it's not already on the stack this means we need to push our own
					// function on the stack too and swap things to be in-place
					if (!is_stack_handler::value) {
						// so, we need to remove the function at the top and then dump the handler out ourselves
						base_t::push();
					}
					detail::protected_handler<true, handler_t> h(error_handler);
					if (!is_stack_handler::value) {
						lua_replace(lua_state(), -3);
						h.stackindex = lua_absindex(lua_state(), -2);
					}
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
				else {
					detail::protected_handler<false, handler_t> h(error_handler);
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
			}
		}
	};
} // namespace sol

// end of sol/protected_function.hpp

namespace sol {
	template <typename... Ret, typename... Args>
	inline decltype(auto) stack_proxy::call(Args&&... args) {
		stack_function sf(this->lua_state(), this->stack_index());
		return sf.template call<Ret...>(std::forward<Args>(args)...);
	}

	inline protected_function_result::protected_function_result(unsafe_function_result&& o) noexcept
	: L(o.lua_state()), index(o.stack_index()), returncount(o.return_count()), popcount(o.return_count()), err(o.status()) {
		// Must be manual, otherwise destructor will screw us
		// return count being 0 is enough to keep things clean
		// but we will be thorough
		o.abandon();
	}

	inline protected_function_result& protected_function_result::operator=(unsafe_function_result&& o) noexcept {
		L = o.lua_state();
		index = o.stack_index();
		returncount = o.return_count();
		popcount = o.return_count();
		err = o.status();
		// Must be manual, otherwise destructor will screw us
		// return count being 0 is enough to keep things clean
		// but we will be thorough
		o.abandon();
		return *this;
	}

	inline unsafe_function_result::unsafe_function_result(protected_function_result&& o) noexcept
	: L(o.lua_state()), index(o.stack_index()), returncount(o.return_count()) {
		// Must be manual, otherwise destructor will screw us
		// return count being 0 is enough to keep things clean
		// but we will be thorough
		o.abandon();
	}
	inline unsafe_function_result& unsafe_function_result::operator=(protected_function_result&& o) noexcept {
		L = o.lua_state();
		index = o.stack_index();
		returncount = o.return_count();
		// Must be manual, otherwise destructor will screw us
		// return count being 0 is enough to keep things clean
		// but we will be thorough
		o.abandon();
		return *this;
	}

	namespace stack {
		template <typename Signature>
		struct getter<std::function<Signature>> {
			typedef meta::bind_traits<Signature> fx_t;
			typedef typename fx_t::args_list args_lists;
			typedef meta::tuple_types<typename fx_t::return_type> return_types;

			template <typename... Args, typename... Ret>
			static std::function<Signature> get_std_func(types<Ret...>, types<Args...>, lua_State* L, int index) {
				unsafe_function f(L, index);
				auto fx = [ f = std::move(f) ](Args && ... args) -> meta::return_type_t<Ret...> {
					return f.call<Ret...>(std::forward<Args>(args)...);
				};
				return std::move(fx);
			}

			template <typename... FxArgs>
			static std::function<Signature> get_std_func(types<void>, types<FxArgs...>, lua_State* L, int index) {
				unsafe_function f(L, index);
				auto fx = [f = std::move(f)](FxArgs&&... args) -> void {
					f(std::forward<FxArgs>(args)...);
				};
				return std::move(fx);
			}

			template <typename... FxArgs>
			static std::function<Signature> get_std_func(types<>, types<FxArgs...> t, lua_State* L, int index) {
				return get_std_func(types<void>(), t, L, index);
			}

			static std::function<Signature> get(lua_State* L, int index, record& tracking) {
				tracking.last = 1;
				tracking.used += 1;
				type t = type_of(L, index);
				if (t == type::none || t == type::lua_nil) {
					return nullptr;
				}
				return get_std_func(return_types(), args_lists(), L, index);
			}
		};
	} // namespace stack

} // namespace sol

// end of sol/function.hpp

namespace sol {
	template <typename Table, typename Key>
	struct proxy : public proxy_base<proxy<Table, Key>> {
	private:
		typedef meta::condition<meta::is_specialization_of<Key, std::tuple>, Key, std::tuple<meta::condition<std::is_array<meta::unqualified_t<Key>>, Key&, meta::unqualified_t<Key>>>> key_type;

		template <typename T, std::size_t... I>
		decltype(auto) tuple_get(std::index_sequence<I...>) const {
			return tbl.template traverse_get<T>(std::get<I>(key)...);
		}

		template <std::size_t... I, typename T>
		void tuple_set(std::index_sequence<I...>, T&& value) {
			tbl.traverse_set(std::get<I>(key)..., std::forward<T>(value));
		}

		auto setup_table(std::true_type) {
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, tbl.stack_index());
			lua_pop(lua_state(), p.levels);
			return p;
		}

		bool is_valid(std::false_type) {
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			lua_pop(lua_state(), p.levels);
			return p;
		}

	public:
		Table tbl;
		key_type key;

		template <typename T>
		proxy(Table table, T&& k)
		: tbl(table), key(std::forward<T>(k)) {
		}

		template <typename T>
		proxy& set(T&& item) {
			tuple_set(std::make_index_sequence<std::tuple_size<meta::unqualified_t<key_type>>::value>(), std::forward<T>(item));
			return *this;
		}

		template <typename... Args>
		proxy& set_function(Args&&... args) {
			tbl.set_function(key, std::forward<Args>(args)...);
			return *this;
		}

		template <typename U, meta::enable<meta::neg<is_lua_reference_or_proxy<meta::unwrap_unqualified_t<U>>>, meta::is_callable<meta::unwrap_unqualified_t<U>>> = meta::enabler>
		proxy& operator=(U&& other) {
			return set_function(std::forward<U>(other));
		}

		template <typename U, meta::disable<meta::neg<is_lua_reference_or_proxy<meta::unwrap_unqualified_t<U>>>, meta::is_callable<meta::unwrap_unqualified_t<U>>> = meta::enabler>
		proxy& operator=(U&& other) {
			return set(std::forward<U>(other));
		}

		template <typename T>
		proxy& operator=(std::initializer_list<T> other) {
			return set(std::move(other));
		}

		template <typename T>
		decltype(auto) get() const {
			return tuple_get<T>(std::make_index_sequence<std::tuple_size<meta::unqualified_t<key_type>>::value>());
		}

		template <typename T>
		decltype(auto) get_or(T&& otherwise) const {
			typedef decltype(get<T>()) U;
			optional<U> option = get<optional<U>>();
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename D>
		decltype(auto) get_or(D&& otherwise) const {
			optional<T> option = get<optional<T>>();
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		template <typename T>
		decltype(auto) get_or_create() {
			return get_or_create<T>(new_table());
		}

		template <typename T, typename Otherwise>
		decltype(auto) get_or_create(Otherwise&& other) {
			if (!this->valid()) {
				this->set(std::forward<Otherwise>(other));
			}
			return get<T>();
		}

		template <typename K>
		decltype(auto) operator[](K&& k) const {
			auto keys = meta::tuplefy(key, std::forward<K>(k));
			return proxy<Table, decltype(keys)>(tbl, std::move(keys));
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) {
#if !defined(__clang__) && defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191200000
			// MSVC is ass sometimes
			return get<function>().call<Ret...>(std::forward<Args>(args)...);
#else
			return get<function>().template call<Ret...>(std::forward<Args>(args)...);
#endif
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}

		bool valid() const {
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			lua_pop(lua_state(), p.levels);
			return p;
		}

		int push() const noexcept {
			return push(this->lua_state());
		}

		int push(lua_State* L) const noexcept {
			return get<reference>().push(L);
		}

		type get_type() const {
			type t = type::none;
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			if (p) {
				t = type_of(lua_state(), -1);
			}
			lua_pop(lua_state(), p.levels);
			return t;
		}

		lua_State* lua_state() const {
			return tbl.lua_state();
		}

		proxy& force() {
			if (this->valid()) {
				this->set(new_table());
			}
			return *this;
		}
	};

	template <typename Table, typename Key, typename T>
	inline bool operator==(T&& left, const proxy<Table, Key>& right) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() == left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator==(const proxy<Table, Key>& right, T&& left) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() == left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator!=(T&& left, const proxy<Table, Key>& right) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() != left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator!=(const proxy<Table, Key>& right, T&& left) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() != left;
	}

	template <typename Table, typename Key>
	inline bool operator==(lua_nil_t, const proxy<Table, Key>& right) {
		return !right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator==(const proxy<Table, Key>& right, lua_nil_t) {
		return !right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator!=(lua_nil_t, const proxy<Table, Key>& right) {
		return right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator!=(const proxy<Table, Key>& right, lua_nil_t) {
		return right.valid();
	}

	template <bool b>
	template <typename Super>
	basic_reference<b>& basic_reference<b>::operator=(proxy_base<Super>&& r) {
		basic_reference<b> v = r;
		this->operator=(std::move(v));
		return *this;
	}

	template <bool b>
	template <typename Super>
	basic_reference<b>& basic_reference<b>::operator=(const proxy_base<Super>& r) {
		basic_reference<b> v = r;
		this->operator=(std::move(v));
		return *this;
	}

	namespace stack {
		template <typename Table, typename Key>
		struct pusher<proxy<Table, Key>> {
			static int push(lua_State* L, const proxy<Table, Key>& p) {
				reference r = p;
				return r.push(L);
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/proxy.hpp

// beginning of sol/usertype.hpp

// beginning of sol/usertype_metatable.hpp

// beginning of sol/deprecate.hpp

#ifndef SOL_DEPRECATED
#ifdef _MSC_VER
#define SOL_DEPRECATED __declspec(deprecated)
#elif __GNUC__
#define SOL_DEPRECATED __attribute__((deprecated))
#else
#define SOL_DEPRECATED [[deprecated]]
#endif // compilers
#endif // SOL_DEPRECATED

namespace sol {
namespace detail {
	template <typename T>
	struct SOL_DEPRECATED deprecate_type {
		using type = T;
	};
}
} // namespace sol::detail

// end of sol/deprecate.hpp

// beginning of sol/object.hpp

// beginning of sol/object_base.hpp

namespace sol {

	template <typename base_t>
	class basic_object_base : public base_t {
	private:
		template <typename T>
		decltype(auto) as_stack(std::true_type) const {
			return stack::get<T>(base_t::lua_state(), base_t::stack_index());
		}

		template <typename T>
		decltype(auto) as_stack(std::false_type) const {
			base_t::push();
			return stack::pop<T>(base_t::lua_state());
		}

		template <typename T>
		bool is_stack(std::true_type) const {
			return stack::check<T>(base_t::lua_state(), base_t::stack_index(), no_panic);
		}

		template <typename T>
		bool is_stack(std::false_type) const {
			int r = base_t::registry_index();
			if (r == LUA_REFNIL)
				return meta::any_same<meta::unqualified_t<T>, lua_nil_t, nullopt_t, std::nullptr_t>::value ? true : false;
			if (r == LUA_NOREF)
				return false;
			auto pp = stack::push_pop(*this);
			return stack::check<T>(base_t::lua_state(), -1, no_panic);
		}

	public:
		basic_object_base() noexcept = default;
		basic_object_base(const basic_object_base&) = default;
		basic_object_base(basic_object_base&&) = default;
		basic_object_base& operator=(const basic_object_base&) = default;
		basic_object_base& operator=(basic_object_base&&) = default;
		template <typename T, typename... Args, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_object_base>>> = meta::enabler>
		basic_object_base(T&& arg, Args&&... args)
		: base_t(std::forward<T>(arg), std::forward<Args>(args)...) {
		}

		template <typename T>
		decltype(auto) as() const {
			return as_stack<T>(is_stack_based<base_t>());
		}

		template <typename T>
		bool is() const {
			return is_stack<T>(is_stack_based<base_t>());
		}
	};
} // namespace sol

// end of sol/object_base.hpp

// beginning of sol/userdata.hpp

namespace sol {
	template <typename base_type>
	class basic_userdata : public basic_table<base_type> {
		typedef basic_table<base_type> base_t;

	public:
		using base_t::lua_state;

		basic_userdata() noexcept = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_userdata>>, meta::neg<std::is_same<base_t, stack_reference>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_userdata(T&& r) noexcept
		: base_t(std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_userdata<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				type_assert(lua_state(), -1, type::userdata);
			}
#endif // Safety
		}
		basic_userdata(const basic_userdata&) = default;
		basic_userdata(basic_userdata&&) = default;
		basic_userdata& operator=(const basic_userdata&) = default;
		basic_userdata& operator=(basic_userdata&&) = default;
		basic_userdata(const stack_reference& r)
		: basic_userdata(r.lua_state(), r.stack_index()) {
		}
		basic_userdata(stack_reference&& r)
		: basic_userdata(r.lua_state(), r.stack_index()) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_userdata(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_userdata>(L, -1, handler);
#endif // Safety
		}
		basic_userdata(lua_State* L, int index = -1)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_userdata>(L, index, handler);
#endif // Safety
		}
		basic_userdata(lua_State* L, ref_index index)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_userdata>(L, -1, handler);
#endif // Safety
		}
	};

	template <typename base_type>
	class basic_lightuserdata : public basic_object_base<base_type> {
		typedef basic_object_base<base_type> base_t;

	public:
		using base_t::lua_state;

		basic_lightuserdata() noexcept = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_lightuserdata>>, meta::neg<std::is_same<base_t, stack_reference>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_lightuserdata(T&& r) noexcept
		: base_t(std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_lightuserdata<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				type_assert(lua_state(), -1, type::lightuserdata);
			}
#endif // Safety
		}
		basic_lightuserdata(const basic_lightuserdata&) = default;
		basic_lightuserdata(basic_lightuserdata&&) = default;
		basic_lightuserdata& operator=(const basic_lightuserdata&) = default;
		basic_lightuserdata& operator=(basic_lightuserdata&&) = default;
		basic_lightuserdata(const stack_reference& r)
		: basic_lightuserdata(r.lua_state(), r.stack_index()) {
		}
		basic_lightuserdata(stack_reference&& r)
		: basic_lightuserdata(r.lua_state(), r.stack_index()) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_lightuserdata(lua_State* L, T&& r)
		: basic_lightuserdata(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_lightuserdata>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_lightuserdata(lua_State* L, int index = -1)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_lightuserdata>(L, index, handler);
#endif // Safety
		}
		basic_lightuserdata(lua_State* L, ref_index index)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_lightuserdata>(lua_state(), index, handler);
#endif // Safety
		}
	};

} // namespace sol

// end of sol/userdata.hpp

// beginning of sol/as_args.hpp

namespace sol {
	template <typename T>
	struct as_args_t {
		T src;
	};

	template <typename Source>
	auto as_args(Source&& source) {
		return as_args_t<Source>{ std::forward<Source>(source) };
	}

	namespace stack {
		template <typename T>
		struct pusher<as_args_t<T>> {
			int push(lua_State* L, const as_args_t<T>& e) {
				int p = 0;
				for (const auto& i : e.src) {
					p += stack::push(L, i);
				}
				return p;
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/as_args.hpp

// beginning of sol/variadic_args.hpp

namespace sol {
	struct variadic_args {
	private:
		lua_State* L;
		int index;
		int stacktop;

	public:
		typedef stack_proxy reference_type;
		typedef stack_proxy value_type;
		typedef stack_proxy* pointer;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef stack_iterator<stack_proxy, false> iterator;
		typedef stack_iterator<stack_proxy, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		variadic_args() = default;
		variadic_args(lua_State* luastate, int stackindex = -1)
			: L(luastate), index(lua_absindex(luastate, stackindex)), stacktop(lua_gettop(luastate)) {
		}
		variadic_args(lua_State* luastate, int stackindex, int lastindex)
			: L(luastate), index(lua_absindex(luastate, stackindex)), stacktop(lastindex) {
		}
		variadic_args(const variadic_args&) = default;
		variadic_args& operator=(const variadic_args&) = default;
		variadic_args(variadic_args&& o)
			: L(o.L), index(o.index), stacktop(o.stacktop) {
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but will be thorough
			o.L = nullptr;
			o.index = 0;
			o.stacktop = 0;
		}
		variadic_args& operator=(variadic_args&& o) {
			L = o.L;
			index = o.index;
			stacktop = o.stacktop;
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but will be thorough
			o.L = nullptr;
			o.index = 0;
			o.stacktop = 0;
			return *this;
		}

		iterator begin() {
			return iterator(L, index, stacktop + 1);
		}
		iterator end() {
			return iterator(L, stacktop + 1, stacktop + 1);
		}
		const_iterator begin() const {
			return const_iterator(L, index, stacktop + 1);
		}
		const_iterator end() const {
			return const_iterator(L, stacktop + 1, stacktop + 1);
		}
		const_iterator cbegin() const {
			return begin();
		}
		const_iterator cend() const {
			return end();
		}

		reverse_iterator rbegin() {
			return std::reverse_iterator<iterator>(begin());
		}
		reverse_iterator rend() {
			return std::reverse_iterator<iterator>(end());
		}
		const_reverse_iterator rbegin() const {
			return std::reverse_iterator<const_iterator>(begin());
		}
		const_reverse_iterator rend() const {
			return std::reverse_iterator<const_iterator>(end());
		}
		const_reverse_iterator crbegin() const {
			return std::reverse_iterator<const_iterator>(cbegin());
		}
		const_reverse_iterator crend() const {
			return std::reverse_iterator<const_iterator>(cend());
		}

		int push() const {
			return push(L);
		}

		int push(lua_State* target) const {
			int pushcount = 0;
			for (int i = index; i <= stacktop; ++i) {
				lua_pushvalue(L, i);
				pushcount += 1;
			}
			if (target != L) {
				lua_xmove(L, target, pushcount);
			}
			return pushcount;
		}

		template <typename T>
		decltype(auto) get(difference_type index_offset = 0) const {
			return stack::get<T>(L, index + static_cast<int>(index_offset));
		}

		type get_type(difference_type index_offset = 0) const noexcept {
			return type_of(L, index + static_cast<int>(index_offset));
		}

		stack_proxy operator[](difference_type index_offset) const {
			return stack_proxy(L, index + static_cast<int>(index_offset));
		}

		lua_State* lua_state() const {
			return L;
		};
		int stack_index() const {
			return index;
		};
		int leftover_count() const {
			return stacktop - (index - 1);
		}
		std::size_t size() const {
			return static_cast<std::size_t>(leftover_count());
		}
		int top() const {
			return stacktop;
		}
	};

	namespace stack {
		template <>
		struct getter<variadic_args> {
			static variadic_args get(lua_State* L, int index, record& tracking) {
				tracking.last = 0;
				return variadic_args(L, index);
			}
		};

		template <>
		struct pusher<variadic_args> {
			static int push(lua_State* L, const variadic_args& ref) {
				return ref.push(L);
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/variadic_args.hpp

namespace sol {

	template <typename R = reference, bool should_pop = !is_stack_based<R>::value, typename T>
	R make_reference(lua_State* L, T&& value) {
		int backpedal = stack::push(L, std::forward<T>(value));
		R r = stack::get<R>(L, -backpedal);
		if (should_pop) {
			lua_pop(L, backpedal);
		}
		return r;
	}

	template <typename T, typename R = reference, bool should_pop = !is_stack_based<R>::value, typename... Args>
	R make_reference(lua_State* L, Args&&... args) {
		int backpedal = stack::push<T>(L, std::forward<Args>(args)...);
		R r = stack::get<R>(L, -backpedal);
		if (should_pop) {
			lua_pop(L, backpedal);
		}
		return r;
	}

	template <typename base_type>
	class basic_object : public basic_object_base<base_type> {
	private:
		typedef basic_object_base<base_type> base_t;

		template <bool invert_and_pop = false>
		basic_object(std::integral_constant<bool, invert_and_pop>, lua_State* L, int index = -1) noexcept
		: base_t(L, index) {
			if (invert_and_pop) {
				lua_pop(L, -index);
			}
		}

	public:
		basic_object() noexcept = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_object>>, meta::neg<std::is_same<base_type, stack_reference>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_object(T&& r)
		: base_t(std::forward<T>(r)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_object(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
		}
		basic_object(lua_nil_t r)
		: base_t(r) {
		}
		basic_object(const basic_object&) = default;
		basic_object(basic_object&&) = default;
		basic_object(const stack_reference& r) noexcept
		: basic_object(r.lua_state(), r.stack_index()) {
		}
		basic_object(stack_reference&& r) noexcept
		: basic_object(r.lua_state(), r.stack_index()) {
		}
		template <typename Super>
		basic_object(const proxy_base<Super>& r) noexcept
		: basic_object(r.operator basic_object()) {
		}
		template <typename Super>
		basic_object(proxy_base<Super>&& r) noexcept
		: basic_object(r.operator basic_object()) {
		}
		basic_object(lua_State* L, lua_nil_t r) noexcept
		: base_t(L, r) {
		}
		basic_object(lua_State* L, int index = -1) noexcept
		: base_t(L, index) {
		}
		basic_object(lua_State* L, absolute_index index) noexcept
		: base_t(L, index) {
		}
		basic_object(lua_State* L, raw_index index) noexcept
		: base_t(L, index) {
		}
		basic_object(lua_State* L, ref_index index) noexcept
		: base_t(L, index) {
		}
		template <typename T, typename... Args>
		basic_object(lua_State* L, in_place_type_t<T>, Args&&... args) noexcept
		: basic_object(std::integral_constant<bool, !is_stack_based<base_t>::value>(), L, -stack::push<T>(L, std::forward<Args>(args)...)) {
		}
		template <typename T, typename... Args>
		basic_object(lua_State* L, in_place_t, T&& arg, Args&&... args) noexcept
		: basic_object(L, in_place_type<T>, std::forward<T>(arg), std::forward<Args>(args)...) {
		}
		basic_object& operator=(const basic_object&) = default;
		basic_object& operator=(basic_object&&) = default;
		basic_object& operator=(const base_type& b) {
			base_t::operator=(b);
			return *this;
		}
		basic_object& operator=(base_type&& b) {
			base_t::operator=(std::move(b));
			return *this;
		}
		template <typename Super>
		basic_object& operator=(const proxy_base<Super>& r) {
			this->operator=(r.operator basic_object());
			return *this;
		}
		template <typename Super>
		basic_object& operator=(proxy_base<Super>&& r) {
			this->operator=(r.operator basic_object());
			return *this;
		}
	};

	template <typename T>
	object make_object(lua_State* L, T&& value) {
		return make_reference<object, true>(L, std::forward<T>(value));
	}

	template <typename T, typename... Args>
	object make_object(lua_State* L, Args&&... args) {
		return make_reference<T, object, true>(L, std::forward<Args>(args)...);
	}
} // namespace sol

// end of sol/object.hpp

// beginning of sol/container_usertype_metatable.hpp

// beginning of sol/container_traits.hpp

namespace sol {

	template <typename T>
	struct container_traits;

	template <typename T>
	struct as_container_t {
		T source;

		as_container_t(T value)
		: source(std::move(value)) {
		}

		operator std::add_rvalue_reference_t<T>() {
			return std::move(source);
		}

		operator std::add_lvalue_reference_t<std::add_const_t<T>>() const {
			return source;
		}
	};

	template <typename T>
	struct as_container_t<T&> {
		std::reference_wrapper<T> source;

		as_container_t(T& value)
		: source(value) {
		}

		operator T&() {
			return source;
		}
	};

	template <typename T>
	auto as_container(T&& value) {
		return as_container_t<T>(std::forward<T>(value));
	}

	namespace container_detail {

		template <typename T>
		struct has_clear_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::clear));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_empty_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::empty));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_erase_after_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(std::declval<C>().erase_after(std::declval<std::add_rvalue_reference_t<typename C::const_iterator>>()))*);
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T, typename = void>
		struct has_find_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(std::declval<C>().find(std::declval<std::add_rvalue_reference_t<typename C::value_type>>()))*);
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_find_test<T, std::enable_if_t<meta::is_lookup<T>::value>> {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(std::declval<C>().find(std::declval<std::add_rvalue_reference_t<typename C::key_type>>()))*);
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_erase_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(std::declval<C>().erase(std::declval<typename C::iterator>()))*);
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_find_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::find));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_insert_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::insert));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_erase_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::erase));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_index_set_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::index_set));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_index_get_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::index_get));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_set_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::set));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_get_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::get));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_at_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::at));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_pairs_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::pairs));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_ipairs_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::ipairs));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_next_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::next));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_add_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::add));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		struct has_traits_size_test {
		private:
			typedef std::array<char, 1> one;
			typedef std::array<char, 2> two;

			template <typename C>
			static one test(decltype(&C::size));
			template <typename C>
			static two test(...);

		public:
			static const bool value = sizeof(test<T>(0)) == sizeof(char);
		};

		template <typename T>
		using has_clear = meta::boolean<has_clear_test<T>::value>;

		template <typename T>
		using has_empty = meta::boolean<has_empty_test<T>::value>;

		template <typename T>
		using has_find = meta::boolean<has_find_test<T>::value>;

		template <typename T>
		using has_erase = meta::boolean<has_erase_test<T>::value>;

		template <typename T>
		using has_erase_after = meta::boolean<has_erase_after_test<T>::value>;

		template <typename T>
		using has_traits_get = meta::boolean<has_traits_get_test<T>::value>;

		template <typename T>
		using has_traits_at = meta::boolean<has_traits_at_test<T>::value>;

		template <typename T>
		using has_traits_set = meta::boolean<has_traits_set_test<T>::value>;

		template <typename T>
		using has_traits_index_get = meta::boolean<has_traits_index_get_test<T>::value>;

		template <typename T>
		using has_traits_index_set = meta::boolean<has_traits_index_set_test<T>::value>;

		template <typename T>
		using has_traits_pairs = meta::boolean<has_traits_pairs_test<T>::value>;

		template <typename T>
		using has_traits_ipairs = meta::boolean<has_traits_ipairs_test<T>::value>;

		template <typename T>
		using has_traits_next = meta::boolean<has_traits_next_test<T>::value>;

		template <typename T>
		using has_traits_add = meta::boolean<has_traits_add_test<T>::value>;

		template <typename T>
		using has_traits_size = meta::boolean<has_traits_size_test<T>::value>;

		template <typename T>
		using has_traits_clear = has_clear<T>;

		template <typename T>
		using has_traits_empty = has_empty<T>;

		template <typename T>
		using has_traits_find = meta::boolean<has_traits_find_test<T>::value>;

		template <typename T>
		using has_traits_insert = meta::boolean<has_traits_insert_test<T>::value>;

		template <typename T>
		using has_traits_erase = meta::boolean<has_traits_erase_test<T>::value>;

		template <typename T>
		struct is_forced_container : is_container<T> {};

		template <typename T>
		struct is_forced_container<as_container_t<T>> : std::true_type {};

		template <typename T>
		struct container_decay {
			typedef T type;
		};

		template <typename T>
		struct container_decay<as_container_t<T>> {
			typedef T type;
		};

		template <typename T>
		using container_decay_t = typename container_decay<meta::unqualified_t<T>>::type;

		template <typename T>
		decltype(auto) get_key(std::false_type, T&& t) {
			return std::forward<T>(t);
		}

		template <typename T>
		decltype(auto) get_key(std::true_type, T&& t) {
			return t.first;
		}

		template <typename T>
		decltype(auto) get_value(std::false_type, T&& t) {
			return std::forward<T>(t);
		}

		template <typename T>
		decltype(auto) get_value(std::true_type, T&& t) {
			return t.second;
		}

		struct error_result {
			int results;
			const char* fmt_;
			std::array<const char*, 4> args;

			error_result() : results(0), fmt_(nullptr) {
			}

			error_result(int results) : results(results), fmt_(nullptr) {
			}

			error_result(const char* fmt_, const char* msg) : results(0), fmt_(fmt_) {
				args[0] = msg;
			}
		};

		inline int handle_errors(lua_State* L, const error_result& er) {
			if (er.fmt_ == nullptr) {
				return er.results;
			}
			return luaL_error(L, er.fmt_, er.args[0], er.args[1], er.args[2], er.args[3]);
		}

		template <typename X, typename = void>
		struct container_traits_default {
		private:
			typedef std::remove_pointer_t<meta::unwrap_unqualified_t<X>> T;

		public:
			typedef lua_nil_t iterator;
			typedef lua_nil_t value_type;

			static int at(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'at(index)' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int get(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'get(key)' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int index_get(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'container[key]' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int set(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'set(key, value)' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int index_set(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'container[key] = value' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int add(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'add' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int insert(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'insert' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int find(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'find' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int size(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'end' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int clear(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'clear' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int empty(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'empty' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int erase(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'erase' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int next(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'next' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int pairs(lua_State* L) {
				return luaL_error(L, "sol: cannot call '__pairs/pairs' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static int ipairs(lua_State* L) {
				return luaL_error(L, "sol: cannot call '__ipairs' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
			}

			static iterator begin(lua_State* L, T&) {
				luaL_error(L, "sol: cannot call 'being' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
				return lua_nil;
			}

			static iterator end(lua_State* L, T&) {
				luaL_error(L, "sol: cannot call 'end' on type '%s': it is not recognized as a container", detail::demangle<T>().c_str());
				return lua_nil;
			}
		};

		template <typename X>
		struct container_traits_default<X, std::enable_if_t<meta::all<is_forced_container<meta::unqualified_t<X>>, meta::has_value_type<meta::unqualified_t<container_decay_t<X>>>, meta::has_iterator<meta::unqualified_t<container_decay_t<X>>>>::value>> {
		private:
			typedef std::remove_pointer_t<meta::unwrap_unqualified_t<container_decay_t<X>>> T;

		private:
			typedef container_traits<X> deferred_traits;
			typedef meta::is_associative<T> is_associative;
			typedef meta::is_lookup<T> is_lookup;
			typedef meta::is_matched_lookup<T> is_matched_lookup;
			typedef typename T::iterator iterator;
			typedef typename T::value_type value_type;
			typedef std::conditional_t<is_matched_lookup::value,
				std::pair<value_type, value_type>,
				std::conditional_t<is_associative::value || is_lookup::value,
					value_type,
					std::pair<std::ptrdiff_t, value_type>
				>
			> KV;
			typedef typename KV::first_type K;
			typedef typename KV::second_type V;
			typedef std::conditional_t<is_matched_lookup::value, std::ptrdiff_t, K> next_K;
			typedef decltype(*std::declval<iterator&>()) iterator_return;
			typedef std::conditional_t<is_associative::value || is_matched_lookup::value,
				std::add_lvalue_reference_t<V>,
				std::conditional_t<is_lookup::value,
					V,
					iterator_return
				>
			> captured_type;
			typedef typename meta::iterator_tag<iterator>::type iterator_category;
			typedef std::is_same<iterator_category, std::input_iterator_tag> is_input_iterator;
			typedef std::conditional_t<is_input_iterator::value,
				V,
				decltype(detail::deref_non_pointer(std::declval<captured_type>()))
			> push_type;
			typedef std::is_copy_assignable<V> is_copyable;
			typedef meta::neg<meta::any<
				std::is_const<V>, std::is_const<std::remove_reference_t<iterator_return>>, meta::neg<is_copyable>
			>> is_writable;
			typedef meta::unqualified_t<decltype(get_key(is_associative(), std::declval<std::add_lvalue_reference_t<value_type>>()))> key_type;
			typedef meta::all<std::is_integral<K>, meta::neg<meta::any<is_associative, is_lookup>>> is_linear_integral;

			struct iter {
				T& source;
				iterator it;
				std::size_t i;

				iter(T& source, iterator it)
				: source(source), it(std::move(it)), i(0) {
				}
			};

			static auto& get_src(lua_State* L) {
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto p = stack::unqualified_check_get<T*>(L, 1);
				if (!p) {
					luaL_error(L, "sol: 'self' is not of type '%s' (pass 'self' as first argument with ':' or call on proper type)", detail::demangle<T>().c_str());
				}
				if (p.value() == nullptr) {
					luaL_error(L, "sol: 'self' argument is nil (pass 'self' as first argument with ':' or call on a '%s' type)", detail::demangle<T>().c_str());
				}
				return *p.value();
#else
				return stack::unqualified_get<T>(L, 1);
#endif // Safe getting with error
			}

			static error_result at_category(std::input_iterator_tag, lua_State* L, T& self, std::ptrdiff_t pos) {
				pos += deferred_traits::index_adjustment(L, self);
				if (pos < 0) {
					return stack::push(L, lua_nil);
				}
				auto it = deferred_traits::begin(L, self);
				auto e = deferred_traits::end(L, self);
				if (it == e) {
					return stack::push(L, lua_nil);
				}
				while (pos > 0) {
					--pos;
					++it;
					if (it == e) {
						return stack::push(L, lua_nil);
					}
				}
				return get_associative(is_associative(), L, it);
			}

			static error_result at_category(std::random_access_iterator_tag, lua_State* L, T& self, std::ptrdiff_t pos) {
				std::ptrdiff_t len = static_cast<std::ptrdiff_t>(size_start(L, self));
				pos += deferred_traits::index_adjustment(L, self);
				if (pos < 0 || pos >= len) {
					return stack::push(L, lua_nil);
				}
				auto it = std::next(deferred_traits::begin(L, self), pos);
				return get_associative(is_associative(), L, it);
			}

			static error_result at_start(lua_State* L, T& self, std::ptrdiff_t pos) {
				return at_category(iterator_category(), L, self, pos);
			}

			static error_result get_associative(std::true_type, lua_State* L, iterator& it) {
				auto& v = *it;
				return stack::stack_detail::push_reference<push_type>(L, detail::deref_non_pointer(v.second));
			}

			static error_result get_associative(std::false_type, lua_State* L, iterator& it) {
				return stack::stack_detail::push_reference<push_type>(L, detail::deref_non_pointer(*it));
			}

			static error_result get_category(std::input_iterator_tag, lua_State* L, T& self, K& key) {
				key += deferred_traits::index_adjustment(L, self);
				if (key < 0) {
					return stack::push(L, lua_nil);
				}
				auto it = deferred_traits::begin(L, self);
				auto e = deferred_traits::end(L, self);
				if (it == e) {
					return stack::push(L, lua_nil);
				}
				while (key > 0) {
					--key;
					++it;
					if (it == e) {
						return stack::push(L, lua_nil);
					}
				}
				return get_associative(is_associative(), L, it);
			}

			static error_result get_category(std::random_access_iterator_tag, lua_State* L, T& self, K& key) {
				std::ptrdiff_t len = static_cast<std::ptrdiff_t>(size_start(L, self));
				key += deferred_traits::index_adjustment(L, self);
				if (key < 0 || key >= len) {
					return stack::push(L, lua_nil);
				}
				auto it = std::next(deferred_traits::begin(L, self), key);
				return get_associative(is_associative(), L, it);
			}

			static error_result get_it(std::true_type, lua_State* L, T& self, K& key) {
				return get_category(iterator_category(), L, self, key);
			}

			static error_result get_comparative(std::true_type, lua_State* L, T& self, K& key) {
				auto fx = [&](const value_type& r) -> bool {
					return key == get_key(is_associative(), r);
				};
				auto e = deferred_traits::end(L, self);
				auto it = std::find_if(deferred_traits::begin(L, self), e, std::ref(fx));
				if (it == e) {
					return stack::push(L, lua_nil);
				}
				return get_associative(is_associative(), L, it);
			}

			static error_result get_comparative(std::false_type, lua_State*, T&, K&) {
				return error_result("cannot get this key on '%s': no suitable way to increment iterator and compare to key value '%s'", detail::demangle<T>().data(), detail::demangle<K>().data());
			}

			static error_result get_it(std::false_type, lua_State* L, T& self, K& key) {
				return get_comparative(meta::supports_op_equal<K, key_type>(), L, self, key);
			}

			static error_result set_associative(std::true_type, iterator& it, stack_object value) {
				auto& v = *it;
				v.second = value.as<V>();
				return {};
			}

			static error_result set_associative(std::false_type, iterator& it, stack_object value) {
				auto& v = *it;
				v = value.as<V>();
				return {};
			}

			static error_result set_writable(std::true_type, lua_State*, T&, iterator& it, stack_object value) {
				return set_associative(is_associative(), it, std::move(value));
			}

			static error_result set_writable(std::false_type, lua_State*, T&, iterator&, stack_object) {
				return error_result("cannot perform a 'set': '%s's iterator reference is not writable (non-copy-assignable or const)", detail::demangle<T>().data());
			}

			static error_result set_category(std::input_iterator_tag, lua_State* L, T& self, stack_object okey, stack_object value) {
				decltype(auto) key = okey.as<K>();
				key += deferred_traits::index_adjustment(L, self);
				auto e = deferred_traits::end(L, self);
				auto it = deferred_traits::begin(L, self);
				auto backit = it;
				for (; key > 0 && it != e; --key, ++it) {
					backit = it;
				}
				if (it == e) {
					if (key == 0) {
						return add_copyable(is_copyable(), L, self, std::move(value), meta::has_insert_after<T>::value ? backit : it);
					}
					return error_result("out of bounds (too big) for set on '%s'", detail::demangle<T>().c_str());
				}
				return set_writable(is_writable(), L, self, it, std::move(value));
			}

			static error_result set_category(std::random_access_iterator_tag, lua_State* L, T& self, stack_object okey, stack_object value) {
				decltype(auto) key = okey.as<K>();
				if (key <= 0) {
					return error_result("sol: out of bounds (too small) for set on '%s'", detail::demangle<T>().c_str());
				}
				key += deferred_traits::index_adjustment(L, self);
				std::ptrdiff_t len = static_cast<std::ptrdiff_t>(size_start(L, self));
				if (key == len) {
					return add_copyable(is_copyable(), L, self, std::move(value));
				}
				else if (key > len) {
					return error_result("sol: out of bounds (too big) for set on '%s'", detail::demangle<T>().c_str());
				}
				auto it = std::next(deferred_traits::begin(L, self), key);
				return set_writable(is_writable(), L, self, it, std::move(value));
			}

			static error_result set_comparative(std::true_type, lua_State* L, T& self, stack_object okey, stack_object value) {
				decltype(auto) key = okey.as<K>();
				if (!is_writable::value) {
					return error_result("cannot perform a 'set': '%s's iterator reference is not writable (non-copy-assignable or const)", detail::demangle<T>().data());
				}
				auto fx = [&](const value_type& r) -> bool {
					return key == get_key(is_associative(), r);
				};
				auto e = deferred_traits::end(L, self);
				auto it = std::find_if(deferred_traits::begin(L, self), e, std::ref(fx));
				if (it == e) {
					return {};
				}
				return set_writable(is_writable(), L, self, it, std::move(value));
			}

			static error_result set_comparative(std::false_type, lua_State*, T&, stack_object, stack_object) {
				return error_result("cannot set this value on '%s': no suitable way to increment iterator or compare to '%s' key", detail::demangle<T>().data(), detail::demangle<K>().data());
			}

			static error_result set_associative_insert(std::true_type, lua_State*, T& self, iterator& it, K& key, stack_object value) {
				self.insert(it, value_type(key, value.as<V>()));
				return {};
			}

			static error_result set_associative_insert(std::false_type, lua_State*, T& self, iterator& it, K& key, stack_object) {
				self.insert(it, key);
				return {};
			}

			static error_result set_associative_find(std::true_type, lua_State* L, T& self, stack_object okey, stack_object value) {
				decltype(auto) key = okey.as<K>();
				auto it = self.find(key);
				if (it == deferred_traits::end(L, self)) {
					return set_associative_insert(is_associative(), L, self, it, key, std::move(value));
				}
				return set_writable(is_writable(), L, self, it, std::move(value));
			}

			static error_result set_associative_find(std::false_type, lua_State* L, T& self, stack_object key, stack_object value) {
				return set_comparative(meta::supports_op_equal<K, key_type>(), L, self, std::move(key), std::move(value));
			}

			static error_result set_it(std::true_type, lua_State* L, T& self, stack_object key, stack_object value) {
				return set_category(iterator_category(), L, self, std::move(key), std::move(value));
			}

			static error_result set_it(std::false_type, lua_State* L, T& self, stack_object key, stack_object value) {
				return set_associative_find(meta::all<has_find<T>, meta::any<is_associative, is_lookup>>(), L, self, std::move(key), std::move(value));
			}

			static error_result find_has_associative_lookup(std::true_type, lua_State* L, T& self) {
				decltype(auto) key = stack::unqualified_get<K>(L, 2);
				auto it = self.find(key);
				if (it == deferred_traits::end(L, self)) {
					return stack::push(L, lua_nil);
				}
				return get_associative(is_associative(), L, it);
			}

			static error_result find_has_associative_lookup(std::false_type, lua_State* L, T& self) {
				decltype(auto) value = stack::unqualified_get<V>(L, 2);
				auto it = self.find(value);
				if (it == deferred_traits::end(L, self)) {
					return stack::push(L, lua_nil);
				}
				return get_associative(is_associative(), L, it);
			}

			static error_result find_has(std::true_type, lua_State* L, T& self) {
				return find_has_associative_lookup(meta::any<is_lookup, is_associative>(), L, self);
			}

			static error_result find_associative_lookup(std::true_type, lua_State* L, iterator& it, std::size_t) {
				return get_associative(is_associative(), L, it);
			}

			static error_result find_associative_lookup(std::false_type, lua_State* L, iterator&, std::size_t index) {
				return stack::push(L, index);
			}

			static error_result find_comparative(std::false_type, lua_State*, T&) {
				return error_result("cannot call 'find' on '%s': there is no 'find' function and the value_type is not equality comparable", detail::demangle<T>().c_str());
			}

			static error_result find_comparative(std::true_type, lua_State* L, T& self) {
				decltype(auto) value = stack::unqualified_get<V>(L, 2);
				auto it = deferred_traits::begin(L, self);
				auto e = deferred_traits::end(L, self);
				std::size_t index = 1;
				for (;; ++it, ++index) {
					if (it == e) {
						return stack::push(L, lua_nil);
					}
					if (value == get_value(is_associative(), *it)) {
						break;
					}
				}
				return find_associative_lookup(meta::any<is_lookup, is_associative>(), L, it, index);
			}

			static error_result find_has(std::false_type, lua_State* L, T& self) {
				return find_comparative(meta::supports_op_equal<V>(), L, self);
			}

			static error_result add_insert_after(std::false_type, lua_State* L, T& self, stack_object value, iterator&) {
				return add_insert_after(std::false_type(), L, self, value);
			}

			static error_result add_insert_after(std::false_type, lua_State*, T&, stack_object) {
				return error_result("cannot call 'add' on type '%s': no suitable insert/push_back C++ functions", detail::demangle<T>().data());
			}

			static error_result add_insert_after(std::true_type, lua_State*, T& self, stack_object value, iterator& pos) {
				self.insert_after(pos, value.as<V>());
				return {};
			}

			static error_result add_insert_after(std::true_type, lua_State* L, T& self, stack_object value) {
				auto backit = self.before_begin();
				{
					auto e = deferred_traits::end(L, self);
					for (auto it = deferred_traits::begin(L, self); it != e; ++backit, ++it) {
					}
				}
				return add_insert_after(std::true_type(), L, self, value, backit);
			}

			static error_result add_insert(std::true_type, lua_State*, T& self, stack_object value, iterator& pos) {
				self.insert(pos, value.as<V>());
				return {};
			}

			static error_result add_insert(std::true_type, lua_State* L, T& self, stack_object value) {
				auto pos = deferred_traits::end(L, self);
				return add_insert(std::true_type(), L, self, value, pos);
			}

			static error_result add_insert(std::false_type, lua_State* L, T& self, stack_object value, iterator& pos) {
				return add_insert_after(meta::has_insert_after<T>(), L, self, std::move(value), pos);
			}

			static error_result add_insert(std::false_type, lua_State* L, T& self, stack_object value) {
				return add_insert_after(meta::has_insert_after<T>(), L, self, std::move(value));
			}

			static error_result add_push_back(std::true_type, lua_State*, T& self, stack_object value, iterator&) {
				self.push_back(value.as<V>());
				return {};
			}

			static error_result add_push_back(std::true_type, lua_State*, T& self, stack_object value) {
				self.push_back(value.as<V>());
				return {};
			}

			static error_result add_push_back(std::false_type, lua_State* L, T& self, stack_object value, iterator& pos) {
				return add_insert(meta::has_insert<T>(), L, self, value, pos);
			}

			static error_result add_push_back(std::false_type, lua_State* L, T& self, stack_object value) {
				return add_insert(meta::has_insert<T>(), L, self, value);
			}

			static error_result add_associative(std::true_type, lua_State* L, T& self, stack_object key, iterator& pos) {
				self.insert(pos, value_type(key.as<K>(), stack::unqualified_get<V>(L, 3)));
				return {};
			}

			static error_result add_associative(std::true_type, lua_State* L, T& self, stack_object key) {
				auto pos = deferred_traits::end(L, self);
				return add_associative(std::true_type(), L, self, std::move(key), pos);
			}

			static error_result add_associative(std::false_type, lua_State* L, T& self, stack_object value, iterator& pos) {
				return add_push_back(meta::has_push_back<T>(), L, self, value, pos);
			}

			static error_result add_associative(std::false_type, lua_State* L, T& self, stack_object value) {
				return add_push_back(meta::has_push_back<T>(), L, self, value);
			}

			static error_result add_copyable(std::true_type, lua_State* L, T& self, stack_object value, iterator& pos) {
				return add_associative(is_associative(), L, self, std::move(value), pos);
			}

			static error_result add_copyable(std::true_type, lua_State* L, T& self, stack_object value) {
				return add_associative(is_associative(), L, self, value);
			}

			static error_result add_copyable(std::false_type, lua_State* L, T& self, stack_object value, iterator&) {
				return add_copyable(std::false_type(), L, self, std::move(value));
			}

			static error_result add_copyable(std::false_type, lua_State*, T&, stack_object) {
				return error_result("cannot call 'add' on '%s': value_type is non-copyable", detail::demangle<T>().data());
			}

			static error_result insert_lookup(std::true_type, lua_State* L, T& self, stack_object, stack_object value) {
				// TODO: should we warn or error about someone calling insert on an ordered / lookup container with no associativity?
				return add_copyable(std::true_type(), L, self, std::move(value));
			}

			static error_result insert_lookup(std::false_type, lua_State* L, T& self, stack_object where, stack_object value) {
				auto it = deferred_traits::begin(L, self);
				auto key = where.as<K>();
				key += deferred_traits::index_adjustment(L, self);
				std::advance(it, key);
				self.insert(it, value.as<V>());
				return {};
			}

			static error_result insert_after_has(std::true_type, lua_State* L, T& self, stack_object where, stack_object value) {
				auto key = where.as<K>();
				auto backit = self.before_begin();
				{
					key += deferred_traits::index_adjustment(L, self);
					auto e = deferred_traits::end(L, self);
					for (auto it = deferred_traits::begin(L, self); key > 0; ++backit, ++it, --key) {
						if (backit == e) {
							return error_result("sol: out of bounds (too big) for set on '%s'", detail::demangle<T>().c_str());
						}
					}
				}
				self.insert_after(backit, value.as<V>());
				return {};
			}

			static error_result insert_after_has(std::false_type, lua_State*, T&, stack_object, stack_object) {
				return error_result("cannot call 'insert' on '%s': no suitable or similar functionality detected on this container", detail::demangle<T>().data());
			}

			static error_result insert_has(std::true_type, lua_State* L, T& self, stack_object key, stack_object value) {
				return insert_lookup(meta::any<is_associative, is_lookup>(), L, self, std::move(key), std::move(value));
			}

			static error_result insert_has(std::false_type, lua_State* L, T& self, stack_object where, stack_object value) {
				return insert_after_has(meta::has_insert_after<T>(), L, self, where, value);
			}

			static error_result insert_copyable(std::true_type, lua_State* L, T& self, stack_object key, stack_object value) {
				return insert_has(meta::has_insert<T>(), L, self, std::move(key), std::move(value));
			}

			static error_result insert_copyable(std::false_type, lua_State*, T&, stack_object, stack_object) {
				return error_result("cannot call 'insert' on '%s': value_type is non-copyable", detail::demangle<T>().data());
			}

			static error_result erase_integral(std::true_type, lua_State* L, T& self, K& key) {
				auto it = deferred_traits::begin(L, self);
				key += deferred_traits::index_adjustment(L, self);
				std::advance(it, key);
				self.erase(it);

				return {};
			}

			static error_result erase_integral(std::false_type, lua_State* L, T& self, const K& key) {
				auto fx = [&](const value_type& r) -> bool {
					return key == r;
				};
				auto e = deferred_traits::end(L, self);
				auto it = std::find_if(deferred_traits::begin(L, self), e, std::ref(fx));
				if (it == e) {
					return {};
				}
				self.erase(it);

				return {};
			}

			static error_result erase_associative_lookup(std::true_type, lua_State*, T& self, const K& key) {
				self.erase(key);
				return {};
			}

			static error_result erase_associative_lookup(std::false_type, lua_State* L, T& self, K& key) {
				return erase_integral(std::is_integral<K>(), L, self, key);
			}

			static error_result erase_after_has(std::true_type, lua_State* L, T& self, K& key) {
				auto backit = self.before_begin();
				{
					key += deferred_traits::index_adjustment(L, self);
					auto e = deferred_traits::end(L, self);
					for (auto it = deferred_traits::begin(L, self); key > 0; ++backit, ++it, --key) {
						if (backit == e) {
							return error_result("sol: out of bounds for erase on '%s'", detail::demangle<T>().c_str());
						}
					}
				}
				self.erase_after(backit);
				return {};
			}

			static error_result erase_after_has(std::false_type, lua_State*, T&, const K&) {
				return error_result("sol: cannot call erase on '%s'", detail::demangle<T>().c_str());
			}

			static error_result erase_has(std::true_type, lua_State* L, T& self, K& key) {
				return erase_associative_lookup(meta::any<is_associative, is_lookup>(), L, self, key);
			}

			static error_result erase_has(std::false_type, lua_State* L, T& self, K& key) {
				return erase_after_has(has_erase_after<T>(), L, self, key);
			}

			static auto size_has(std::false_type, lua_State* L, T& self) {
				return std::distance(deferred_traits::begin(L, self), deferred_traits::end(L, self));
			}

			static auto size_has(std::true_type, lua_State*, T& self) {
				return self.size();
			}

			static void clear_has(std::true_type, lua_State*, T& self) {
				self.clear();
			}

			static void clear_has(std::false_type, lua_State* L, T&) {
				luaL_error(L, "sol: cannot call clear on '%s'", detail::demangle<T>().c_str());
			}

			static bool empty_has(std::true_type, lua_State*, T& self) {
				return self.empty();
			}

			static bool empty_has(std::false_type, lua_State* L, T& self) {
				return deferred_traits::begin(L, self) == deferred_traits::end(L, self);
			}

			static error_result get_start(lua_State* L, T& self, K& key) {
				return get_it(is_linear_integral(), L, self, key);
			}

			static error_result set_start(lua_State* L, T& self, stack_object key, stack_object value) {
				return set_it(is_linear_integral(), L, self, std::move(key), std::move(value));
			}

			static std::size_t size_start(lua_State* L, T& self) {
				return size_has(meta::has_size<T>(), L, self);
			}

			static void clear_start(lua_State* L, T& self) {
				clear_has(has_clear<T>(), L, self);
			}

			static bool empty_start(lua_State* L, T& self) {
				return empty_has(has_empty<T>(), L, self);
			}

			static error_result erase_start(lua_State* L, T& self, K& key) {
				return erase_has(has_erase<T>(), L, self, key);
			}

			template <bool ip>
			static int next_associative(std::true_type, lua_State* L) {
				iter& i = stack::unqualified_get<user<iter>>(L, 1);
				auto& source = i.source;
				auto& it = i.it;
				if (it == deferred_traits::end(L, source)) {
					return 0;
				}
				int p;
				if (ip) {
					++i.i;
					p = stack::push_reference(L, i.i);
				}
				else {
					p = stack::push_reference(L, it->first);
				}
				p += stack::stack_detail::push_reference<push_type>(L, detail::deref_non_pointer(it->second));
				std::advance(it, 1);
				return p;
			}

			template <bool>
			static int next_associative(std::false_type, lua_State* L) {
				iter& i = stack::unqualified_get<user<iter>>(L, 1);
				auto& source = i.source;
				auto& it = i.it;
				next_K k = stack::unqualified_get<next_K>(L, 2);
				if (it == deferred_traits::end(L, source)) {
					return 0;
				}
				int p;
				p = stack::push_reference(L, k + 1);
				p += stack::stack_detail::push_reference<push_type>(L, detail::deref_non_pointer(*it));
				std::advance(it, 1);
				return p;
			}

			template <bool ip>
			static int next_iter(lua_State* L) {
				typedef meta::any<is_associative, meta::all<is_lookup, meta::neg<is_matched_lookup>>> is_assoc;
				return next_associative<ip>(is_assoc(), L);
			}

			template <bool ip>
			static int pairs_associative(std::true_type, lua_State* L) {
				auto& src = get_src(L);
				stack::push(L, next_iter<ip>);
				stack::push<user<iter>>(L, src, deferred_traits::begin(L, src));
				stack::push(L, lua_nil);
				return 3;
			}

			template <bool ip>
			static int pairs_associative(std::false_type, lua_State* L) {
				auto& src = get_src(L);
				stack::push(L, next_iter<ip>);
				stack::push<user<iter>>(L, src, deferred_traits::begin(L, src));
				stack::push(L, 0);
				return 3;
			}

		public:
			static int at(lua_State* L) {
				auto& self = get_src(L);
				error_result er;
				{
					std::ptrdiff_t pos = stack::unqualified_get<std::ptrdiff_t>(L);
					er = at_start(L, self, pos);
				}
				return handle_errors(L, er);
			}

			static int get(lua_State* L) {
				auto& self = get_src(L);
				error_result er;
				{
					decltype(auto) key = stack::unqualified_get<K>(L);
					er = get_start(L, self, key);
				}
				return handle_errors(L, er);
			}

			static int index_get(lua_State* L) {
				return get(L);
			}

			static int set(lua_State* L) {
				stack_object value = stack_object(L, raw_index(3));
				if (type_of(L, 3) == type::lua_nil) {
					return erase(L);
				}
				auto& self = get_src(L);
				error_result er = set_start(L, self, stack_object(L, raw_index(2)), std::move(value));
				return handle_errors(L, er);
			}

			static int index_set(lua_State* L) {
				return set(L);
			}

			static int add(lua_State* L) {
				auto& self = get_src(L);
				error_result er = add_copyable(is_copyable(), L, self, stack_object(L, raw_index(2)));
				return handle_errors(L, er);
			}

			static int insert(lua_State* L) {
				auto& self = get_src(L);
				error_result er = insert_copyable(is_copyable(), L, self, stack_object(L, raw_index(2)), stack_object(L, raw_index(3)));
				return handle_errors(L, er);
			}

			static int find(lua_State* L) {
				auto& self = get_src(L);
				error_result er = find_has(has_find<T>(), L, self);
				return handle_errors(L, er);
			}

			static iterator begin(lua_State*, T& self) {
				using std::begin;
				return begin(self);
			}

			static iterator end(lua_State*, T& self) {
				using std::end;
				return end(self);
			}

			static int size(lua_State* L) {
				auto& self = get_src(L);
				std::size_t r = size_start(L, self);
				return stack::push(L, r);
			}

			static int clear(lua_State* L) {
				auto& self = get_src(L);
				clear_start(L, self);
				return 0;
			}

			static int erase(lua_State* L) {
				auto& self = get_src(L);
				error_result er;
				{
					decltype(auto) key = stack::unqualified_get<K>(L, 2);
					er = erase_start(L, self, key);
				}
				return handle_errors(L, er);
			}

			static int empty(lua_State* L) {
				auto& self = get_src(L);
				return stack::push(L, empty_start(L, self));
			}

			static std::ptrdiff_t index_adjustment(lua_State*, T&) {
#if defined(SOL_CONTAINERS_START_INDEX)
				return static_cast<std::ptrdiff_t>((SOL_CONTAINERS_START) == 0 ? 0 : -(SOL_CONTAINERS_START));
#else
				return static_cast<std::ptrdiff_t>(-1);
#endif
			}

			static int pairs(lua_State* L) {
				typedef meta::any<is_associative, meta::all<is_lookup, meta::neg<is_matched_lookup>>> is_assoc;
				return pairs_associative<false>(is_assoc(), L);
			}

			static int ipairs(lua_State* L) {
				typedef meta::any<is_associative, meta::all<is_lookup, meta::neg<is_matched_lookup>>> is_assoc;
				return pairs_associative<true>(is_assoc(), L);
			}

			static int next(lua_State* L) {
				return stack::push(L, next_iter<false>);
			}
		};

		template <typename X>
		struct container_traits_default<X, std::enable_if_t<std::is_array<std::remove_pointer_t<meta::unwrap_unqualified_t<X>>>::value>> {
		private:
			typedef std::remove_pointer_t<meta::unwrap_unqualified_t<X>> T;
			typedef container_traits<X> deferred_traits;

		public:
			typedef std::remove_extent_t<T> value_type;
			typedef value_type* iterator;

		private:
			struct iter {
				T& source;
				iterator it;

				iter(T& source, iterator it)
				: source(source), it(std::move(it)) {
				}
			};

			static auto& get_src(lua_State* L) {
				auto p = stack::unqualified_check_get<T*>(L, 1);
#if defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				if (!p) {
					luaL_error(L, "sol: 'self' is not of type '%s' (pass 'self' as first argument with ':' or call on proper type)", detail::demangle<T>().c_str());
				}
				if (p.value() == nullptr) {
					luaL_error(L, "sol: 'self' argument is nil (pass 'self' as first argument with ':' or call on a '%s' type)", detail::demangle<T>().c_str());
				}
#endif // Safe getting with error
				return *p.value();
			}

			static int find(std::true_type, lua_State* L) {
				T& self = get_src(L);
				decltype(auto) value = stack::unqualified_get<value_type>(L, 2);
				std::size_t N = std::extent<T>::value;
				for (std::size_t idx = 0; idx < N; ++idx) {
					const auto& v = self[idx];
					if (v == value) {
						return stack::push(L, idx + 1);
					}
				}
				return stack::push(L, lua_nil);
			}

			static int find(std::false_type, lua_State* L) {
				return luaL_error(L, "sol: cannot call 'find' on '%s': no supported comparison operator for the value type", detail::demangle<T>().c_str());
			}

			static int next_iter(lua_State* L) {
				iter& i = stack::unqualified_get<user<iter>>(L, 1);
				auto& source = i.source;
				auto& it = i.it;
				std::size_t k = stack::unqualified_get<std::size_t>(L, 2);
				if (it == deferred_traits::end(L, source)) {
					return 0;
				}
				int p;
				p = stack::push_reference(L, k + 1);
				p += stack::push_reference(L, detail::deref_non_pointer(*it));
				std::advance(it, 1);
				return p;
			}

		public:
			static int clear(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'clear' on type '%s': cannot remove all items from a fixed array", detail::demangle<T>().c_str());
			}

			static int erase(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'erase' on type '%s': cannot remove an item from fixed arrays", detail::demangle<T>().c_str());
			}

			static int add(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'add' on type '%s': cannot add to fixed arrays", detail::demangle<T>().c_str());
			}

			static int insert(lua_State* L) {
				return luaL_error(L, "sol: cannot call 'insert' on type '%s': cannot insert new entries into fixed arrays", detail::demangle<T>().c_str());
			}

			static int at(lua_State* L) {
				return get(L);
			}

			static int get(lua_State* L) {
				T& self = get_src(L);
				std::ptrdiff_t idx = stack::unqualified_get<std::ptrdiff_t>(L, 2);
				idx += deferred_traits::index_adjustment(L, self);
				if (idx >= static_cast<std::ptrdiff_t>(std::extent<T>::value) || idx < 0) {
					return stack::push(L, lua_nil);
				}
				return stack::push_reference(L, detail::deref_non_pointer(self[idx]));
			}

			static int index_get(lua_State* L) {
				return get(L);
			}

			static int set(lua_State* L) {
				T& self = get_src(L);
				std::ptrdiff_t idx = stack::unqualified_get<std::ptrdiff_t>(L, 2);
				idx += deferred_traits::index_adjustment(L, self);
				if (idx >= static_cast<std::ptrdiff_t>(std::extent<T>::value)) {
					return luaL_error(L, "sol: index out of bounds (too big) for set on '%s'", detail::demangle<T>().c_str());
				}
				if (idx < 0) {
					return luaL_error(L, "sol: index out of bounds (too small) for set on '%s'", detail::demangle<T>().c_str());
				}
				self[idx] = stack::unqualified_get<value_type>(L, 3);
				return 0;
			}

			static int index_set(lua_State* L) {
				return set(L);
			}

			static int find(lua_State* L) {
				return find(meta::supports_op_equal<value_type, value_type>(), L);
			}

			static int size(lua_State* L) {
				return stack::push(L, std::extent<T>::value);
			}

			static int empty(lua_State* L) {
				return stack::push(L, std::extent<T>::value > 0);
			}

			static int pairs(lua_State* L) {
				auto& src = get_src(L);
				stack::push(L, next_iter);
				stack::push<user<iter>>(L, src, deferred_traits::begin(L, src));
				stack::push(L, 0);
				return 3;
			}

			static int ipairs(lua_State* L) {
				return pairs(L);
			}

			static int next(lua_State* L) {
				return stack::push(L, next_iter);
			}

			static std::ptrdiff_t index_adjustment(lua_State*, T&) {
#if defined(SOL_CONTAINERS_START_INDEX)
				return (SOL_CONTAINERS_START) == 0 ? 0 : -(SOL_CONTAINERS_START);
#else
				return -1;
#endif
			}

			static iterator begin(lua_State*, T& self) {
				return std::addressof(self[0]);
			}

			static iterator end(lua_State*, T& self) {
				return std::addressof(self[0]) + std::extent<T>::value;
			}
		};

		template <typename X>
		struct container_traits_default<container_traits<X>> : container_traits_default<X> {};
	} // namespace container_detail

	template <typename T>
	struct container_traits : container_detail::container_traits_default<T> {};

} // namespace sol

// end of sol/container_traits.hpp

namespace sol {

	template <typename X>
	struct container_usertype_metatable {
		typedef std::remove_pointer_t<meta::unqualified_t<X>> T;
		typedef container_traits<T> traits;
		typedef container_detail::container_traits_default<T> default_traits;

		static int real_index_get_traits(std::true_type, lua_State* L) {
			return traits::index_get(L);
		}

		static int real_index_get_traits(std::false_type, lua_State* L) {
			return default_traits::index_get(L);
		}

		static int real_index_call(lua_State* L) {
			typedef usertype_detail::map_t<std::string, lua_CFunction> call_map;
			static const call_map calls{
				{ "at", &at_call },
				{ "get", &real_get_call },
				{ "set", &real_set_call },
				{ "size", &real_length_call },
				{ "add", &real_add_call },
				{ "empty", &real_empty_call },
				{ "insert", &real_insert_call },
				{ "clear", &real_clear_call },
				{ "find", &real_find_call },
				{ "erase", &real_erase_call },
				{ "pairs", &pairs_call },
				{ "next", &next_call },
			};
			auto maybenameview = stack::unqualified_check_get<string_view>(L, 2);
			if (maybenameview) {
				const string_view& nameview = *maybenameview;
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				auto it = calls.find(nameview, string_view_hash(), std::equal_to<string_view>());
#else
				std::string name(nameview.data(), nameview.size());
				auto it = calls.find(name);
#endif
				if (it != calls.cend()) {
					return stack::push(L, it->second);
				}
			}
			return real_index_get_traits(container_detail::has_traits_index_get<traits>(), L);
		}

		static int real_at_traits(std::true_type, lua_State* L) {
			return traits::at(L);
		}

		static int real_at_traits(std::false_type, lua_State* L) {
			return default_traits::at(L);
		}

		static int real_at_call(lua_State* L) {
			return real_at_traits(container_detail::has_traits_at<traits>(), L);
		}

		static int real_get_traits(std::true_type, lua_State* L) {
			return traits::get(L);
		}

		static int real_get_traits(std::false_type, lua_State* L) {
			return default_traits::get(L);
		}

		static int real_get_call(lua_State* L) {
			return real_get_traits(container_detail::has_traits_get<traits>(), L);
		}

		static int real_set_traits(std::true_type, lua_State* L) {
			return traits::set(L);
		}

		static int real_set_traits(std::false_type, lua_State* L) {
			return default_traits::set(L);
		}

		static int real_set_call(lua_State* L) {
			return real_set_traits(container_detail::has_traits_set<traits>(), L);
		}

		static int real_index_set_traits(std::true_type, lua_State* L) {
			return traits::index_set(L);
		}

		static int real_index_set_traits(std::false_type, lua_State* L) {
			return default_traits::index_set(L);
		}

		static int real_new_index_call(lua_State* L) {
			return real_index_set_traits(container_detail::has_traits_index_set<traits>(), L);
		}

		static int real_pairs_traits(std::true_type, lua_State* L) {
			return traits::pairs(L);
		}

		static int real_pairs_traits(std::false_type, lua_State* L) {
			return default_traits::pairs(L);
		}

		static int real_pairs_call(lua_State* L) {
			return real_pairs_traits(container_detail::has_traits_pairs<traits>(), L);
		}

		static int real_ipairs_traits(std::true_type, lua_State* L) {
			return traits::ipairs(L);
		}

		static int real_ipairs_traits(std::false_type, lua_State* L) {
			return default_traits::ipairs(L);
		}

		static int real_ipairs_call(lua_State* L) {
			return real_ipairs_traits(container_detail::has_traits_ipairs<traits>(), L);
		}

		static int real_next_traits(std::true_type, lua_State* L) {
			return traits::next(L);
		}

		static int real_next_traits(std::false_type, lua_State* L) {
			return default_traits::next(L);
		}

		static int real_next_call(lua_State* L) {
			return real_next_traits(container_detail::has_traits_next<traits>(), L);
		}

		static int real_size_traits(std::true_type, lua_State* L) {
			return traits::size(L);
		}

		static int real_size_traits(std::false_type, lua_State* L) {
			return default_traits::size(L);
		}

		static int real_length_call(lua_State* L) {
			return real_size_traits(container_detail::has_traits_size<traits>(), L);
		}

		static int real_add_traits(std::true_type, lua_State* L) {
			return traits::add(L);
		}

		static int real_add_traits(std::false_type, lua_State* L) {
			return default_traits::add(L);
		}

		static int real_add_call(lua_State* L) {
			return real_add_traits(container_detail::has_traits_add<traits>(), L);
		}

		static int real_insert_traits(std::true_type, lua_State* L) {
			return traits::insert(L);
		}

		static int real_insert_traits(std::false_type, lua_State* L) {
			return default_traits::insert(L);
		}

		static int real_insert_call(lua_State* L) {
			return real_insert_traits(container_detail::has_traits_insert<traits>(), L);
		}

		static int real_clear_traits(std::true_type, lua_State* L) {
			return traits::clear(L);
		}

		static int real_clear_traits(std::false_type, lua_State* L) {
			return default_traits::clear(L);
		}

		static int real_clear_call(lua_State* L) {
			return real_clear_traits(container_detail::has_traits_clear<traits>(), L);
		}

		static int real_empty_traits(std::true_type, lua_State* L) {
			return traits::empty(L);
		}

		static int real_empty_traits(std::false_type, lua_State* L) {
			return default_traits::empty(L);
		}

		static int real_empty_call(lua_State* L) {
			return real_empty_traits(container_detail::has_traits_empty<traits>(), L);
		}

		static int real_erase_traits(std::true_type, lua_State* L) {
			return traits::erase(L);
		}

		static int real_erase_traits(std::false_type, lua_State* L) {
			return default_traits::erase(L);
		}

		static int real_erase_call(lua_State* L) {
			return real_erase_traits(container_detail::has_traits_erase<traits>(), L);
		}

		static int real_find_traits(std::true_type, lua_State* L) {
			return traits::find(L);
		}

		static int real_find_traits(std::false_type, lua_State* L) {
			return default_traits::find(L);
		}

		static int real_find_call(lua_State* L) {
			return real_find_traits(container_detail::has_traits_find<traits>(), L);
		}

		static int add_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_add_call), (&real_add_call)>(L);
		}

		static int erase_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_erase_call), (&real_erase_call)>(L);
		}

		static int insert_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_insert_call), (&real_insert_call)>(L);
		}

		static int clear_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_clear_call), (&real_clear_call)>(L);
		}

		static int empty_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_empty_call), (&real_empty_call)>(L);
		}

		static int find_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_find_call), (&real_find_call)>(L);
		}

		static int length_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_length_call), (&real_length_call)>(L);
		}

		static int pairs_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_pairs_call), (&real_pairs_call)>(L);
		}

		static int ipairs_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_ipairs_call), (&real_ipairs_call)>(L);
		}

		static int next_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_next_call), (&real_next_call)>(L);
		}

		static int at_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_at_call), (&real_at_call)>(L);
		}

		static int get_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_get_call), (&real_get_call)>(L);
		}

		static int set_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_set_call), (&real_set_call)>(L);
		}

		static int index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_index_call), (&real_index_call)>(L);
		}

		static int new_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_new_index_call), (&real_new_index_call)>(L);
		}
	};

	namespace stack {
		namespace stack_detail {
			template <typename T, bool is_shim = false>
			struct metatable_setup {
				lua_State* L;

				metatable_setup(lua_State* L)
				: L(L) {
				}

				void operator()() {
					typedef container_usertype_metatable<std::conditional_t<is_shim,
						as_container_t<std::remove_pointer_t<T>>,
						std::remove_pointer_t<T>>>
						meta_cumt;
					static const char* metakey = is_shim ? &usertype_traits<as_container_t<std::remove_pointer_t<T>>>::metatable()[0] : &usertype_traits<T>::metatable()[0];
					static const std::array<luaL_Reg, 19> reg = { { 
						{ "__pairs", &meta_cumt::pairs_call },
						{ "__ipairs", &meta_cumt::ipairs_call },
						{ "__len", &meta_cumt::length_call },
						{ "__index", &meta_cumt::index_call },
						{ "__newindex", &meta_cumt::new_index_call },
						{ "pairs", &meta_cumt::pairs_call },
						{ "next", &meta_cumt::next_call },
						{ "at", &meta_cumt::at_call },
						{ "get", &meta_cumt::get_call },
						{ "set", &meta_cumt::set_call },
						{ "size", &meta_cumt::length_call },
						{ "empty", &meta_cumt::empty_call },
						{ "clear", &meta_cumt::clear_call },
						{ "insert", &meta_cumt::insert_call },
						{ "add", &meta_cumt::add_call },
						{ "find", &meta_cumt::find_call },
						{ "erase", &meta_cumt::erase_call },
						std::is_pointer<T>::value ? luaL_Reg{ nullptr, nullptr } : luaL_Reg{ "__gc", &detail::usertype_alloc_destruct<T> },
						{ nullptr, nullptr } 
					} };

					if (luaL_newmetatable(L, metakey) == 1) {
						luaL_setfuncs(L, reg.data(), 0);
					}
					lua_setmetatable(L, -2);
				}
			};
		} // namespace stack_detail

		template <typename T>
		struct pusher<as_container_t<T>> {
			typedef meta::unqualified_t<T> C;

			static int push_lvalue(std::true_type, lua_State* L, const C& cont) {
				stack_detail::metatable_setup<C*, true> fx(L);
				return pusher<detail::as_pointer_tag<const C>>{}.push_fx(L, fx, detail::ptr(cont));
			}

			static int push_lvalue(std::false_type, lua_State* L, const C& cont) {
				stack_detail::metatable_setup<C, true> fx(L);
				return pusher<detail::as_value_tag<C>>{}.push_fx(L, fx, cont);
			}

			static int push_rvalue(std::true_type, lua_State* L, C&& cont) {
				stack_detail::metatable_setup<C, true> fx(L);
				return pusher<detail::as_value_tag<C>>{}.push_fx(L, fx, std::move(cont));
			}

			static int push_rvalue(std::false_type, lua_State* L, const C& cont) {
				return push_lvalue(std::is_lvalue_reference<T>(), L, cont);
			}

			static int push(lua_State* L, const as_container_t<T>& as_cont) {
				return push_lvalue(std::is_lvalue_reference<T>(), L, as_cont.source);
			}

			static int push(lua_State* L, as_container_t<T>&& as_cont) {
				return push_rvalue(meta::all<std::is_rvalue_reference<T>, meta::neg<std::is_lvalue_reference<T>>>(), L, std::forward<T>(as_cont.source));
			}
		};

		template <typename T>
		struct pusher<as_container_t<T*>> {
			typedef std::add_pointer_t<meta::unqualified_t<std::remove_pointer_t<T>>> C;

			static int push(lua_State* L, T* cont) {
				stack_detail::metatable_setup<C> fx(L);
				return pusher<detail::as_pointer_tag<T>>{}.push_fx(L, fx, cont);
			}
		};

		template <typename T>
		struct pusher<T, std::enable_if_t<meta::all<is_container<meta::unqualified_t<T>>, meta::neg<is_lua_reference<meta::unqualified_t<T>>>>::value>> {
			typedef meta::unqualified_t<T> C;

			static int push(lua_State* L, const T& cont) {
				stack_detail::metatable_setup<C> fx(L);
				return pusher<detail::as_value_tag<T>>{}.push_fx(L, fx, cont);
			}

			static int push(lua_State* L, T&& cont) {
				stack_detail::metatable_setup<C> fx(L);
				return pusher<detail::as_value_tag<T>>{}.push_fx(L, fx, std::move(cont));
			}
		};

		template <typename T>
		struct pusher<T*, std::enable_if_t<meta::all<is_container<meta::unqualified_t<T>>, meta::neg<is_lua_reference<meta::unqualified_t<T>>>>::value>> {
			typedef std::add_pointer_t<meta::unqualified_t<std::remove_pointer_t<T>>> C;

			static int push(lua_State* L, T* cont) {
				stack_detail::metatable_setup<C> fx(L);
				return pusher<detail::as_pointer_tag<T>>{}.push_fx(L, fx, cont);
			}
		};

		template <typename T, typename C>
		struct checker<as_container_t<T>, type::userdata, C> {
			template <typename Handler>
			static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
				return stack::check<T>(L, index, std::forward<Handler>(handler), tracking);
			}
		};

		template <typename T>
		struct getter<as_container_t<T>> {
			static decltype(auto) get(lua_State* L, int index, record& tracking) {
				return stack::unqualified_get<T>(L, index, tracking);
			}
		};

		template <typename T>
		struct getter<as_container_t<T>*> {
			static decltype(auto) get(lua_State* L, int index, record& tracking) {
				return stack::unqualified_get<T*>(L, index, tracking);
			}
		};
	} // namespace stack

} // namespace sol

// end of sol/container_usertype_metatable.hpp

// beginning of sol/usertype_core.hpp

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

// end of sol/usertype_core.hpp

#include <cstdio>
#include <bitset>

namespace sol {

	struct usertype_metatable_core;

	namespace usertype_detail {
		const int metatable_index = 2;
		const int metatable_core_index = 3;
		const int filler_index = 4;
		const int magic_index = 5;

		const int simple_metatable_index = 2;
		const int index_function_index = 3;
		const int newindex_function_index = 4;

		typedef void (*base_walk)(lua_State*, bool&, int&, string_view&);
		typedef int (*member_search)(lua_State*, void*, usertype_metatable_core&, int);

		struct call_information {
			member_search index;
			member_search new_index;
			int runtime_target;

			call_information(member_search index, member_search newindex)
			: call_information(index, newindex, -1) {
			}
			call_information(member_search index, member_search newindex, int runtimetarget)
			: index(index), new_index(newindex), runtime_target(runtimetarget) {
			}
		};
		
		typedef map_t<std::string, call_information> mapping_t;

		struct variable_wrapper {
			virtual int index(lua_State* L) = 0;
			virtual int new_index(lua_State* L) = 0;
			virtual ~variable_wrapper(){};
		};

		template <typename T, typename F>
		struct callable_binding : variable_wrapper {
			F fx;

			template <typename Arg>
			callable_binding(Arg&& arg)
			: fx(std::forward<Arg>(arg)) {
			}

			virtual int index(lua_State* L) override {
				return call_detail::call_wrapped<T, true, true>(L, fx);
			}

			virtual int new_index(lua_State* L) override {
				return call_detail::call_wrapped<T, false, true>(L, fx);
			}
		};

		typedef map_t<std::string, std::unique_ptr<variable_wrapper>> variable_map;
		typedef map_t<std::string, object> function_map;

		struct simple_map {
			const char* metakey;
			variable_map variables;
			function_map functions;
			object index;
			object newindex;
			base_walk indexbaseclasspropogation;
			base_walk newindexbaseclasspropogation;

			simple_map(const char* mkey, base_walk index, base_walk newindex, object i, object ni, variable_map&& vars, function_map&& funcs)
			: metakey(mkey), variables(std::move(vars)), functions(std::move(funcs)), index(std::move(i)), newindex(std::move(ni)), indexbaseclasspropogation(index), newindexbaseclasspropogation(newindex) {
			}
		};
	} // namespace usertype_detail

	struct usertype_metatable_core {
		usertype_detail::mapping_t mapping;
		lua_CFunction indexfunc;
		lua_CFunction newindexfunc;
		std::vector<object> runtime;
		bool mustindex;

		usertype_metatable_core(lua_CFunction ifx, lua_CFunction nifx)
		: mapping(), indexfunc(ifx), newindexfunc(nifx), runtime(), mustindex(false) {
		}

		usertype_metatable_core(const usertype_metatable_core&) = default;
		usertype_metatable_core(usertype_metatable_core&&) = default;
		usertype_metatable_core& operator=(const usertype_metatable_core&) = default;
		usertype_metatable_core& operator=(usertype_metatable_core&&) = default;
	};

	namespace usertype_detail {
		const lua_Integer toplevel_magic = static_cast<lua_Integer>(0xCCC2CCC1);

		inline int is_indexer(string_view s) {
			if (s == to_string(meta_function::index)) {
				return 1;
			}
			else if (s == to_string(meta_function::new_index)) {
				return 2;
			}
			return 0;
		}

		inline int is_indexer(meta_function mf) {
			if (mf == meta_function::index) {
				return 1;
			}
			else if (mf == meta_function::new_index) {
				return 2;
			}
			return 0;
		}

		inline int is_indexer(call_construction) {
			return 0;
		}

		inline int is_indexer(base_classes_tag) {
			return 0;
		}

		inline auto make_string_view(string_view s) {
			return s;
		}

		inline auto make_string_view(call_construction) {
			return string_view(to_string(meta_function::call_function));
		}

		inline auto make_string_view(meta_function mf) {
			return string_view(to_string(mf));
		}

		inline auto make_string_view(base_classes_tag) {
			return string_view(detail::base_class_cast_key());
		}

		template <typename Arg>
		inline std::string make_string(Arg&& arg) {
			string_view s = make_string_view(arg);
			return std::string(s.data(), s.size());
		}

		template <typename N>
		inline luaL_Reg make_reg(N&& n, lua_CFunction f) {
			luaL_Reg l{make_string_view(std::forward<N>(n)).data(), f};
			return l;
		}

		struct registrar {
			registrar() = default;
			registrar(const registrar&) = default;
			registrar(registrar&&) = default;
			registrar& operator=(const registrar&) = default;
			registrar& operator=(registrar&&) = default;
			virtual int push_um(lua_State* L) = 0;
			virtual ~registrar() {
			}
		};

		inline bool is_toplevel(lua_State* L, int index = magic_index) {
			int isnum = 0;
			lua_Integer magic = lua_tointegerx(L, upvalue_index(index), &isnum);
			return isnum != 0 && magic == toplevel_magic;
		}

		inline int runtime_object_call(lua_State* L, void*, usertype_metatable_core& umc, int runtimetarget) {
			std::vector<object>& runtime = umc.runtime;
			object& runtimeobj = runtime[runtimetarget];
			return stack::push(L, runtimeobj);
		}

		template <typename T, bool is_index>
		inline int indexing_fail(lua_State* L) {
			if (is_index) {
#if 0 //defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeaccessor = stack::get<optional<string_view>>(L, is_index ? -1 : -2);
				string_view accessor = maybeaccessor.value_or(string_detail::string_shim("(unknown)"));
				return luaL_error(L, "sol: attempt to index (get) nil value \"%s\" on userdata (bad (misspelled?) key name or does not exist)", accessor.data());
#else
				if (is_toplevel(L)) {
					if (lua_getmetatable(L, 1) == 1) {
						int metatarget = lua_gettop(L);
						stack::get_field(L, stack_reference(L, raw_index(2)), metatarget);
						return 1;
					}
				}
				// With runtime extensibility, we can't hard-error things. They have to return nil, like regular table types, unfortunately...
				return stack::push(L, lua_nil);
#endif
			}
			else {
				auto maybeaccessor = stack::get<optional<string_view>>(L, is_index ? -1 : -2);
				string_view accessor = maybeaccessor.value_or(string_view("(unknown)"));
				return luaL_error(L, "sol: attempt to index (set) nil value \"%s\" on userdata (bad (misspelled?) key name or does not exist)", accessor.data());
			}
		}

		int runtime_new_index(lua_State* L, void*, usertype_metatable_core&, int runtimetarget);

		template <typename T, bool is_simple>
		inline int metatable_new_index(lua_State* L) {
			if (is_toplevel(L)) {
				auto non_indexable = [&L]() {
					if (is_simple) {
						simple_map& sm = stack::get<user<simple_map>>(L, upvalue_index(simple_metatable_index));
						function_map& functions = sm.functions;
						optional<string_view> maybeaccessor = stack::get<optional<string_view>>(L, 2);
						if (!maybeaccessor) {
							return;
						}
						string_view& accessor_view = maybeaccessor.value();
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
						auto preexistingit = functions.find(accessor_view, string_view_hash(), std::equal_to<string_view>());
#else
						std::string accessor(accessor_view.data(), accessor_view.size());
						auto preexistingit = functions.find(accessor);
#endif
						if (preexistingit == functions.cend()) {
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
							std::string accessor(accessor_view.data(), accessor_view.size());
#endif
							functions.emplace_hint(preexistingit, std::move(accessor), object(L, 3));
						}
						else {
							preexistingit->second = object(L, 3);
						}
						return;
					}
					usertype_metatable_core& umc = stack::get<light<usertype_metatable_core>>(L, upvalue_index(metatable_core_index));
					bool mustindex = umc.mustindex;
					if (!mustindex)
						return;
					optional<string_view> maybeaccessor = stack::get<optional<string_view>>(L, 2);
					if (!maybeaccessor) {
						return;
					}
					string_view& accessor_view = maybeaccessor.value();
					mapping_t& mapping = umc.mapping;
					std::vector<object>& runtime = umc.runtime;
					int target = static_cast<int>(runtime.size());
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
					auto preexistingit = mapping.find(accessor_view, string_view_hash(), std::equal_to<string_view>());
#else
					std::string accessor(accessor_view.data(), accessor_view.size());
					auto preexistingit = mapping.find(accessor);
#endif
					if (preexistingit == mapping.cend()) {
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
						std::string accessor(accessor_view.data(), accessor_view.size());
#endif
						runtime.emplace_back(L, 3);
						mapping.emplace_hint(mapping.cend(), std::move(accessor), call_information(&runtime_object_call, &runtime_new_index, target));
					}
					else {
						target = preexistingit->second.runtime_target;
						runtime[target] = object(L, 3);
						preexistingit->second = call_information(&runtime_object_call, &runtime_new_index, target);
					}
				};
				non_indexable();
				for (std::size_t i = 0; i < 4; lua_settop(L, 3), ++i) {
					const char* metakey = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 2:
						metakey = &usertype_traits<T>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 3:
					default:
						metakey = &usertype_traits<T>::user_metatable()[0];
						{
							luaL_getmetatable(L, metakey);
							lua_getmetatable(L, -1);
						}
						break;
					}
					int tableindex = lua_gettop(L);
					if (type_of(L, tableindex) == type::lua_nil) {
						continue;
					}
					stack::set_field<false, true>(L, stack_reference(L, raw_index(2)), stack_reference(L, raw_index(3)), tableindex);
				}
				lua_settop(L, 0);
				return 0;
			}
			return indexing_fail<T, false>(L);
		}

		inline int runtime_new_index(lua_State* L, void*, usertype_metatable_core& umc, int runtimetarget) {
			std::vector<object>& runtime = umc.runtime;
			object& runtimeobj = runtime[runtimetarget];
			runtimeobj = object(L, 3);
			return 0;
		}

		template <bool is_index, typename Base>
		static void walk_single_base(lua_State* L, bool& found, int& ret, string_view&) {
			if (found)
				return;
			const char* metakey = &usertype_traits<Base>::metatable()[0];
			const char* gcmetakey = &usertype_traits<Base>::gc_table()[0];
			const char* basewalkkey = is_index ? detail::base_class_index_propogation_key() : detail::base_class_new_index_propogation_key();

			luaL_getmetatable(L, metakey);
			if (type_of(L, -1) == type::lua_nil) {
				lua_pop(L, 1);
				return;
			}

			stack::get_field(L, basewalkkey);
			if (type_of(L, -1) == type::lua_nil) {
				lua_pop(L, 2);
				return;
			}
			lua_CFunction basewalkfunc = stack::pop<lua_CFunction>(L);
			lua_pop(L, 1);

			stack::get_field<true>(L, gcmetakey);
			int value = basewalkfunc(L);
			if (value > -1) {
				found = true;
				ret = value;
			}
		}

		template <bool is_index, typename... Bases>
		static void walk_all_bases(lua_State* L, bool& found, int& ret, string_view& accessor) {
			(void)L;
			(void)found;
			(void)ret;
			(void)accessor;
			(void)detail::swallow{0, (walk_single_base<is_index, Bases>(L, found, ret, accessor), 0)...};
		}
	} // namespace usertype_detail

	template <typename T>
	struct clean_type {
		typedef std::conditional_t<std::is_array<meta::unqualified_t<T>>::value, T&, std::decay_t<T>> type;
	};

	template <typename T>
	using clean_type_t = typename clean_type<T>::type;

	template <typename T, typename IndexSequence, typename... Tn>
	struct usertype_metatable : usertype_detail::registrar {};

	template <typename T, std::size_t... I, typename... Tn>
	struct usertype_metatable<T, std::index_sequence<I...>, Tn...> : usertype_metatable_core, usertype_detail::registrar {
		typedef std::make_index_sequence<sizeof...(I) * 2> indices;
		typedef std::index_sequence<I...> half_indices;
		typedef std::array<luaL_Reg, sizeof...(Tn) / 2 + 1 + 31> regs_t;
		typedef std::tuple<Tn...> RawTuple;
		typedef std::tuple<clean_type_t<Tn>...> Tuple;
		template <std::size_t Idx>
		struct check_binding : is_variable_binding<meta::unqualified_tuple_element_t<Idx, Tuple>> {};
		Tuple functions;
		lua_CFunction destructfunc;
		lua_CFunction callconstructfunc;
		lua_CFunction indexbase;
		lua_CFunction newindexbase;
		usertype_detail::base_walk indexbaseclasspropogation;
		usertype_detail::base_walk newindexbaseclasspropogation;
		void* baseclasscheck;
		void* baseclasscast;
		bool secondarymeta;
		std::bitset<32> properties;

		template <std::size_t Idx, meta::enable<std::is_same<lua_CFunction, meta::unqualified_tuple_element<Idx + 1, RawTuple>>> = meta::enabler>
		lua_CFunction make_func() const {
			return std::get<Idx + 1>(functions);
		}

		template <std::size_t Idx, meta::disable<std::is_same<lua_CFunction, meta::unqualified_tuple_element<Idx + 1, RawTuple>>> = meta::enabler>
		lua_CFunction make_func() const {
			const auto& name = std::get<Idx>(functions);
			return (usertype_detail::make_string_view(name) == "__newindex") ? &call<Idx + 1, false> : &call<Idx + 1, true>;
		}

		static bool contains_variable() {
			typedef meta::any<check_binding<(I * 2 + 1)>...> has_variables;
			return has_variables::value;
		}

		bool contains_index() const {
			bool idx = false;
			(void)detail::swallow{0, ((idx |= (usertype_detail::is_indexer(std::get<I * 2>(functions)) != 0)), 0)...};
			return idx;
		}

		int finish_regs(regs_t& l, int& index) {
			auto prop_fx = [&](meta_function mf) { return !properties[static_cast<int>(mf)]; };
			usertype_detail::insert_default_registrations<T>(l, index, prop_fx);
			if (destructfunc != nullptr) {
				l[index] = luaL_Reg{to_string(meta_function::garbage_collect).c_str(), destructfunc};
				++index;
			}
			return index;
		}

		template <std::size_t Idx, typename F>
		void make_regs(regs_t&, int&, call_construction, F&&) {
			callconstructfunc = call<Idx + 1>;
			secondarymeta = true;
		}

		template <std::size_t, typename... Bases>
		void make_regs(regs_t&, int&, base_classes_tag, bases<Bases...>) {
			static_assert(!meta::any_same<T, Bases...>::value, "base classes cannot list the original class as part of the bases");
			if (sizeof...(Bases) < 1) {
				return;
			}
			mustindex = true;
			(void)detail::swallow{0, ((detail::has_derived<Bases>::value = true), 0)...};

			static_assert(sizeof(void*) <= sizeof(detail::inheritance_check_function), "The size of this data pointer is too small to fit the inheritance checking function: file a bug report.");
			static_assert(sizeof(void*) <= sizeof(detail::inheritance_cast_function), "The size of this data pointer is too small to fit the inheritance checking function: file a bug report.");
			baseclasscheck = (void*)&detail::inheritance<T, Bases...>::type_check;
			baseclasscast = (void*)&detail::inheritance<T, Bases...>::type_cast;
			indexbaseclasspropogation = usertype_detail::walk_all_bases<true, Bases...>;
			newindexbaseclasspropogation = usertype_detail::walk_all_bases<false, Bases...>;
		}

		template <std::size_t Idx, typename N, typename F, typename = std::enable_if_t<!meta::any_same<meta::unqualified_t<N>, base_classes_tag, call_construction>::value>>
		void make_regs(regs_t& l, int& index, N&& n, F&&) {
			if (is_variable_binding<meta::unqualified_t<F>>::value) {
				return;
			}
			luaL_Reg reg = usertype_detail::make_reg(std::forward<N>(n), make_func<Idx>());
			for (std::size_t i = 0; i < properties.size(); ++i) {
				meta_function mf = static_cast<meta_function>(i);
				const std::string& mfname = to_string(mf);
				if (mfname == reg.name) {
					switch (mf) {
					case meta_function::construct:
						if (properties[i]) {
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
							throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
							assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
						}
						break;
					case meta_function::garbage_collect:
						if (destructfunc != nullptr) {
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
							throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
							assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
						}
						destructfunc = reg.func;
						return;
					case meta_function::index:
						indexfunc = reg.func;
						mustindex = true;
						properties.set(i);
						return;
					case meta_function::new_index:
						newindexfunc = reg.func;
						mustindex = true;
						properties.set(i);
						return;
					default:
						break;
					}
					properties.set(i);
					break;
				}
			}
			l[index] = reg;
			++index;
		}

		template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == sizeof...(Tn)>>
		usertype_metatable(Args&&... args)
		: usertype_metatable_core(&usertype_detail::indexing_fail<T, true>, &usertype_detail::metatable_new_index<T, false>), usertype_detail::registrar(), functions(std::forward<Args>(args)...), destructfunc(nullptr), callconstructfunc(nullptr), indexbase(&core_indexing_call<true>), newindexbase(&core_indexing_call<false>), indexbaseclasspropogation(usertype_detail::walk_all_bases<true>), newindexbaseclasspropogation(usertype_detail::walk_all_bases<false>), baseclasscheck(nullptr), baseclasscast(nullptr), secondarymeta(contains_variable()), properties() {
			properties.reset();
			std::initializer_list<typename usertype_detail::mapping_t::value_type> ilist{{std::pair<std::string, usertype_detail::call_information>(usertype_detail::make_string(std::get<I * 2>(functions)),
				usertype_detail::call_information(&usertype_metatable::real_find_call<I * 2, I * 2 + 1, true>,
					&usertype_metatable::real_find_call<I * 2, I * 2 + 1, false>))}...};
			this->mapping.insert(ilist);
			for (const auto& n : meta_function_names()) {
				this->mapping.erase(n);
			}
			this->mustindex = contains_variable() || contains_index();
		}

		usertype_metatable(const usertype_metatable&) = default;
		usertype_metatable(usertype_metatable&&) = default;
		usertype_metatable& operator=(const usertype_metatable&) = default;
		usertype_metatable& operator=(usertype_metatable&&) = default;

		template <std::size_t I0, std::size_t I1, bool is_index>
		static int real_find_call(lua_State* L, void* um, usertype_metatable_core&, int) {
			auto& f = *static_cast<usertype_metatable*>(um);
			if (is_variable_binding<decltype(std::get<I1>(f.functions))>::value) {
				return real_call_with<I1, is_index, true>(L, f);
			}
			// set up upvalues
			// for a chained call
			int upvalues = 0;
			upvalues += stack::push(L, nullptr);
			upvalues += stack::push(L, light<usertype_metatable>(f));
			auto cfunc = &call<I1, is_index>;
			return stack::push(L, c_closure(cfunc, upvalues));
		}

		template <bool is_index>
		static int real_meta_call(lua_State* L, void* um, int) {
			auto& f = *static_cast<usertype_metatable*>(um);
			return is_index ? f.indexfunc(L) : f.newindexfunc(L);
		}

		template <bool is_index, bool toplevel = false, bool is_meta_bound = false>
		static int core_indexing_call(lua_State* L) {
			usertype_metatable& f = toplevel
				? static_cast<usertype_metatable&>(stack::get<light<usertype_metatable>>(L, upvalue_index(usertype_detail::metatable_index)))
				: static_cast<usertype_metatable&>(stack::pop<user<usertype_metatable>>(L));
			static const int keyidx = -2 + static_cast<int>(is_index);
			if (toplevel && stack::get<type>(L, keyidx) != type::string) {
				return is_index ? f.indexfunc(L) : f.newindexfunc(L);
			}
			int runtime_target = 0;
			usertype_detail::member_search member = nullptr;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view name = stack::get<string_view>(L, keyidx);
				auto memberit = f.mapping.find(name, string_view_hash(), std::equal_to<string_view>());
#else
				std::string name = stack::get<std::string>(L, keyidx);
				auto memberit = f.mapping.find(name);
#endif
				if (memberit != f.mapping.cend()) {
					const usertype_detail::call_information& ci = memberit->second;
					member = is_index ? ci.index : ci.new_index;
					runtime_target = ci.runtime_target;
				}
			}
			if (member != nullptr) {
				return (member)(L, static_cast<void*>(&f), static_cast<usertype_metatable_core&>(f), runtime_target);
			}
			if (is_meta_bound && toplevel && !is_index) {
				return usertype_detail::metatable_new_index<T, false>(L);
			}
			string_view accessor = stack::get<string_view>(L, keyidx);
			int ret = 0;
			bool found = false;
			// Otherwise, we need to do propagating calls through the bases
			if (is_index)
				f.indexbaseclasspropogation(L, found, ret, accessor);
			else
				f.newindexbaseclasspropogation(L, found, ret, accessor);
			if (found) {
				return ret;
			}
			if (is_meta_bound) {
				return is_index ? usertype_detail::indexing_fail<T, is_index>(L) : usertype_detail::metatable_new_index<T, false>(L);
			}
			return toplevel ? (is_index ? f.indexfunc(L) : f.newindexfunc(L)) : -1;
		}

		static int real_index_call(lua_State* L) {
			return core_indexing_call<true, true>(L);
		}

		static int real_new_index_call(lua_State* L) {
			return core_indexing_call<false, true>(L);
		}

		static int real_meta_index_call(lua_State* L) {
			return core_indexing_call<true, true, true>(L);
		}

		static int real_meta_new_index_call(lua_State* L) {
			return core_indexing_call<false, true, true>(L);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int real_call(lua_State* L) {
			usertype_metatable& f = stack::get<light<usertype_metatable>>(L, upvalue_index(usertype_detail::metatable_index));
			return real_call_with<Idx, is_index, is_variable>(L, f);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int real_call_with(lua_State* L, usertype_metatable& um) {
			typedef meta::unqualified_tuple_element_t<Idx - 1, Tuple> K;
			typedef meta::unqualified_tuple_element_t<Idx, Tuple> F;
			static const int boost = !detail::is_non_factory_constructor<F>::value
					&& std::is_same<K, call_construction>::value
				? 1
				: 0;
			auto& f = std::get<Idx>(um.functions);
			return call_detail::call_wrapped<T, is_index, is_variable, boost>(L, f);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_call<Idx, is_index, is_variable>), (&real_call<Idx, is_index, is_variable>)>(L);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int call_with(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_call_with<Idx, is_index, is_variable>), (&real_call_with<Idx, is_index, is_variable>)>(L);
		}

		static int index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_index_call), (&real_index_call)>(L);
		}

		static int new_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_new_index_call), (&real_new_index_call)>(L);
		}

		static int meta_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_meta_index_call), (&real_meta_index_call)>(L);
		}

		static int meta_new_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_meta_new_index_call), (&real_meta_new_index_call)>(L);
		}

		virtual int push_um(lua_State* L) override {
			return stack::push(L, std::move(*this));
		}

		~usertype_metatable() override {
		}
	};

	namespace stack {

		template <typename T, std::size_t... I, typename... Args>
		struct pusher<usertype_metatable<T, std::index_sequence<I...>, Args...>> {
			typedef usertype_metatable<T, std::index_sequence<I...>, Args...> umt_t;
			typedef typename umt_t::regs_t regs_t;

			static umt_t& make_cleanup(lua_State* L, umt_t&& umx) {
				// ensure some sort of uniqueness
				static int uniqueness = 0;
				std::string uniquegcmetakey = usertype_traits<T>::user_gc_metatable();
				// std::to_string doesn't exist in android still, with NDK, so this bullshit
				// is necessary
				// thanks, Android :v
				int appended = snprintf(nullptr, 0, "%d", uniqueness);
				std::size_t insertionpoint = uniquegcmetakey.length() - 1;
				uniquegcmetakey.append(appended, '\0');
				char* uniquetarget = &uniquegcmetakey[insertionpoint];
				snprintf(uniquetarget, uniquegcmetakey.length(), "%d", uniqueness);
				++uniqueness;

				const char* gcmetakey = &usertype_traits<T>::gc_table()[0];
				// Make sure userdata's memory is properly in lua first,
				// otherwise all the light userdata we make later will become invalid
				stack::push<user<umt_t>>(L, metatable_key, uniquegcmetakey, std::move(umx));
				// Create the top level thing that will act as our deleter later on
				stack_reference umt(L, -1);
				stack::set_field<true>(L, gcmetakey, umt);
				umt.pop();

				stack::get_field<true>(L, gcmetakey);
				umt_t& target_umt = stack::pop<user<umt_t>>(L);
				return target_umt;
			}

			static int push(lua_State* L, umt_t&& umx) {

				umt_t& um = make_cleanup(L, std::move(umx));
				usertype_metatable_core& umc = um;
				regs_t value_table{{}};
				int lastreg = 0;
				(void)detail::swallow{0, (um.template make_regs<(I * 2)>(value_table, lastreg, std::get<(I * 2)>(um.functions), std::get<(I * 2 + 1)>(um.functions)), 0)...};
				um.finish_regs(value_table, lastreg);
				value_table[lastreg] = {nullptr, nullptr};
				regs_t ref_table = value_table;
				regs_t unique_table = value_table;
				bool hasdestructor = !value_table.empty() && to_string(meta_function::garbage_collect) == value_table[lastreg - 1].name;
				if (hasdestructor) {
					ref_table[lastreg - 1] = {nullptr, nullptr};
				}
				unique_table[lastreg - 1] = {value_table[lastreg - 1].name, detail::unique_destruct<T>};

				lua_createtable(L, 0, 2);
				stack_reference type_table(L, -1);

				stack::set_field(L, "name", detail::demangle<T>(), type_table.stack_index());
				stack::set_field(L, "is", &usertype_detail::is_check<T>, type_table.stack_index());

				// Now use um
				const bool& mustindex = umc.mustindex;
				for (std::size_t i = 0; i < 3; ++i) {
					// Pointer types, AKA "references" from C++
					const char* metakey = nullptr;
					luaL_Reg* metaregs = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						metaregs = ref_table.data();
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						metaregs = unique_table.data();
						break;
					case 2:
					default:
						metakey = &usertype_traits<T>::metatable()[0];
						metaregs = value_table.data();
						break;
					}
					luaL_newmetatable(L, metakey);
					stack_reference t(L, -1);
					stack::set_field(L, meta_function::type, type_table, t.stack_index());
					int upvalues = 0;
					upvalues += stack::push(L, nullptr);
					upvalues += stack::push(L, make_light(um));
					luaL_setfuncs(L, metaregs, upvalues);

					if (um.baseclasscheck != nullptr) {
						stack::set_field(L, detail::base_class_check_key(), um.baseclasscheck, t.stack_index());
					}
					if (um.baseclasscast != nullptr) {
						stack::set_field(L, detail::base_class_cast_key(), um.baseclasscast, t.stack_index());
					}

					stack::set_field(L, detail::base_class_index_propogation_key(), make_closure(um.indexbase, nullptr, make_light(um), make_light(umc)), t.stack_index());
					stack::set_field(L, detail::base_class_new_index_propogation_key(), make_closure(um.newindexbase, nullptr, make_light(um), make_light(umc)), t.stack_index());

					if (mustindex) {
						// Basic index pushing: specialize
						// index and newindex to give variables and stuff
						stack::set_field(L, meta_function::index, make_closure(umt_t::index_call, nullptr, make_light(um), make_light(umc)), t.stack_index());
						stack::set_field(L, meta_function::new_index, make_closure(umt_t::new_index_call, nullptr, make_light(um), make_light(umc)), t.stack_index());
					}
					else {
						// If there's only functions, we can use the fast index version
						stack::set_field(L, meta_function::index, t, t.stack_index());
					}
					// metatable on the metatable
					// for call constructor purposes and such
					lua_createtable(L, 0, 3);
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (um.callconstructfunc != nullptr) {
						stack::set_field(L, meta_function::call_function, make_closure(um.callconstructfunc, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}
					if (um.secondarymeta) {
						stack::set_field(L, meta_function::index, make_closure(umt_t::index_call, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
						stack::set_field(L, meta_function::new_index, make_closure(umt_t::new_index_call, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}
					// type information needs to be present on the behind-tables too

					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
					// We want to just leave the table
					// in the registry only, otherwise we return it
					t.pop();
				}

				// Now for the shim-table that actually gets assigned to the name
				luaL_newmetatable(L, &usertype_traits<T>::user_metatable()[0]);
				stack_reference t(L, -1);
				stack::set_field(L, meta_function::type, type_table, t.stack_index());
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push(L, make_light(um));
				luaL_setfuncs(L, value_table.data(), upvalues);
				{
					lua_createtable(L, 0, 3);
					stack_reference metabehind(L, -1);
					// type information needs to be present on the behind-tables too
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (um.callconstructfunc != nullptr) {
						stack::set_field(L, meta_function::call_function, make_closure(um.callconstructfunc, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}

					stack::set_field(L, meta_function::index, make_closure(umt_t::meta_index_call, nullptr, make_light(um), make_light(umc), nullptr, usertype_detail::toplevel_magic), metabehind.stack_index());
					stack::set_field(L, meta_function::new_index, make_closure(umt_t::meta_new_index_call, nullptr, make_light(um), make_light(umc), nullptr, usertype_detail::toplevel_magic), metabehind.stack_index());
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
				}

				lua_remove(L, type_table.stack_index());

				return 1;
			}
		};

	} // namespace stack

} // namespace sol

// end of sol/usertype_metatable.hpp

// beginning of sol/simple_usertype_metatable.hpp

namespace sol {

	namespace usertype_detail {
		inline int call_indexing_object(lua_State* L, object& f) {
			int before = lua_gettop(L);
			f.push();
			for (int i = 1; i <= before; ++i) {
				lua_pushvalue(L, i);
			}
			lua_call(L, before, LUA_MULTRET);
			int after = lua_gettop(L);
			return after - before;
		}

		template <typename T, bool is_index, bool toplevel = false, bool has_indexing = false>
		inline int simple_core_indexing_call(lua_State* L) {
			simple_map& sm = toplevel
				? stack::get<user<simple_map>>(L, upvalue_index(simple_metatable_index))
				: stack::pop<user<simple_map>>(L);
			variable_map& variables = sm.variables;
			function_map& functions = sm.functions;
			static const int keyidx = -2 + static_cast<int>(is_index);
			if (toplevel) {
				if (type_of(L, keyidx) != type::string) {
					if (has_indexing) {
						object& indexingfunc = is_index
							? sm.index
							: sm.newindex;
						return call_indexing_object(L, indexingfunc);
					}
					else {
						return is_index
							? indexing_fail<T, is_index>(L)
							: metatable_new_index<T, true>(L);
					}
				}
			}
			string_view accessor = stack::get<string_view>(L, keyidx);
			variable_wrapper* varwrap = nullptr;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view& accessorkey = accessor;
				auto vit = variables.find(accessorkey, string_view_hash(), std::equal_to<string_view>());
#else
				std::string accessorkey(accessor.data(), accessor.size());
				auto vit = variables.find(accessorkey);
#endif // Compatible Hash
				if (vit != variables.cend()) {
					varwrap = vit->second.get();
				}
			}
			if (varwrap != nullptr) {
				return is_index ? varwrap->index(L) : varwrap->new_index(L);
			}
			bool function_failed = false;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view& accessorkey = accessor;
				auto fit = functions.find(accessorkey, string_view_hash(), std::equal_to<string_view>());
#else
				std::string accessorkey(accessor.data(), accessor.size());
				auto fit = functions.find(accessorkey);
#endif // Compatible Hash
				if (fit != functions.cend()) {
					object& func = fit->second;
					if (is_index) {
						return stack::push(L, func);
					}
					else {
						function_failed = true;
					}
				}
			}
			if (function_failed) {
				if (has_indexing && !is_toplevel(L)) {
					object& indexingfunc = is_index
						? sm.index
						: sm.newindex;
					return call_indexing_object(L, indexingfunc);
				}
				else {
					return is_index
						? indexing_fail<T, is_index>(L)
						: metatable_new_index<T, true>(L);
				}
			}
			/* Check table storage first for a method that works
			luaL_getmetatable(L, sm.metakey);
			if (type_of(L, -1) != type::lua_nil) {
				stack::get_field<false, true>(L, accessor.c_str(), lua_gettop(L));
				if (type_of(L, -1) != type::lua_nil) {
					// Woo, we found it?
					lua_remove(L, -2);
					return 1;
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			*/

			int ret = 0;
			bool found = false;
			// Otherwise, we need to do propagating calls through the bases
			if (is_index) {
				sm.indexbaseclasspropogation(L, found, ret, accessor);
			}
			else {
				sm.newindexbaseclasspropogation(L, found, ret, accessor);
			}
			if (found) {
				return ret;
			}
			if (toplevel) {
				if (has_indexing && !is_toplevel(L)) {
					object& indexingfunc = is_index
						? sm.index
						: sm.newindex;
					return call_indexing_object(L, indexingfunc);
				}
				else {
					return is_index
						? indexing_fail<T, is_index>(L)
						: metatable_new_index<T, true>(L);
				}
			}
			return -1;
		}

		template <typename T, bool has_indexing = false>
		inline int simple_real_index_call(lua_State* L) {
			return simple_core_indexing_call<T, true, true, has_indexing>(L);
		}

		template <typename T, bool has_indexing = false>
		inline int simple_real_new_index_call(lua_State* L) {
			return simple_core_indexing_call<T, false, true, has_indexing>(L);
		}

		template <typename T, bool has_indexing = false>
		inline int simple_index_call(lua_State* L) {
#if defined(__clang__)
			return detail::trampoline(L, &simple_real_index_call<T, has_indexing>);
#else
			return detail::typed_static_trampoline<decltype(&simple_real_index_call<T, has_indexing>), (&simple_real_index_call<T, has_indexing>)>(L);
#endif
		}

		template <typename T, bool has_indexing = false>
		inline int simple_new_index_call(lua_State* L) {
#if defined(__clang__)
			return detail::trampoline(L, &simple_real_new_index_call<T, has_indexing>);
#else
			return detail::typed_static_trampoline<decltype(&simple_real_new_index_call<T, has_indexing>), (&simple_real_new_index_call<T, has_indexing>)>(L);
#endif
		}
	} // namespace usertype_detail

	struct simple_tag {
	} const simple{};

	template <typename T>
	struct simple_usertype_metatable : usertype_detail::registrar {
	public:
		usertype_detail::function_map registrations;
		usertype_detail::variable_map varmap;
		object callconstructfunc;
		object indexfunc;
		object newindexfunc;
		lua_CFunction indexbase;
		lua_CFunction newindexbase;
		usertype_detail::base_walk indexbaseclasspropogation;
		usertype_detail::base_walk newindexbaseclasspropogation;
		void* baseclasscheck;
		void* baseclasscast;
		bool mustindex;
		bool secondarymeta;
		std::array<bool, 32> properties;

		template <typename N>
		void insert(N&& n, object&& o) {
			std::string key = usertype_detail::make_string(std::forward<N>(n));
			int is_indexer = static_cast<int>(usertype_detail::is_indexer(n));
			if (is_indexer == 1) {
				indexfunc = o;
				mustindex = true;
			}
			else if (is_indexer == 2) {
				newindexfunc = o;
				mustindex = true;
			}
			auto hint = registrations.find(key);
			if (hint == registrations.cend()) {
				registrations.emplace_hint(hint, std::move(key), std::move(o));
				return;
			}
			hint->second = std::move(o);
		}

		template <typename N, typename F, typename... Args>
		void insert_prepare(std::true_type, lua_State* L, N&&, F&& f, Args&&... args) {
			object o = make_object<F>(L, std::forward<F>(f), function_detail::call_indicator(), std::forward<Args>(args)...);
			callconstructfunc = std::move(o);
		}

		template <typename N, typename F, typename... Args>
		void insert_prepare(std::false_type, lua_State* L, N&& n, F&& f, Args&&... args) {
			object o = make_object<F>(L, std::forward<F>(f), std::forward<Args>(args)...);
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename F>
		void add_member_function(std::true_type, lua_State* L, N&& n, F&& f) {
			insert_prepare(std::is_same<meta::unqualified_t<N>, call_construction>(), L, std::forward<N>(n), std::forward<F>(f), function_detail::class_indicator<T>());
		}

		template <typename N, typename F>
		void add_member_function(std::false_type, lua_State* L, N&& n, F&& f) {
			insert_prepare(std::is_same<meta::unqualified_t<N>, call_construction>(), L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::enable<meta::is_callable<meta::unwrap_unqualified_t<F>>> = meta::enabler>
		void add_function(lua_State* L, N&& n, F&& f) {
			object o = make_object(L, as_function_reference(std::forward<F>(f)));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename F, meta::disable<meta::is_callable<meta::unwrap_unqualified_t<F>>> = meta::enabler>
		void add_function(lua_State* L, N&& n, F&& f) {
			add_member_function(std::is_member_pointer<meta::unwrap_unqualified_t<F>>(), L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::disable<is_variable_binding<meta::unqualified_t<F>>> = meta::enabler>
		void add(lua_State* L, N&& n, F&& f) {
			add_function(L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::enable<is_variable_binding<meta::unqualified_t<F>>> = meta::enabler>
		void add(lua_State*, N&& n, F&& f) {
			mustindex = true;
			secondarymeta = true;
			std::string key = usertype_detail::make_string(std::forward<N>(n));
			auto o = std::make_unique<usertype_detail::callable_binding<T, std::decay_t<F>>>(std::forward<F>(f));
			auto hint = varmap.find(key);
			if (hint == varmap.cend()) {
				varmap.emplace_hint(hint, std::move(key), std::move(o));
				return;
			}
			hint->second = std::move(o);
		}

		template <typename N, typename... Fxs>
		void add(lua_State* L, N&& n, constructor_wrapper<Fxs...> c) {
			object o(L, in_place_type<detail::tagged<T, constructor_wrapper<Fxs...>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename... Lists>
		void add(lua_State* L, N&& n, constructor_list<Lists...> c) {
			object o(L, in_place_type<detail::tagged<T, constructor_list<Lists...>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N>
		void add(lua_State* L, N&& n, destructor_wrapper<void> c) {
			object o(L, in_place_type<detail::tagged<T, destructor_wrapper<void>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename Fx>
		void add(lua_State* L, N&& n, destructor_wrapper<Fx> c) {
			object o(L, in_place_type<detail::tagged<T, destructor_wrapper<Fx>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename... Bases>
		void add(lua_State*, base_classes_tag, bases<Bases...>) {
			static_assert(sizeof(usertype_detail::base_walk) <= sizeof(void*), "size of function pointer is greater than sizeof(void*); cannot work on this platform. Please file a bug report.");
			static_assert(!meta::any_same<T, Bases...>::value, "base classes cannot list the original class as part of the bases");
			if (sizeof...(Bases) < 1) {
				return;
			}
			mustindex = true;
			(void)detail::swallow{0, ((detail::has_derived<Bases>::value = true), 0)...};

			static_assert(sizeof(void*) <= sizeof(detail::inheritance_check_function), "The size of this data pointer is too small to fit the inheritance checking function: Please file a bug report.");
			static_assert(sizeof(void*) <= sizeof(detail::inheritance_cast_function), "The size of this data pointer is too small to fit the inheritance checking function: Please file a bug report.");
			baseclasscheck = reinterpret_cast<void*>(&detail::inheritance<T, Bases...>::type_check);
			baseclasscast = reinterpret_cast<void*>(&detail::inheritance<T, Bases...>::type_cast);
			indexbaseclasspropogation = usertype_detail::walk_all_bases<true, Bases...>;
			newindexbaseclasspropogation = usertype_detail::walk_all_bases<false, Bases...>;
		}

	private:
		template <std::size_t... I, typename Tuple>
		simple_usertype_metatable(detail::verified_tag, std::index_sequence<I...>, lua_State* L, Tuple&& args)
		: callconstructfunc(lua_nil), indexfunc(lua_nil), newindexfunc(lua_nil), indexbase(&usertype_detail::simple_core_indexing_call<T, true>), newindexbase(&usertype_detail::simple_core_indexing_call<T, false>), indexbaseclasspropogation(usertype_detail::walk_all_bases<true>), newindexbaseclasspropogation(&usertype_detail::walk_all_bases<false>), baseclasscheck(nullptr), baseclasscast(nullptr), mustindex(false), secondarymeta(false), properties() {
			properties.fill(false);

			(void)detail::swallow{0,
				(add(L, detail::forward_get<I * 2>(args), detail::forward_get<I * 2 + 1>(args)), 0)...};
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::verified_tag v, Args&&... args)
		: simple_usertype_metatable(v, std::make_index_sequence<sizeof...(Args) / 2>(), L, std::forward_as_tuple(std::forward<Args>(args)...)) {
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::add_destructor_tag, Args&&... args)
		: simple_usertype_metatable(L, detail::verified, std::forward<Args>(args)..., "__gc", default_destructor) {
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::check_destructor_tag, Args&&... args)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_destructible<T>, meta::neg<detail::has_destructor<Args...>>>, detail::add_destructor_tag, detail::verified_tag>(), std::forward<Args>(args)...) {
		}

	public:
		simple_usertype_metatable(lua_State* L)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_default_constructible<T>>, decltype(default_constructor), detail::check_destructor_tag>()) {
		}

		template <typename Arg, typename... Args, meta::disable_any<meta::any_same<meta::unqualified_t<Arg>, detail::verified_tag, detail::add_destructor_tag, detail::check_destructor_tag>, meta::is_specialization_of<meta::unqualified_t<Arg>, constructors>, meta::is_specialization_of<meta::unqualified_t<Arg>, constructor_wrapper>> = meta::enabler>
		simple_usertype_metatable(lua_State* L, Arg&& arg, Args&&... args)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_default_constructible<T>, meta::neg<detail::has_constructor<Args...>>>, decltype(default_constructor), detail::check_destructor_tag>(), std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}

		template <typename... Args, typename... CArgs>
		simple_usertype_metatable(lua_State* L, constructors<CArgs...> constructorlist, Args&&... args)
		: simple_usertype_metatable(L, detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		template <typename... Args, typename... Fxs>
		simple_usertype_metatable(lua_State* L, constructor_wrapper<Fxs...> constructorlist, Args&&... args)
		: simple_usertype_metatable(L, detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		simple_usertype_metatable(const simple_usertype_metatable&) = default;
		simple_usertype_metatable(simple_usertype_metatable&&) = default;
		simple_usertype_metatable& operator=(const simple_usertype_metatable&) = default;
		simple_usertype_metatable& operator=(simple_usertype_metatable&&) = default;

		virtual int push_um(lua_State* L) override {
			return stack::push(L, std::move(*this));
		}
	};

	namespace stack {
		template <typename T>
		struct pusher<simple_usertype_metatable<T>> {
			typedef simple_usertype_metatable<T> umt_t;

			static usertype_detail::simple_map& make_cleanup(lua_State* L, umt_t& umx) {
				static int uniqueness = 0;
				std::string uniquegcmetakey = usertype_traits<T>::user_gc_metatable();
				// std::to_string doesn't exist in android still, with NDK, so this bullshit
				// is necessary
				// thanks, Android :v
				int appended = snprintf(nullptr, 0, "%d", uniqueness);
				std::size_t insertionpoint = uniquegcmetakey.length() - 1;
				uniquegcmetakey.append(appended, '\0');
				char* uniquetarget = &uniquegcmetakey[insertionpoint];
				snprintf(uniquetarget, uniquegcmetakey.length(), "%d", uniqueness);
				++uniqueness;

				const char* gcmetakey = &usertype_traits<T>::gc_table()[0];
				stack::push<user<usertype_detail::simple_map>>(L, metatable_key, uniquegcmetakey, &usertype_traits<T>::metatable()[0],
					umx.indexbaseclasspropogation, umx.newindexbaseclasspropogation,
					std::move(umx.indexfunc), std::move(umx.newindexfunc),
					std::move(umx.varmap), std::move(umx.registrations));
				stack_reference stackvarmap(L, -1);
				stack::set_field<true>(L, gcmetakey, stackvarmap);
				stackvarmap.pop();

				stack::get_field<true>(L, gcmetakey);
				usertype_detail::simple_map& varmap = stack::pop<user<usertype_detail::simple_map>>(L);
				return varmap;
			}

			static int push(lua_State* L, umt_t&& umx) {
				bool hasindex = umx.indexfunc.valid();
				bool hasnewindex = umx.newindexfunc.valid();
				auto& varmap = make_cleanup(L, umx);
				auto& properties = umx.properties;
				auto sic = hasindex ? &usertype_detail::simple_index_call<T, true> : &usertype_detail::simple_index_call<T, false>;
				auto snic = hasnewindex ? &usertype_detail::simple_new_index_call<T, true> : &usertype_detail::simple_new_index_call<T, false>;

				lua_createtable(L, 0, 2);
				stack_reference type_table(L, -1);

				stack::set_field(L, "name", detail::demangle<T>(), type_table.stack_index());
				stack::set_field(L, "is", &usertype_detail::is_check<T>, type_table.stack_index());

				auto safety_check = [&](const std::string& first) {
					for (std::size_t j = 0; j < properties.size(); ++j) {
						meta_function mf = static_cast<meta_function>(j);
						const std::string& mfname = to_string(mf);
						bool& prop = properties[j];
						if (mfname != first)
							continue;
						switch (mf) {
						case meta_function::construct:
							if (prop) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
								assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
								throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
							}
							break;
						case meta_function::garbage_collect:
							if (prop) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
								assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
								throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
							}
							return;
						default:
							break;
						}
						prop = true;
						break;
					}
				};

				for (auto& kvp : varmap.functions) {
					auto& first = std::get<0>(kvp);
					safety_check(first);
				}

				auto register_kvp = [&](std::size_t meta_index, stack_reference& t, const std::string& first, object& second) {
					meta_function mf = meta_function::construct;
					for (std::size_t j = 0; j < properties.size(); ++j) {
						mf = static_cast<meta_function>(j);
						const std::string& mfname = to_string(mf);
						bool& prop = properties[j];
						if (mfname != first)
							continue;
						switch (mf) {
						case meta_function::index:
							umx.indexfunc = second;
							break;
						case meta_function::new_index:
							umx.newindexfunc = second;
							break;
						default:
							break;
						}
						prop = true;
						break;
					}
					switch (meta_index) {
					case 0:
						if (mf == meta_function::garbage_collect) {
							return;
						}
						break;
					case 1:
						if (mf == meta_function::garbage_collect) {
							stack::set_field(L, first, detail::unique_destruct<T>, t.stack_index());
							return;
						}
						break;
					case 2:
					default:
						break;
					}
					stack::set_field(L, first, second, t.stack_index());
				};
				for (std::size_t i = 0; i < 3; ++i) {
					const char* metakey = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						break;
					case 2:
					default:
						metakey = &usertype_traits<T>::metatable()[0];
						break;
					}
					luaL_newmetatable(L, metakey);
					stack_reference t(L, -1);
					stack::set_field(L, meta_function::type, type_table, t.stack_index());

					for (auto& kvp : varmap.functions) {
						auto& first = std::get<0>(kvp);
						auto& second = std::get<1>(kvp);
						register_kvp(i, t, first, second);
					}
					luaL_Reg opregs[34]{};
					int opregsindex = 0;
					auto prop_fx = [&](meta_function mf) { return !properties[static_cast<int>(mf)]; };
					usertype_detail::insert_default_registrations<T>(opregs, opregsindex, prop_fx);
					t.push();
					luaL_setfuncs(L, opregs, 0);
					t.pop();

					if (umx.baseclasscheck != nullptr) {
						stack::set_field(L, detail::base_class_check_key(), umx.baseclasscheck, t.stack_index());
					}
					if (umx.baseclasscast != nullptr) {
						stack::set_field(L, detail::base_class_cast_key(), umx.baseclasscast, t.stack_index());
					}

					// Base class propagation features
					stack::set_field(L, detail::base_class_index_propogation_key(), umx.indexbase, t.stack_index());
					stack::set_field(L, detail::base_class_new_index_propogation_key(), umx.newindexbase, t.stack_index());

					if (umx.mustindex) {
						// use indexing function
						stack::set_field(L, meta_function::index,
							make_closure(sic,
								nullptr,
								make_light(varmap)),
							t.stack_index());
						stack::set_field(L, meta_function::new_index,
							make_closure(snic,
								nullptr,
								make_light(varmap)),
							t.stack_index());
					}
					else {
						// Metatable indexes itself
						stack::set_field(L, meta_function::index, t, t.stack_index());
					}
					// metatable on the metatable
					// for call constructor purposes and such
					lua_createtable(L, 0, 2 * static_cast<int>(umx.secondarymeta) + static_cast<int>(umx.callconstructfunc.valid()));
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (umx.callconstructfunc.valid()) {
						stack::set_field(L, meta_function::call_function, umx.callconstructfunc, metabehind.stack_index());
					}
					if (umx.secondarymeta) {
						stack::set_field(L, meta_function::index,
							make_closure(sic,
								nullptr,
								make_light(varmap)),
							metabehind.stack_index());
						stack::set_field(L, meta_function::new_index,
							make_closure(snic,
								nullptr,
								make_light(varmap)),
							metabehind.stack_index());
					}
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();

					t.pop();
				}

				// Now for the shim-table that actually gets pushed
				luaL_newmetatable(L, &usertype_traits<T>::user_metatable()[0]);
				stack_reference t(L, -1);
				stack::set_field(L, meta_function::type, type_table, t.stack_index());

				for (auto& kvp : varmap.functions) {
					auto& first = std::get<0>(kvp);
					auto& second = std::get<1>(kvp);
					register_kvp(2, t, first, second);
				}
				{
					lua_createtable(L, 0, 2 + static_cast<int>(umx.callconstructfunc.valid()));
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (umx.callconstructfunc.valid()) {
						stack::set_field(L, meta_function::call_function, umx.callconstructfunc, metabehind.stack_index());
					}
					// use indexing function
					stack::set_field(L, meta_function::index,
						make_closure(sic,
							nullptr,
							make_light(varmap),
							nullptr,
							nullptr,
							usertype_detail::toplevel_magic),
						metabehind.stack_index());
					stack::set_field(L, meta_function::new_index,
						make_closure(snic,
							nullptr,
							make_light(varmap),
							nullptr,
							nullptr,
							usertype_detail::toplevel_magic),
						metabehind.stack_index());
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
				}

				lua_remove(L, type_table.stack_index());

				// Don't pop the table when we're done;
				// return it
				return 1;
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/simple_usertype_metatable.hpp

namespace sol {

	template <typename T>
	class usertype {
	private:
		std::unique_ptr<usertype_detail::registrar> metatableregister;

		template <typename... Args>
		usertype(detail::verified_tag, Args&&... args)
		: metatableregister(std::make_unique<usertype_metatable<T, std::make_index_sequence<sizeof...(Args) / 2>, Args...>>(std::forward<Args>(args)...)) {
			static_assert(detail::has_destructor<Args...>::value, "this type does not have an explicit destructor declared; please pass a custom destructor function wrapped in sol::destruct, especially if the type does not have an accessible (private) destructor");
		}

		template <typename... Args>
		usertype(detail::add_destructor_tag, Args&&... args)
		: usertype(detail::verified, std::forward<Args>(args)..., "__gc", default_destructor) {
		}

		template <typename... Args>
		usertype(detail::check_destructor_tag, Args&&... args)
		: usertype(meta::condition<meta::all<std::is_destructible<T>, meta::neg<detail::has_destructor<Args...>>>, detail::add_destructor_tag, detail::verified_tag>(), std::forward<Args>(args)...) {
		}

	public:
		template <typename... Args>
		usertype(Args&&... args)
		: usertype(meta::condition<meta::all<std::is_default_constructible<T>, meta::neg<detail::has_constructor<Args...>>>, decltype(default_constructor), detail::check_destructor_tag>(), std::forward<Args>(args)...) {
		}

		template <typename... Args, typename... CArgs>
		usertype(constructors<CArgs...> constructorlist, Args&&... args)
		: usertype(detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		template <typename... Args, typename... Fxs>
		usertype(constructor_wrapper<Fxs...> constructorlist, Args&&... args)
		: usertype(detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		template <typename... Args>
		usertype(simple_tag, lua_State* L, Args&&... args)
		: metatableregister(std::make_unique<simple_usertype_metatable<T>>(L, std::forward<Args>(args)...)) {
		}

		usertype_detail::registrar* registrar_data() {
			return metatableregister.get();
		}

		int push(lua_State* L) {
			int r = metatableregister->push_um(L);
			metatableregister = nullptr;
			return r;
		}
	};

	template <typename T>
	class simple_usertype : public usertype<T> {
	private:
		typedef usertype<T> base_t;
		lua_State* state;

	public:
		template <typename... Args>
		simple_usertype(lua_State* L, Args&&... args)
		: base_t(simple, L, std::forward<Args>(args)...), state(L) {
		}

		template <typename N, typename F>
		void set(N&& n, F&& f) {
			auto meta = static_cast<simple_usertype_metatable<T>*>(base_t::registrar_data());
			meta->add(state, std::forward<N>(n), std::forward<F>(f));
		}
	};

	namespace stack {
		template <typename T>
		struct pusher<usertype<T>> {
			static int push(lua_State* L, usertype<T>& user) {
				return user.push(L);
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/usertype.hpp

// beginning of sol/table_iterator.hpp

namespace sol {

	template <typename reference_type>
	class basic_table_iterator : public std::iterator<std::input_iterator_tag, std::pair<object, object>> {
	public:
		typedef object key_type;
		typedef object mapped_type;
		typedef std::pair<object, object> value_type;
		typedef std::input_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		typedef value_type* pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;

	private:
		std::pair<object, object> kvp;
		reference_type ref;
		int tableidx = 0;
		int keyidx = 0;
		std::ptrdiff_t idx = 0;

	public:
		basic_table_iterator()
		: keyidx(-1), idx(-1) {
		}

		basic_table_iterator(reference_type x)
		: ref(std::move(x)) {
			ref.push();
			tableidx = lua_gettop(ref.lua_state());
			stack::push(ref.lua_state(), lua_nil);
			this->operator++();
			if (idx == -1) {
				return;
			}
			--idx;
		}

		basic_table_iterator& operator++() {
			if (idx == -1)
				return *this;

			if (lua_next(ref.lua_state(), tableidx) == 0) {
				idx = -1;
				keyidx = -1;
				return *this;
			}
			++idx;
			kvp.first = object(ref.lua_state(), -2);
			kvp.second = object(ref.lua_state(), -1);
			lua_pop(ref.lua_state(), 1);
			// leave key on the stack
			keyidx = lua_gettop(ref.lua_state());
			return *this;
		}

		basic_table_iterator operator++(int) {
			auto saved = *this;
			this->operator++();
			return saved;
		}

		reference operator*() {
			return kvp;
		}

		const_reference operator*() const {
			return kvp;
		}

		bool operator==(const basic_table_iterator& right) const {
			return idx == right.idx;
		}

		bool operator!=(const basic_table_iterator& right) const {
			return idx != right.idx;
		}

		~basic_table_iterator() {
			if (keyidx != -1) {
				stack::remove(ref.lua_state(), keyidx, 1);
			}
			if (ref.lua_state() != nullptr && ref.valid()) {
				stack::remove(ref.lua_state(), tableidx, 1);
			}
		}
	};

} // namespace sol

// end of sol/table_iterator.hpp

namespace sol {
	namespace detail {
		template <std::size_t n>
		struct clean {
			lua_State* L;
			clean(lua_State* luastate)
			: L(luastate) {
			}
			~clean() {
				lua_pop(L, static_cast<int>(n));
			}
		};
		struct ref_clean {
			lua_State* L;
			int& n;
			ref_clean(lua_State* luastate, int& n)
			: L(luastate), n(n) {
			}
			~ref_clean() {
				lua_pop(L, static_cast<int>(n));
			}
		};
		inline int fail_on_newindex(lua_State* L) {
			return luaL_error(L, "sol: cannot modify the elements of an enumeration table");
		}
	} // namespace detail

	const new_table create = new_table{};

	template <bool top_level, typename base_type>
	class basic_table_core : public basic_object_base<base_type> {
		typedef basic_object_base<base_type> base_t;
		friend class state;
		friend class state_view;

		template <typename... Args>
		using is_global = meta::all<meta::boolean<top_level>, meta::is_c_str<Args>...>;

		template <typename Fx>
		void for_each(std::true_type, Fx&& fx) const {
			auto pp = stack::push_pop(*this);
			stack::push(base_t::lua_state(), lua_nil);
			while (lua_next(base_t::lua_state(), -2)) {
				object key(base_t::lua_state(), -2);
				object value(base_t::lua_state(), -1);
				std::pair<object&, object&> keyvalue(key, value);
				auto pn = stack::pop_n(base_t::lua_state(), 1);
				fx(keyvalue);
			}
		}

		template <typename Fx>
		void for_each(std::false_type, Fx&& fx) const {
			auto pp = stack::push_pop(*this);
			stack::push(base_t::lua_state(), lua_nil);
			while (lua_next(base_t::lua_state(), -2)) {
				object key(base_t::lua_state(), -2);
				object value(base_t::lua_state(), -1);
				auto pn = stack::pop_n(base_t::lua_state(), 1);
				fx(key, value);
			}
		}

		template <bool raw, typename Ret0, typename Ret1, typename... Ret, std::size_t... I, typename Keys>
		auto tuple_get(types<Ret0, Ret1, Ret...>, std::index_sequence<0, 1, I...>, Keys&& keys) const
			-> decltype(stack::pop<std::tuple<Ret0, Ret1, Ret...>>(nullptr)) {
			typedef decltype(stack::pop<std::tuple<Ret0, Ret1, Ret...>>(nullptr)) Tup;
			return Tup(
				traverse_get_optional<top_level, raw, Ret0>(meta::is_optional<meta::unqualified_t<Ret0>>(), detail::forward_get<0>(keys)),
				traverse_get_optional<top_level, raw, Ret1>(meta::is_optional<meta::unqualified_t<Ret1>>(), detail::forward_get<1>(keys)),
				traverse_get_optional<top_level, raw, Ret>(meta::is_optional<meta::unqualified_t<Ret>>(), detail::forward_get<I>(keys))...);
		}

		template <bool raw, typename Ret, std::size_t I, typename Keys>
		decltype(auto) tuple_get(types<Ret>, std::index_sequence<I>, Keys&& keys) const {
			return traverse_get_optional<top_level, raw, Ret>(meta::is_optional<meta::unqualified_t<Ret>>(), detail::forward_get<I>(keys));
		}

		template <bool raw, typename Pairs, std::size_t... I>
		void tuple_set(std::index_sequence<I...>, Pairs&& pairs) {
			auto pp = stack::push_pop < top_level && (is_global<decltype(detail::forward_get<I * 2>(pairs))...>::value) > (*this);
			void(detail::swallow{ (stack::set_field<top_level, raw>(base_t::lua_state(),
								   detail::forward_get<I * 2>(pairs),
								   detail::forward_get<I * 2 + 1>(pairs),
								   lua_gettop(base_t::lua_state())),
				0)... });
		}

		template <bool global, bool raw, typename T, typename Key>
		decltype(auto) traverse_get_deep(Key&& key) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			return stack::get<T>(base_t::lua_state());
		}

		template <bool global, bool raw, typename T, typename Key, typename... Keys>
		decltype(auto) traverse_get_deep(Key&& key, Keys&&... keys) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			return traverse_get_deep<false, raw, T>(std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, std::size_t I, typename Key>
		decltype(auto) traverse_get_deep_optional(int& popcount, Key&& key) const {
			typedef decltype(stack::get<T>(base_t::lua_state())) R;
			auto p = stack::probe_get_field<global, raw, T>(base_t::lua_state(), std::forward<Key>(key), lua_gettop(base_t::lua_state()));
			popcount += p.levels;
			if (!p.success)
				return R(nullopt);
			return stack::get<T>(base_t::lua_state());
		}

		template <bool global, bool raw, typename T, std::size_t I, typename Key, typename... Keys>
		decltype(auto) traverse_get_deep_optional(int& popcount, Key&& key, Keys&&... keys) const {
			auto p = I > 0 ? stack::probe_get_field<global>(base_t::lua_state(), std::forward<Key>(key), -1) : stack::probe_get_field<global>(base_t::lua_state(), std::forward<Key>(key), lua_gettop(base_t::lua_state()));
			popcount += p.levels;
			if (!p.success)
				return T(nullopt);
			return traverse_get_deep_optional<false, raw, T, I + 1>(popcount, std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, typename... Keys>
		decltype(auto) traverse_get_optional(std::false_type, Keys&&... keys) const {
			detail::clean<sizeof...(Keys)> c(base_t::lua_state());
			return traverse_get_deep<global, raw, T>(std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, typename... Keys>
		decltype(auto) traverse_get_optional(std::true_type, Keys&&... keys) const {
			int popcount = 0;
			detail::ref_clean c(base_t::lua_state(), popcount);
			return traverse_get_deep_optional<global, raw, T, 0>(popcount, std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename Key, typename Value>
		void traverse_set_deep(Key&& key, Value&& value) const {
			stack::set_field<global, raw>(base_t::lua_state(), std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global, bool raw, typename Key, typename... Keys>
		void traverse_set_deep(Key&& key, Keys&&... keys) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			traverse_set_deep<false, raw>(std::forward<Keys>(keys)...);
		}

		basic_table_core(lua_State* L, detail::global_tag t) noexcept
		: base_t(L, t) {
		}

	protected:
		basic_table_core(detail::no_safety_tag, lua_nil_t n)
		: base_t(n) {
		}
		basic_table_core(detail::no_safety_tag, lua_State* L, int index)
		: base_t(L, index) {
		}
		basic_table_core(detail::no_safety_tag, lua_State* L, ref_index index)
		: base_t(L, index) {
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_table_core>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(detail::no_safety_tag, T&& r) noexcept
		: base_t(std::forward<T>(r)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(detail::no_safety_tag, lua_State*L, T&& r) noexcept
		: base_t(L, std::forward<T>(r)) {
		}

	public:
		typedef basic_table_iterator<base_type> iterator;
		typedef iterator const_iterator;

		using base_t::lua_state;

		basic_table_core() noexcept = default;
		basic_table_core(const basic_table_core&) = default;
		basic_table_core(basic_table_core&&) = default;
		basic_table_core& operator=(const basic_table_core&) = default;
		basic_table_core& operator=(basic_table_core&&) = default;
		basic_table_core(const stack_reference& r)
		: basic_table_core(r.lua_state(), r.stack_index()) {
		}
		basic_table_core(stack_reference&& r)
		: basic_table_core(r.lua_state(), r.stack_index()) {
		}
		template <typename T, meta::enable_any<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_table_core>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_table_core(lua_State* L, const new_table& nt)
		: base_t(L, -stack::push(L, nt)) {
			if (!is_stack_based<meta::unqualified_t<base_type>>::value) {
				lua_pop(L, 1);
			}
		}
		basic_table_core(lua_State* L, int index = -1)
		: basic_table_core(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_table_core>(L, index, handler);
#endif // Safety
		}
		basic_table_core(lua_State* L, ref_index index)
		: basic_table_core(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_table_core>(lua_state(), -1, handler);
#endif // Safety
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_table_core>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(T&& r) noexcept
		: basic_table_core(detail::no_safety, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_table<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_table_core>(base_t::lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_table_core(lua_nil_t r) noexcept
		: basic_table_core(detail::no_safety, r) {
		}

		iterator begin() const {
			return iterator(*this);
		}

		iterator end() const {
			return iterator();
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		template <typename... Ret, typename... Keys>
		decltype(auto) get(Keys&&... keys) const {
			static_assert(sizeof...(Keys) == sizeof...(Ret), "number of keys and number of return types do not match");
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return tuple_get<false>(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), std::forward_as_tuple(std::forward<Keys>(keys)...));
		}

		template <typename T, typename Key>
		decltype(auto) get_or(Key&& key, T&& otherwise) const {
			typedef decltype(get<T>("")) U;
			optional<U> option = get<optional<U>>(std::forward<Key>(key));
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename Key, typename D>
		decltype(auto) get_or(Key&& key, D&& otherwise) const {
			optional<T> option = get<optional<T>>(std::forward<Key>(key));
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		template <typename T, typename... Keys>
		decltype(auto) traverse_get(Keys&&... keys) const {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return traverse_get_optional<top_level, false, T>(meta::is_optional<meta::unqualified_t<T>>(), std::forward<Keys>(keys)...);
		}

		template <typename... Keys>
		basic_table_core& traverse_set(Keys&&... keys) {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			auto pn = stack::pop_n(base_t::lua_state(), static_cast<int>(sizeof...(Keys) - 2));
			traverse_set_deep<top_level, false>(std::forward<Keys>(keys)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& set(Args&&... args) {
			tuple_set<false>(std::make_index_sequence<sizeof...(Args) / 2>(), std::forward_as_tuple(std::forward<Args>(args)...));
			return *this;
		}

		template <typename... Ret, typename... Keys>
		decltype(auto) raw_get(Keys&&... keys) const {
			static_assert(sizeof...(Keys) == sizeof...(Ret), "number of keys and number of return types do not match");
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return tuple_get<true>(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), std::forward_as_tuple(std::forward<Keys>(keys)...));
		}

		template <typename T, typename Key>
		decltype(auto) raw_get_or(Key&& key, T&& otherwise) const {
			typedef decltype(raw_get<T>("")) U;
			optional<U> option = raw_get<optional<U>>(std::forward<Key>(key));
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename Key, typename D>
		decltype(auto) raw_get_or(Key&& key, D&& otherwise) const {
			optional<T> option = raw_get<optional<T>>(std::forward<Key>(key));
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		template <typename T, typename... Keys>
		decltype(auto) traverse_raw_get(Keys&&... keys) const {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return traverse_get_optional<top_level, true, T>(meta::is_optional<meta::unqualified_t<T>>(), std::forward<Keys>(keys)...);
		}

		template <typename... Keys>
		basic_table_core& traverse_raw_set(Keys&&... keys) {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			auto pn = stack::pop_n(base_t::lua_state(), static_cast<int>(sizeof...(Keys) - 2));
			traverse_set_deep<top_level, true>(std::forward<Keys>(keys)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& raw_set(Args&&... args) {
			tuple_set<true>(std::make_index_sequence<sizeof...(Args) / 2>(), std::forward_as_tuple(std::forward<Args>(args)...));
			return *this;
		}

		template <typename T>
		basic_table_core& set_usertype(usertype<T>& user) {
			return set_usertype(usertype_traits<T>::name(), user);
		}

		template <typename Key, typename T>
		basic_table_core& set_usertype(Key&& key, usertype<T>& user) {
			return set(std::forward<Key>(key), user);
		}

		template <typename Class, typename... Args>
		basic_table_core& new_usertype(const std::string& name, Args&&... args) {
			usertype<Class> utype(std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		basic_table_core& new_usertype(const std::string& name, Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return new_usertype<Class>(name, ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		basic_table_core& new_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			usertype<Class> utype(ctor, std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return new_simple_usertype<Class>(name, ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), ctor, std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), std::forward<Args>(args)...);
			return utype;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return create_simple_usertype<Class>(ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		simple_usertype<Class> create_simple_usertype(constructors<CArgs...> ctor, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), ctor, std::forward<Args>(args)...);
			return utype;
		}

		template <bool read_only = true, typename... Args>
		table new_enum(const string_view& name, Args&&... args) {
			table target = create_with(std::forward<Args>(args)...);
			if (read_only) {
				table x = create_with(
					meta_function::new_index, detail::fail_on_newindex,
					meta_function::index, target);
				table shim = create_named(name, metatable_key, x);
				return shim;
			}
			else {
				set(name, target);
				return target;
			}
		}

		template <typename T, bool read_only = true>
		table new_enum(const string_view& name, std::initializer_list<std::pair<string_view, T>> items) {
			table target = create(static_cast<int>(items.size()), static_cast<int>(0));
			for (const auto& kvp : items) {
				target.set(kvp.first, kvp.second);
			}
			if (read_only) {
				table x = create_with(
					meta_function::new_index, detail::fail_on_newindex,
					meta_function::index, target);
				table shim = create_named(name, metatable_key, x);
				return shim;
			}
			else {
				set(name, target);
				return target;
			}
		}

		template <typename Fx>
		void for_each(Fx&& fx) const {
			typedef meta::is_invokable<Fx(std::pair<object, object>)> is_paired;
			for_each(is_paired(), std::forward<Fx>(fx));
		}

		size_t size() const {
			auto pp = stack::push_pop(*this);
			lua_len(base_t::lua_state(), -1);
			return stack::pop<size_t>(base_t::lua_state());
		}

		bool empty() const {
			return cbegin() == cend();
		}

		template <typename T>
		proxy<basic_table_core&, T> operator[](T&& key) & {
			return proxy<basic_table_core&, T>(*this, std::forward<T>(key));
		}

		template <typename T>
		proxy<const basic_table_core&, T> operator[](T&& key) const& {
			return proxy<const basic_table_core&, T>(*this, std::forward<T>(key));
		}

		template <typename T>
		proxy<basic_table_core, T> operator[](T&& key) && {
			return proxy<basic_table_core, T>(*this, std::forward<T>(key));
		}

		template <typename Sig, typename Key, typename... Args>
		basic_table_core& set_function(Key&& key, Args&&... args) {
			set_fx(types<Sig>(), std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename Key, typename... Args>
		basic_table_core& set_function(Key&& key, Args&&... args) {
			set_fx(types<>(), std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& add(Args&&... args) {
			auto pp = stack::push_pop(*this);
			(void)detail::swallow{ 0,
				(stack::set_ref(base_t::lua_state(), std::forward<Args>(args)), 0)... };
			return *this;
		}

	private:
		template <typename R, typename... Args, typename Fx, typename Key, typename = std::result_of_t<Fx(Args...)>>
		void set_fx(types<R(Args...)>, Key&& key, Fx&& fx) {
			set_resolved_function<R(Args...)>(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, meta::enable<meta::is_specialization_of<meta::unqualified_t<Fx>, overload_set>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx) {
			set(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, typename... Args, meta::disable<meta::is_specialization_of<meta::unqualified_t<Fx>, overload_set>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference(std::forward<Fx>(fx), std::forward<Args>(args)...));
		}

		template <typename... Sig, typename... Args, typename Key>
		void set_resolved_function(Key&& key, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference<function_sig<Sig...>>(std::forward<Args>(args)...));
		}

	public:
		static inline table create(lua_State* L, int narr = 0, int nrec = 0) {
			lua_createtable(L, narr, nrec);
			table result(L);
			lua_pop(L, 1);
			return result;
		}

		template <typename Key, typename Value, typename... Args>
		static inline table create(lua_State* L, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			lua_createtable(L, narr, nrec);
			table result(L);
			result.set(std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
			lua_pop(L, 1);
			return result;
		}

		template <typename... Args>
		static inline table create_with(lua_State* L, Args&&... args) {
			static_assert(sizeof...(Args) % 2 == 0, "You must have an even number of arguments for a key, value ... list.");
			static const int narr = static_cast<int>(meta::count_2_for_pack<std::is_integral, Args...>::value);
			return create(L, narr, static_cast<int>((sizeof...(Args) / 2) - narr), std::forward<Args>(args)...);
		}

		table create(int narr = 0, int nrec = 0) {
			return create(base_t::lua_state(), narr, nrec);
		}

		template <typename Key, typename Value, typename... Args>
		table create(int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			return create(base_t::lua_state(), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
		}

		template <typename Name>
		table create(Name&& name, int narr = 0, int nrec = 0) {
			table x = create(base_t::lua_state(), narr, nrec);
			this->set(std::forward<Name>(name), x);
			return x;
		}

		template <typename Name, typename Key, typename Value, typename... Args>
		table create(Name&& name, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			table x = create(base_t::lua_state(), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
			this->set(std::forward<Name>(name), x);
			return x;
		}

		template <typename... Args>
		table create_with(Args&&... args) {
			return create_with(base_t::lua_state(), std::forward<Args>(args)...);
		}

		template <typename Name, typename... Args>
		table create_named(Name&& name, Args&&... args) {
			static const int narr = static_cast<int>(meta::count_2_for_pack<std::is_integral, Args...>::value);
			return create(std::forward<Name>(name), narr, (sizeof...(Args) / 2) - narr, std::forward<Args>(args)...);
		}
	};
} // namespace sol

// end of sol/table_core.hpp

namespace sol {
	typedef table_core<false> table;

	namespace stack {
		template <>
		struct getter<metatable_t> {
			static table get(lua_State* L, int index = -1) {
				if (lua_getmetatable(L, index) == 0) {
					return table(L, ref_index(LUA_REFNIL));
				}
				return table(L, -1);
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/table.hpp

// beginning of sol/environment.hpp

namespace sol {

	template <typename base_type>
	struct basic_environment : basic_table<base_type> {
	private:
		typedef basic_table<base_type> base_t;

	public:
		using base_t::lua_state;

		basic_environment() noexcept = default;
		basic_environment(const basic_environment&) = default;
		basic_environment(basic_environment&&) = default;
		basic_environment& operator=(const basic_environment&) = default;
		basic_environment& operator=(basic_environment&&) = default;
		basic_environment(const stack_reference& r)
		: basic_environment(r.lua_state(), r.stack_index()) {
		}
		basic_environment(stack_reference&& r)
		: basic_environment(r.lua_state(), r.stack_index()) {
		}

		basic_environment(lua_State* L, new_table nt)
		: base_t(L, std::move(nt)) {
		}
		template <bool b>
		basic_environment(lua_State* L, new_table t, const basic_reference<b>& fallback)
		: basic_environment(L, std::move(t)) {
			stack_table mt(L, new_table(0, 1));
			mt.set(meta_function::index, fallback);
			this->set(metatable_key, mt);
			mt.pop();
		}

		basic_environment(env_t, const stack_reference& extraction_target)
		: base_t(detail::no_safety, extraction_target.lua_state(), (stack::push_environment_of(extraction_target), -1)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<env_t>(this->lua_state(), -1, handler);
#endif // Safety
			lua_pop(this->lua_state(), 2);
		}
		template <bool b>
		basic_environment(env_t, const basic_reference<b>& extraction_target)
		: base_t(detail::no_safety, extraction_target.lua_state(), (stack::push_environment_of(extraction_target), -1)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<env_t>(this->lua_state(), -1, handler);
#endif // Safety
			lua_pop(this->lua_state(), 2);
		}
		basic_environment(lua_State* L, int index = -1)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_environment>(L, index, handler);
#endif // Safety
		}
		basic_environment(lua_State* L, ref_index index)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_environment>(L, -1, handler);
#endif // Safety
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_environment>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_environment(T&& r) noexcept
		: base_t(detail::no_safety, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_environment<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_environment>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_environment(lua_nil_t r) noexcept
		: base_t(detail::no_safety, r) {
		}

		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_environment(lua_State* L, T&& r) noexcept
			: base_t(detail::no_safety, L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_environment<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_environment>(lua_state(), -1, handler);
			}
#endif // Safety
		}

		template <typename T>
		void set_on(const T& target) const {
			lua_State* L = target.lua_state();
			auto pp = stack::push_pop(target);
#if SOL_LUA_VERSION < 502
			// Use lua_setfenv
			this->push();
			lua_setfenv(L, -2);
#else
			// Use upvalues as explained in Lua 5.2 and beyond's manual
			this->push();
			const char* name = lua_setupvalue(L, -2, 1);
			if (name == nullptr) {
				this->pop();
			}
#endif
		}
	};

	template <typename T, typename E>
	void set_environment(const basic_environment<E>& env, const T& target) {
		env.set_on(target);
	}

	template <typename E = reference, typename T>
	basic_environment<E> get_environment(const T& target) {
		lua_State* L = target.lua_state();
		auto pp = stack::pop_n(L, stack::push_environment_of(target));
		return basic_environment<E>(L, -1);
	}

	struct this_environment {
		optional<environment> env;

		this_environment()
		: env(nullopt) {
		}
		this_environment(environment e)
		: env(std::move(e)) {
		}
		this_environment(const this_environment&) = default;
		this_environment(this_environment&&) = default;
		this_environment& operator=(const this_environment&) = default;
		this_environment& operator=(this_environment&&) = default;

		explicit operator bool() const {
			return static_cast<bool>(env);
		}

		operator optional<environment>&() {
			return env;
		}

		operator const optional<environment>&() const {
			return env;
		}

		operator environment&() {
			return env.value();
		}

		operator const environment&() const {
			return env.value();
		}
	};

	namespace stack {
		template <>
		struct getter<env_t> {
			static environment get(lua_State* L, int index, record& tracking) {
				tracking.use(1);
				return get_environment(stack_reference(L, raw_index(index)));
			}
		};

		template <>
		struct getter<this_environment> {
			static this_environment get(lua_State* L, int, record& tracking) {
				tracking.use(0);
				lua_Debug info;
				// Level 0 means current function (this C function, which may or may not be useful for us?)
				// Level 1 means next call frame up the stack. (Can be nothing if function called directly from C++ with lua_p/call)
				int pre_stack_size = lua_gettop(L);
				if (lua_getstack(L, 1, &info) != 1) {
					if (lua_getstack(L, 0, &info) != 1) {
						lua_settop(L, pre_stack_size);
						return this_environment();
					}
				}
				if (lua_getinfo(L, "f", &info) == 0) {
					lua_settop(L, pre_stack_size);
					return this_environment();
				}

				stack_reference f(L, -1);
				environment env(env_key, f);
				if (!env.valid()) {
					lua_settop(L, pre_stack_size);
					return this_environment();
				}
				return this_environment(std::move(env));
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/environment.hpp

// beginning of sol/load_result.hpp

namespace sol {
	struct load_result : public proxy_base<load_result> {
	private:
		lua_State* L;
		int index;
		int returncount;
		int popcount;
		load_status err;

		template <typename T>
		decltype(auto) tagged_get(types<optional<T>>) const {
			if (!valid()) {
				return optional<T>(nullopt);
			}
			return stack::get<optional<T>>(L, index);
		}

		template <typename T>
		decltype(auto) tagged_get(types<T>) const {
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES != 0
			if (!valid()) {
				type_panic_c_str(L, index, type_of(L, index), type::none);
			}
#endif // Check Argument Safety
			return stack::get<T>(L, index);
		}

		optional<error> tagged_get(types<optional<error>>) const {
			if (valid()) {
				return nullopt;
			}
			return error(detail::direct_error, stack::get<std::string>(L, index));
		}

		error tagged_get(types<error>) const {
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES != 0
			if (valid()) {
				type_panic_c_str(L, index, type_of(L, index), type::none, "expecting an error type (a string, from Lua)");
			}
#endif // Check Argument Safety
			return error(detail::direct_error, stack::get<std::string>(L, index));
		}

	public:
		load_result() = default;
		load_result(lua_State* Ls, int stackindex = -1, int retnum = 0, int popnum = 0, load_status lerr = load_status::ok) noexcept
		: L(Ls), index(stackindex), returncount(retnum), popcount(popnum), err(lerr) {
		}
		load_result(const load_result&) = default;
		load_result& operator=(const load_result&) = default;
		load_result(load_result&& o) noexcept
		: L(o.L), index(o.index), returncount(o.returncount), popcount(o.popcount), err(o.err) {
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.L = nullptr;
			o.index = 0;
			o.returncount = 0;
			o.popcount = 0;
			o.err = load_status::syntax;
		}
		load_result& operator=(load_result&& o) noexcept {
			L = o.L;
			index = o.index;
			returncount = o.returncount;
			popcount = o.popcount;
			err = o.err;
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.L = nullptr;
			o.index = 0;
			o.returncount = 0;
			o.popcount = 0;
			o.err = load_status::syntax;
			return *this;
		}

		load_status status() const noexcept {
			return err;
		}

		bool valid() const noexcept {
			return status() == load_status::ok;
		}

		template <typename T>
		T get() const {
			return tagged_get(types<meta::unqualified_t<T>>());
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) {
#if !defined(__clang__) && defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191200000
			// MSVC is ass sometimes
			return get<protected_function>().call<Ret...>(std::forward<Args>(args)...);
#else
			return get<protected_function>().template call<Ret...>(std::forward<Args>(args)...);
#endif
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}

		lua_State* lua_state() const noexcept {
			return L;
		};
		int stack_index() const noexcept {
			return index;
		};

		~load_result() {
			stack::remove(L, index, popcount);
		}
	};
} // namespace sol

// end of sol/load_result.hpp

// beginning of sol/state_handling.hpp

#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
#endif

namespace sol {
	inline void register_main_thread(lua_State* L) {
#if SOL_LUA_VERSION < 502
		if (L == nullptr) {
			lua_pushnil(L);
			lua_setglobal(L, detail::default_main_thread_name());
			return;
		}
		lua_pushthread(L);
		lua_setglobal(L, detail::default_main_thread_name());
#else
		(void)L;
#endif
	}

	inline int default_at_panic(lua_State* L) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
		(void)L;
		return -1;
#else
		size_t messagesize;
		const char* message = lua_tolstring(L, -1, &messagesize);
		if (message) {
			std::string err(message, messagesize);
			lua_settop(L, 0);
#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
			std::cerr << "[sol2] An error occurred and panic has been invoked: ";
			std::cerr << err;
			std::cerr << std::endl;
#endif
			throw error(err);
		}
		lua_settop(L, 0);
		throw error(std::string("An unexpected error occurred and panic has been invoked"));
#endif // Printing Errors
	}

	inline int default_traceback_error_handler(lua_State* L) {
		std::string msg = "An unknown error has triggered the default error handler";
		optional<string_view> maybetopmsg = stack::check_get<string_view>(L, 1);
		if (maybetopmsg) {
			const string_view& topmsg = maybetopmsg.value();
			msg.assign(topmsg.data(), topmsg.size());
		}
		luaL_traceback(L, L, msg.c_str(), 1);
		optional<string_view> maybetraceback = stack::check_get<string_view>(L, -1);
		if (maybetraceback) {
			const string_view& traceback = maybetraceback.value();
			msg.assign(traceback.data(), traceback.size());
		}
#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
		//std::cerr << "[sol2] An error occurred and was caught in traceback: ";
		//std::cerr << msg;
		//std::cerr << std::endl;
#endif // Printing
		return stack::push(L, msg);
	}

	inline void set_default_state(lua_State* L, lua_CFunction panic_function = &default_at_panic, lua_CFunction traceback_function = c_call<decltype(&default_traceback_error_handler), &default_traceback_error_handler>, exception_handler_function exf = detail::default_exception_handler) {
		lua_atpanic(L, panic_function);
		protected_function::set_default_handler(object(L, in_place, traceback_function));
		set_default_exception_handler(L, exf);
		register_main_thread(L);
		stack::luajit_exception_handler(L);
	}

	inline std::size_t total_memory_used(lua_State* L) {
		std::size_t kb = lua_gc(L, LUA_GCCOUNT, 0);
		kb *= 1024;
		kb += lua_gc(L, LUA_GCCOUNTB, 0);
		return kb;
	}

	inline protected_function_result script_pass_on_error(lua_State*, protected_function_result result) {
		return result;
	}

	inline protected_function_result script_throw_on_error(lua_State*L, protected_function_result result) {
		type t = type_of(L, result.stack_index());
		std::string err = "sol: ";
		err += to_string(result.status());
		err += " error";
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
		std::exception_ptr eptr = std::current_exception();
		if (eptr) {
			err += " with a ";
			try {
				std::rethrow_exception(eptr);
			}
			catch (const std::exception& ex) {
				err += "std::exception -- ";
				err.append(ex.what());
			}
			catch (const std::string& message) {
				err += "thrown message -- ";
				err.append(message);
			}
			catch (const char* message) {
				err += "thrown message -- ";
				err.append(message);
			}
			catch (...) {
				err.append("thrown but unknown type, cannot serialize into error message");
			}
		}
#endif // serialize exception information if possible
		if (t == type::string) {
			err += ": ";
			string_view serr = stack::get<string_view>(L, result.stack_index());
			err.append(serr.data(), serr.size());
		}
#if defined(SOL_PRINT_ERRORS) && SOL_PRINT_ERRORS
		std::cerr << "[sol2] An error occurred and has been passed to an error handler: ";
		std::cerr << err;
		std::cerr << std::endl;
#endif
		// replacing information of stack error into pfr
		int target = result.stack_index();
		if (result.pop_count() > 0) {
			stack::remove(L, target, result.pop_count());
		}
		stack::push(L, err);
		int top = lua_gettop(L);
		int towards = top - target;
		if (towards != 0) {
			lua_rotate(L, top, towards);
		}
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
		return result;
#else
		// just throw our error
		throw error(detail::direct_error, err);
#endif // If exceptions are allowed
	}

	inline protected_function_result script_default_on_error(lua_State* L, protected_function_result pfr) {
#if defined(SOL_DEFAULT_PASS_ON_ERROR) && SOL_DEFAULT_PASS_ON_ERROR
		return script_pass_on_error(L, std::move(pfr));
#else
		return script_throw_on_error(L, std::move(pfr));
#endif
	}
} // namespace sol

// end of sol/state_handling.hpp

namespace sol {

	class state_view {
	private:
		lua_State* L;
		table reg;
		global_table global;

		optional<object> is_loaded_package(const std::string& key) {
			auto loaded = reg.traverse_get<optional<object>>("_LOADED", key);
			bool is53mod = loaded && !(loaded->is<bool>() && !loaded->as<bool>());
			if (is53mod)
				return loaded;
#if SOL_LUA_VERSION <= 501
			auto loaded51 = global.traverse_get<optional<object>>("package", "loaded", key);
			bool is51mod = loaded51 && !(loaded51->is<bool>() && !loaded51->as<bool>());
			if (is51mod)
				return loaded51;
#endif
			return nullopt;
		}

		template <typename T>
		void ensure_package(const std::string& key, T&& sr) {
#if SOL_LUA_VERSION <= 501
			auto pkg = global["package"];
			if (!pkg.valid()) {
				pkg = create_table_with("loaded", create_table_with(key, sr));
			}
			else {
				auto ld = pkg["loaded"];
				if (!ld.valid()) {
					ld = create_table_with(key, sr);
				}
				else {
					ld[key] = sr;
				}
			}
#endif
			auto loaded = reg["_LOADED"];
			if (!loaded.valid()) {
				loaded = create_table_with(key, sr);
			}
			else {
				loaded[key] = sr;
			}
		}

		template <typename Fx>
		object require_core(const std::string& key, Fx&& action, bool create_global = true) {
			optional<object> loaded = is_loaded_package(key);
			if (loaded && loaded->valid())
				return std::move(*loaded);
			action();
			stack_reference sr(L, -1);
			if (create_global)
				set(key, sr);
			ensure_package(key, sr);
			return stack::pop<object>(L);
		}

	public:
		typedef global_table::iterator iterator;
		typedef global_table::const_iterator const_iterator;

		state_view(lua_State* Ls)
		: L(Ls), reg(Ls, LUA_REGISTRYINDEX), global(Ls, detail::global_) {
		}

		state_view(this_state Ls)
		: state_view(Ls.L) {
		}

		lua_State* lua_state() const {
			return L;
		}

		template <typename... Args>
		void open_libraries(Args&&... args) {
			static_assert(meta::all_same<lib, Args...>::value, "all types must be libraries");
			if (sizeof...(args) == 0) {
				luaL_openlibs(L);
				return;
			}

			lib libraries[1 + sizeof...(args)] = {lib::count, std::forward<Args>(args)...};

			for (auto&& library : libraries) {
				switch (library) {
#if SOL_LUA_VERSION <= 501 && defined(SOL_LUAJIT)
				case lib::coroutine:
#endif // luajit opens coroutine base stuff
				case lib::base:
					luaL_requiref(L, "base", luaopen_base, 1);
					lua_pop(L, 1);
					break;
				case lib::package:
					luaL_requiref(L, "package", luaopen_package, 1);
					lua_pop(L, 1);
					break;
#if !defined(SOL_LUAJIT)
				case lib::coroutine:
#if SOL_LUA_VERSION > 501
					luaL_requiref(L, "coroutine", luaopen_coroutine, 1);
					lua_pop(L, 1);
#endif // Lua 5.2+ only
					break;
#endif // Not LuaJIT - comes builtin
				case lib::string:
					luaL_requiref(L, "string", luaopen_string, 1);
					lua_pop(L, 1);
					break;
				case lib::table:
					luaL_requiref(L, "table", luaopen_table, 1);
					lua_pop(L, 1);
					break;
				case lib::math:
					luaL_requiref(L, "math", luaopen_math, 1);
					lua_pop(L, 1);
					break;
				case lib::bit32:
#ifdef SOL_LUAJIT
					luaL_requiref(L, "bit32", luaopen_bit, 1);
					lua_pop(L, 1);
#elif (SOL_LUA_VERSION == 502) || defined(LUA_COMPAT_BITLIB) || defined(LUA_COMPAT_5_2)
					luaL_requiref(L, "bit32", luaopen_bit32, 1);
					lua_pop(L, 1);
#else
#endif // Lua 5.2 only (deprecated in 5.3 (503)) (Can be turned on with Compat flags)
					break;
				case lib::io:
					luaL_requiref(L, "io", luaopen_io, 1);
					lua_pop(L, 1);
					break;
				case lib::os:
					luaL_requiref(L, "os", luaopen_os, 1);
					lua_pop(L, 1);
					break;
				case lib::debug:
					luaL_requiref(L, "debug", luaopen_debug, 1);
					lua_pop(L, 1);
					break;
				case lib::utf8:
#if SOL_LUA_VERSION > 502 && !defined(SOL_LUAJIT)
					luaL_requiref(L, "utf8", luaopen_utf8, 1);
					lua_pop(L, 1);
#endif // Lua 5.3+ only
					break;
				case lib::ffi:
#ifdef SOL_LUAJIT
					luaL_requiref(L, "ffi", luaopen_ffi, 1);
					lua_pop(L, 1);
#endif // LuaJIT only
					break;
				case lib::jit:
#ifdef SOL_LUAJIT
					luaL_requiref(L, "jit", luaopen_jit, 0);
					lua_pop(L, 1);
#endif // LuaJIT Only
					break;
				case lib::count:
				default:
					break;
				}
			}
		}

		object require(const std::string& key, lua_CFunction open_function, bool create_global = true) {
			luaL_requiref(L, key.c_str(), open_function, create_global ? 1 : 0);
			return stack::pop<object>(L);
		}

		object require_script(const std::string& key, const string_view& code, bool create_global = true, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			auto action = [this, &code, &chunkname, &mode]() {
				stack::script(L, code, chunkname, mode);
			};
			return require_core(key, action, create_global);
		}

		object require_file(const std::string& key, const std::string& filename, bool create_global = true, load_mode mode = load_mode::any) {
			auto action = [this, &filename, &mode]() {
				stack::script_file(L, filename, mode);
			};
			return require_core(key, action, create_global);
		}

		template <typename E>
		protected_function_result do_string(const string_view& code, const basic_environment<E>& env, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name(code, chunkname, basechunkname);
			load_status x = static_cast<load_status>(luaL_loadbufferx(L, code.data(), code.size(), chunknametarget, to_string(mode).c_str()));
			if (x != load_status::ok) {
				return protected_function_result(L, absolute_index(L, -1), 0, 1, static_cast<call_status>(x));
			}
			stack_aligned_protected_function pf(L, -1);
			set_environment(env, pf);
			return pf();
		}

		template <typename E>
		protected_function_result do_file(const std::string& filename, const basic_environment<E>& env, load_mode mode = load_mode::any) {
			load_status x = static_cast<load_status>(luaL_loadfilex(L, filename.c_str(), to_string(mode).c_str()));
			if (x != load_status::ok) {
				return protected_function_result(L, absolute_index(L, -1), 0, 1, static_cast<call_status>(x));
			}
			stack_aligned_protected_function pf(L, -1);
			set_environment(env, pf);
			return pf();
		}

		protected_function_result do_string(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name(code, chunkname, basechunkname);
			load_status x = static_cast<load_status>(luaL_loadbufferx(L, code.data(), code.size(), chunknametarget, to_string(mode).c_str()));
			if (x != load_status::ok) {
				return protected_function_result(L, absolute_index(L, -1), 0, 1, static_cast<call_status>(x));
			}
			stack_aligned_protected_function pf(L, -1);
			return pf();
		}

		protected_function_result do_file(const std::string& filename, load_mode mode = load_mode::any) {
			load_status x = static_cast<load_status>(luaL_loadfilex(L, filename.c_str(), to_string(mode).c_str()));
			if (x != load_status::ok) {
				return protected_function_result(L, absolute_index(L, -1), 0, 1, static_cast<call_status>(x));
			}
			stack_aligned_protected_function pf(L, -1);
			return pf();
		}

		template <typename Fx, meta::disable_any<meta::is_string_constructible<meta::unqualified_t<Fx>>, meta::is_specialization_of<meta::unqualified_t<Fx>, basic_environment>> = meta::enabler>
		protected_function_result safe_script(const string_view& code, Fx&& on_error, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			protected_function_result pfr = do_string(code, chunkname, mode);
			if (!pfr.valid()) {
				return on_error(L, std::move(pfr));
			}
			return pfr;
		}

		template <typename Fx, typename E>
		protected_function_result safe_script(const string_view& code, const basic_environment<E>& env, Fx&& on_error, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			protected_function_result pfr = do_string(code, env, chunkname, mode);
			if (!pfr.valid()) {
				return on_error(L, std::move(pfr));
			}
			return pfr;
		}

		template <typename E>
		protected_function_result safe_script(const string_view& code, const basic_environment<E>& env, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, env, script_default_on_error, chunkname, mode);
		}

		protected_function_result safe_script(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, script_default_on_error, chunkname, mode);
		}

		template <typename Fx, meta::disable_any<meta::is_string_constructible<meta::unqualified_t<Fx>>, meta::is_specialization_of<meta::unqualified_t<Fx>, basic_environment>> = meta::enabler>
		protected_function_result safe_script_file(const std::string& filename, Fx&& on_error, load_mode mode = load_mode::any) {
			protected_function_result pfr = do_file(filename, mode);
			if (!pfr.valid()) {
				return on_error(L, std::move(pfr));
			}
			return pfr;
		}

		template <typename Fx, typename E>
		protected_function_result safe_script_file(const std::string& filename, const basic_environment<E>& env, Fx&& on_error, load_mode mode = load_mode::any) {
			protected_function_result pfr = do_file(filename, env, mode);
			if (!pfr.valid()) {
				return on_error(L, std::move(pfr));
			}
			return pfr;
		}

		template <typename E>
		protected_function_result safe_script_file(const std::string& filename, const basic_environment<E>& env, load_mode mode = load_mode::any) {
			return safe_script_file(filename, env, script_default_on_error, mode);
		}

		protected_function_result safe_script_file(const std::string& filename, load_mode mode = load_mode::any) {
			return safe_script_file(filename, script_default_on_error, mode);
		}

		template <typename E>
		unsafe_function_result unsafe_script(const string_view& code, const basic_environment<E>& env, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name(code, chunkname, basechunkname);
			int index = lua_gettop(L);
			if (luaL_loadbufferx(L, code.data(), code.size(), chunknametarget, to_string(mode).c_str())) {
				lua_error(L);
			}
			set_environment(env, stack_reference(L, raw_index(index + 1)));
			if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
				lua_error(L);
			}
			int postindex = lua_gettop(L);
			int returns = postindex - index;
			return unsafe_function_result(L, (std::max)(postindex - (returns - 1), 1), returns);
		}

		unsafe_function_result unsafe_script(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			int index = lua_gettop(L);
			stack::script(L, code, chunkname, mode);
			int postindex = lua_gettop(L);
			int returns = postindex - index;
			return unsafe_function_result(L, (std::max)(postindex - (returns - 1), 1), returns);
		}

		template <typename E>
		unsafe_function_result unsafe_script_file(const std::string& filename, const basic_environment<E>& env, load_mode mode = load_mode::any) {
			int index = lua_gettop(L);
			if (luaL_loadfilex(L, filename.c_str(), to_string(mode).c_str())) {
				lua_error(L);
			}
			set_environment(env, stack_reference(L, raw_index(index + 1)));
			if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
				lua_error(L);
			}
			int postindex = lua_gettop(L);
			int returns = postindex - index;
			return unsafe_function_result(L, (std::max)(postindex - (returns - 1), 1), returns);
		}

		unsafe_function_result unsafe_script_file(const std::string& filename, load_mode mode = load_mode::any) {
			int index = lua_gettop(L);
			stack::script_file(L, filename, mode);
			int postindex = lua_gettop(L);
			int returns = postindex - index;
			return unsafe_function_result(L, (std::max)(postindex - (returns - 1), 1), returns);
		}

		template <typename Fx, meta::disable_any<meta::is_string_constructible<meta::unqualified_t<Fx>>, meta::is_specialization_of<meta::unqualified_t<Fx>, basic_environment>> = meta::enabler>
		protected_function_result script(const string_view& code, Fx&& on_error, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, std::forward<Fx>(on_error), chunkname, mode);
		}

		template <typename Fx, meta::disable_any<meta::is_string_constructible<meta::unqualified_t<Fx>>, meta::is_specialization_of<meta::unqualified_t<Fx>, basic_environment>> = meta::enabler>
		protected_function_result script_file(const std::string& filename, Fx&& on_error, load_mode mode = load_mode::any) {
			return safe_script_file(filename, std::forward<Fx>(on_error), mode);
		}

		template <typename Fx, typename E>
		protected_function_result script(const string_view& code, const basic_environment<E>& env, Fx&& on_error, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, env, std::forward<Fx>(on_error), chunkname, mode);
		}

		template <typename Fx, typename E>
		protected_function_result script_file(const std::string& filename, const basic_environment<E>& env, Fx&& on_error, load_mode mode = load_mode::any) {
			return safe_script_file(filename, env, std::forward<Fx>(on_error), mode);
		}

		protected_function_result script(const string_view& code, const environment& env, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, env, script_default_on_error, chunkname, mode);
		}

		protected_function_result script_file(const std::string& filename, const environment& env, load_mode mode = load_mode::any) {
			return safe_script_file(filename, env, script_default_on_error, mode);
		}

#if defined(SOL_SAFE_FUNCTION) && SOL_SAFE_FUNCTION
		protected_function_result script(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return safe_script(code, chunkname, mode);
		}

		protected_function_result script_file(const std::string& filename, load_mode mode = load_mode::any) {
			return safe_script_file(filename, mode);
		}
#else
		unsafe_function_result script(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return unsafe_script(code, chunkname, mode);
		}

		unsafe_function_result script_file(const std::string& filename, load_mode mode = load_mode::any) {
			return unsafe_script_file(filename, mode);
		}
#endif
		load_result load(const string_view& code, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name(code, chunkname, basechunkname);
			load_status x = static_cast<load_status>(luaL_loadbufferx(L, code.data(), code.size(), chunknametarget, to_string(mode).c_str()));
			return load_result(L, absolute_index(L, -1), 1, 1, x);
		}

		load_result load_buffer(const char* buff, size_t size, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			return load(string_view(buff, size), chunkname, mode);
		}

		load_result load_file(const std::string& filename, load_mode mode = load_mode::any) {
			load_status x = static_cast<load_status>(luaL_loadfilex(L, filename.c_str(), to_string(mode).c_str()));
			return load_result(L, absolute_index(L, -1), 1, 1, x);
		}

		load_result load(lua_Reader reader, void* data, const std::string& chunkname = detail::default_chunk_name(), load_mode mode = load_mode::any) {
			detail::typical_chunk_name_t basechunkname = {};
			const char* chunknametarget = detail::make_chunk_name("lua_Reader", chunkname, basechunkname);
			load_status x = static_cast<load_status>(lua_load(L, reader, data, chunknametarget, to_string(mode).c_str()));
			return load_result(L, absolute_index(L, -1), 1, 1, x);
		}

		iterator begin() const {
			return global.begin();
		}

		iterator end() const {
			return global.end();
		}

		const_iterator cbegin() const {
			return global.cbegin();
		}

		const_iterator cend() const {
			return global.cend();
		}

		global_table globals() const {
			return global;
		}

		table registry() const {
			return reg;
		}

		std::size_t memory_used() const {
			return total_memory_used(lua_state());
		}

		int stack_top() const {
			return stack::top(L);
		}

		int stack_clear() {
			int s = stack_top();
			lua_pop(L, s);
			return s;
		}

		void collect_garbage() {
			lua_gc(lua_state(), LUA_GCCOLLECT, 0);
		}

		operator lua_State*() const {
			return lua_state();
		}

		void set_panic(lua_CFunction panic) {
			lua_atpanic(lua_state(), panic);
		}

		void set_exception_handler(exception_handler_function handler) {
			set_default_exception_handler(lua_state(), handler);
		}

		template <typename... Args, typename... Keys>
		decltype(auto) get(Keys&&... keys) const {
			return global.get<Args...>(std::forward<Keys>(keys)...);
		}

		template <typename T, typename Key>
		decltype(auto) get_or(Key&& key, T&& otherwise) const {
			return global.get_or(std::forward<Key>(key), std::forward<T>(otherwise));
		}

		template <typename T, typename Key, typename D>
		decltype(auto) get_or(Key&& key, D&& otherwise) const {
			return global.get_or<T>(std::forward<Key>(key), std::forward<D>(otherwise));
		}

		template <typename... Args>
		state_view& set(Args&&... args) {
			global.set(std::forward<Args>(args)...);
			return *this;
		}

		template <typename T, typename... Keys>
		decltype(auto) traverse_get(Keys&&... keys) const {
			return global.traverse_get<T>(std::forward<Keys>(keys)...);
		}

		template <typename... Args>
		state_view& traverse_set(Args&&... args) {
			global.traverse_set(std::forward<Args>(args)...);
			return *this;
		}

		template <typename T>
		state_view& set_usertype(usertype<T>& user) {
			return set_usertype(usertype_traits<T>::name(), user);
		}

		template <typename Key, typename T>
		state_view& set_usertype(Key&& key, usertype<T>& user) {
			global.set_usertype(std::forward<Key>(key), user);
			return *this;
		}

		template <typename Class, typename... Args>
		state_view& new_usertype(const std::string& name, Args&&... args) {
			global.new_usertype<Class>(name, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		state_view& new_usertype(const std::string& name, Args&&... args) {
			global.new_usertype<Class, CTor0, CTor...>(name, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename... CArgs, typename... Args>
		state_view& new_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			global.new_usertype<Class>(name, ctor, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename... Args>
		state_view& new_simple_usertype(const std::string& name, Args&&... args) {
			global.new_simple_usertype<Class>(name, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		state_view& new_simple_usertype(const std::string& name, Args&&... args) {
			global.new_simple_usertype<Class, CTor0, CTor...>(name, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename... CArgs, typename... Args>
		state_view& new_simple_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			global.new_simple_usertype<Class>(name, ctor, std::forward<Args>(args)...);
			return *this;
		}

		template <typename Class, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			return global.create_simple_usertype<Class>(std::forward<Args>(args)...);
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			return global.create_simple_usertype<Class, CTor0, CTor...>(std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		simple_usertype<Class> create_simple_usertype(constructors<CArgs...> ctor, Args&&... args) {
			return global.create_simple_usertype<Class>(ctor, std::forward<Args>(args)...);
		}

		template <bool read_only = true, typename... Args>
		state_view& new_enum(const string_view& name, Args&&... args) {
			global.new_enum<read_only>(name, std::forward<Args>(args)...);
			return *this;
		}

		template <typename T, bool read_only = true>
		state_view& new_enum(const string_view& name, std::initializer_list<std::pair<string_view, T>> items) {
			global.new_enum<T, read_only>(name, std::move(items));
			return *this;
		}

		template <typename Fx>
		void for_each(Fx&& fx) {
			global.for_each(std::forward<Fx>(fx));
		}

		template <typename T>
		proxy<global_table&, T> operator[](T&& key) {
			return global[std::forward<T>(key)];
		}

		template <typename T>
		proxy<const global_table&, T> operator[](T&& key) const {
			return global[std::forward<T>(key)];
		}

		template <typename Sig, typename... Args, typename Key>
		state_view& set_function(Key&& key, Args&&... args) {
			global.set_function<Sig>(std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename... Args, typename Key>
		state_view& set_function(Key&& key, Args&&... args) {
			global.set_function(std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename Name>
		table create_table(Name&& name, int narr = 0, int nrec = 0) {
			return global.create(std::forward<Name>(name), narr, nrec);
		}

		template <typename Name, typename Key, typename Value, typename... Args>
		table create_table(Name&& name, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			return global.create(std::forward<Name>(name), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
		}

		template <typename Name, typename... Args>
		table create_named_table(Name&& name, Args&&... args) {
			table x = global.create_with(std::forward<Args>(args)...);
			global.set(std::forward<Name>(name), x);
			return x;
		}

		table create_table(int narr = 0, int nrec = 0) {
			return create_table(lua_state(), narr, nrec);
		}

		template <typename Key, typename Value, typename... Args>
		table create_table(int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			return create_table(lua_state(), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
		}

		template <typename... Args>
		table create_table_with(Args&&... args) {
			return create_table_with(lua_state(), std::forward<Args>(args)...);
		}

		static inline table create_table(lua_State* L, int narr = 0, int nrec = 0) {
			return global_table::create(L, narr, nrec);
		}

		template <typename Key, typename Value, typename... Args>
		static inline table create_table(lua_State* L, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			return global_table::create(L, narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
		}

		template <typename... Args>
		static inline table create_table_with(lua_State* L, Args&&... args) {
			return global_table::create_with(L, std::forward<Args>(args)...);
		}
	};
} // namespace sol

// end of sol/state_view.hpp

// beginning of sol/thread.hpp

namespace sol {
	struct lua_thread_state {
		lua_State* L;

		lua_thread_state(lua_State* Ls)
		: L(Ls) {
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
		operator lua_State*() const noexcept {
			return lua_state();
		}
		lua_State* operator->() const noexcept {
			return lua_state();
		}
	};

	namespace stack {
		template <>
		struct pusher<lua_thread_state> {
			int push(lua_State*, lua_thread_state lts) {
				lua_pushthread(lts.L);
				return 1;
			}
		};

		template <>
		struct getter<lua_thread_state> {
			lua_thread_state get(lua_State* L, int index, record& tracking) {
				tracking.use(1);
				lua_thread_state lts( lua_tothread(L, index) );
				return lts;
			}
		};

		template <>
		struct check_getter<lua_thread_state> {
			template <typename Handler>
			optional<lua_thread_state> get(lua_State* L, int index, Handler&& handler, record& tracking) {
				lua_thread_state lts( lua_tothread(L, index) );
				if (lts.lua_state() == nullptr) {
					handler(L, index, type::thread, type_of(L, index), "value is not a valid thread type");
					return nullopt;
				}
				tracking.use(1);
				return lts;
			}
		};
	} // namespace stack

	template <typename base_t>
	class basic_thread : public base_t {
	public:
		using base_t::lua_state;

		basic_thread() noexcept = default;
		basic_thread(const basic_thread&) = default;
		basic_thread(basic_thread&&) = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_thread>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_thread(T&& r)
		: base_t(std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(const stack_reference& r)
		: basic_thread(r.lua_state(), r.stack_index()){};
		basic_thread(stack_reference&& r)
		: basic_thread(r.lua_state(), r.stack_index()){};
		basic_thread& operator=(const basic_thread&) = default;
		basic_thread& operator=(basic_thread&&) = default;
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_thread(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, int index = -1)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_thread>(L, index, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, ref_index index)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, lua_State* actualthread)
		: basic_thread(L, lua_thread_state{ actualthread }) {
		}
		basic_thread(lua_State* L, this_state actualthread)
		: basic_thread(L, lua_thread_state{ actualthread.L }) {
		}
		basic_thread(lua_State* L, lua_thread_state actualthread)
		: base_t(L, -stack::push(L, actualthread)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
			if (!is_stack_based<base_t>::value) {
				lua_pop(lua_state(), 1);
			}
		}

		state_view state() const {
			return state_view(this->thread_state());
		}

		bool is_main_thread() const {
			return stack::is_main_thread(this->thread_state());
		}

		lua_State* thread_state() const {
			auto pp = stack::push_pop(*this);
			lua_State* lthread = lua_tothread(lua_state(), -1);
			return lthread;
		}

		thread_status status() const {
			lua_State* lthread = thread_state();
			auto lstat = static_cast<thread_status>(lua_status(lthread));
			if (lstat == thread_status::ok) {
				lua_Debug ar;
				if (lua_getstack(lthread, 0, &ar) > 0)
					return thread_status::ok;
				else if (lua_gettop(lthread) == 0)
					return thread_status::dead;
				else
					return thread_status::yielded;
			}
			return lstat;
		}

		basic_thread create() {
			return create(lua_state());
		}

		static basic_thread create(lua_State* L) {
			lua_newthread(L);
			basic_thread result(L);
			if (!is_stack_based<base_t>::value) {
				lua_pop(L, 1);
			}
			return result;
		}
	};

	typedef basic_thread<reference> thread;
	typedef basic_thread<stack_reference> stack_thread;
} // namespace sol

// end of sol/thread.hpp

namespace sol {

	class state : private std::unique_ptr<lua_State, detail::state_deleter>, public state_view {
	private:
		typedef std::unique_ptr<lua_State, detail::state_deleter> unique_base;

	public:
		state(lua_CFunction panic = default_at_panic)
		: unique_base(luaL_newstate()), state_view(unique_base::get()) {
			set_default_state(unique_base::get(), panic);
		}

		state(lua_CFunction panic, lua_Alloc alfunc, void* alpointer = nullptr)
		: unique_base(lua_newstate(alfunc, alpointer)), state_view(unique_base::get()) {
			set_default_state(unique_base::get(), panic);
		}

		state(const state&) = delete;
		state(state&&) = default;
		state& operator=(const state&) = delete;
		state& operator=(state&& that) {
			state_view::operator=(std::move(that));
			unique_base::operator=(std::move(that));
			return *this;
		}

		using state_view::get;

		~state() {
		}
	};
} // namespace sol

// end of sol/state.hpp

// beginning of sol/coroutine.hpp

namespace sol {
	template <typename base_t>
	class basic_coroutine : public base_t {
	public:
		typedef reference handler_t;
		handler_t error_handler;

	private:
		call_status stats = call_status::yielded;

		void luacall(std::ptrdiff_t argcount, std::ptrdiff_t) {
#if SOL_LUA_VERSION >= 504
			int nres;
			stats = static_cast<call_status>(lua_resume(lua_state(), nullptr, static_cast<int>(argcount), &nres));
#else
			stats = static_cast<call_status>(lua_resume(lua_state(), nullptr, static_cast<int>(argcount)));
#endif
		}

		template <std::size_t... I, typename... Ret>
		auto invoke(types<Ret...>, std::index_sequence<I...>, std::ptrdiff_t n) {
			luacall(n, sizeof...(Ret));
			return stack::pop<std::tuple<Ret...>>(lua_state());
		}

		template <std::size_t I, typename Ret>
		Ret invoke(types<Ret>, std::index_sequence<I>, std::ptrdiff_t n) {
			luacall(n, 1);
			return stack::pop<Ret>(lua_state());
		}

		template <std::size_t I>
		void invoke(types<void>, std::index_sequence<I>, std::ptrdiff_t n) {
			luacall(n, 0);
		}

		protected_function_result invoke(types<>, std::index_sequence<>, std::ptrdiff_t n) {
			int firstreturn = 1;
			luacall(n, LUA_MULTRET);
			int poststacksize = lua_gettop(this->lua_state());
			int returncount = poststacksize - (firstreturn - 1);
			if (error()) {
				if (error_handler.valid()) {
					string_view err = stack::get<string_view>(this->lua_state(), poststacksize);
					error_handler.push();
					stack::push(this->lua_state(), err);
					lua_call(lua_state(), 1, 1);
				}
				return protected_function_result(this->lua_state(), lua_absindex(this->lua_state(), -1), 1, returncount, status());
			}
			return protected_function_result(this->lua_state(), firstreturn, returncount, returncount, status());
		}

	public:
		using base_t::lua_state;

		basic_coroutine() = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_coroutine>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<T>>>, meta::neg<std::is_same<base_t, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_coroutine(T&& r) noexcept
		: base_t(std::forward<T>(r)), error_handler(detail::get_default_handler<reference, is_main_threaded<base_t>::value>(r.lua_state())) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_function<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_coroutine>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_coroutine(const basic_coroutine&) = default;
		basic_coroutine& operator=(const basic_coroutine&) = default;
		basic_coroutine(basic_coroutine&&) = default;
		basic_coroutine& operator=(basic_coroutine&&) = default;
		basic_coroutine(const basic_function<base_t>& b)
		: basic_coroutine(b, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(b.lua_state())) {
		}
		basic_coroutine(basic_function<base_t>&& b)
		: basic_coroutine(std::move(b), detail::get_default_handler<reference, is_main_threaded<base_t>::value>(b.lua_state())) {
		}
		basic_coroutine(const basic_function<base_t>& b, handler_t eh)
		: base_t(b), error_handler(std::move(eh)) {
		}
		basic_coroutine(basic_function<base_t>&& b, handler_t eh)
		: base_t(std::move(b)), error_handler(std::move(eh)) {
		}
		basic_coroutine(const stack_reference& r)
		: basic_coroutine(r.lua_state(), r.stack_index(), detail::get_default_handler<reference, is_main_threaded<base_t>::value>(r.lua_state())) {
		}
		basic_coroutine(stack_reference&& r)
		: basic_coroutine(r.lua_state(), r.stack_index(), detail::get_default_handler<reference, is_main_threaded<base_t>::value>(r.lua_state())) {
		}
		basic_coroutine(const stack_reference& r, handler_t eh)
		: basic_coroutine(r.lua_state(), r.stack_index(), std::move(eh)) {
		}
		basic_coroutine(stack_reference&& r, handler_t eh)
		: basic_coroutine(r.lua_state(), r.stack_index(), std::move(eh)) {
		}

		template <typename Super>
		basic_coroutine(const proxy_base<Super>& p)
		: basic_coroutine(p, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(p.lua_state())) {
		}
		template <typename Super>
		basic_coroutine(proxy_base<Super>&& p)
		: basic_coroutine(std::move(p), detail::get_default_handler<reference, is_main_threaded<base_t>::value>(p.lua_state())) {
		}
		template <typename Proxy, typename Handler, meta::enable<std::is_base_of<proxy_base_tag, meta::unqualified_t<Proxy>>, meta::neg<is_lua_index<meta::unqualified_t<Handler>>>> = meta::enabler>
		basic_coroutine(Proxy&& p, Handler&& eh)
		: basic_coroutine(detail::force_cast<base_t>(p), std::forward<Handler>(eh)) {
		}

		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_coroutine(lua_State* L, T&& r)
		: basic_coroutine(L, std::forward<T>(r), detail::get_default_handler<reference, is_main_threaded<base_t>::value>(L)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_coroutine(lua_State* L, T&& r, handler_t eh)
		: base_t(L, std::forward<T>(r)), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_coroutine>(lua_state(), -1, handler);
#endif // Safety
		}

		basic_coroutine(lua_nil_t n)
		: base_t(n), error_handler(n) {
		}

		basic_coroutine(lua_State* L, int index = -1)
		: basic_coroutine(L, index, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(L)) {
		}
		basic_coroutine(lua_State* L, int index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#ifdef SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_coroutine>(L, index, handler);
#endif // Safety
		}
		basic_coroutine(lua_State* L, absolute_index index)
		: basic_coroutine(L, index, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(L)) {
		}
		basic_coroutine(lua_State* L, absolute_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_coroutine>(L, index, handler);
#endif // Safety
		}
		basic_coroutine(lua_State* L, raw_index index)
		: basic_coroutine(L, index, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(L)) {
		}
		basic_coroutine(lua_State* L, raw_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_coroutine>(L, index, handler);
#endif // Safety
		}
		basic_coroutine(lua_State* L, ref_index index)
		: basic_coroutine(L, index, detail::get_default_handler<reference, is_main_threaded<base_t>::value>(L)) {
		}
		basic_coroutine(lua_State* L, ref_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_coroutine>(lua_state(), -1, handler);
#endif // Safety
		}

		call_status status() const noexcept {
			return stats;
		}

		bool error() const noexcept {
			call_status cs = status();
			return cs != call_status::ok && cs != call_status::yielded;
		}

		bool runnable() const noexcept {
			return base_t::valid()
				&& (status() == call_status::yielded);
		}

		explicit operator bool() const noexcept {
			return runnable();
		}

		template <typename... Args>
		protected_function_result operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) operator()(types<Ret...>, Args&&... args) {
			return call<Ret...>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) {
			// some users screw up coroutine.create
			// and try to use it with sol::coroutine without ever calling the first resume in Lua
			// this makes the stack incompatible with other kinds of stacks: protect against this
			// make sure coroutines don't screw us over
			base_t::push();
			int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
			return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount);
		}
	};
} // namespace sol

// end of sol/coroutine.hpp

// beginning of sol/variadic_results.hpp

// beginning of sol/as_returns.hpp

namespace sol {
	template <typename T>
	struct as_returns_t {
		T src;
	};

	template <typename Source>
	auto as_returns(Source&& source) {
		return as_returns_t<std::decay_t<Source>>{ std::forward<Source>(source) };
	}

	namespace stack {
		template <typename T>
		struct pusher<as_returns_t<T>> {
			int push(lua_State* L, const as_returns_t<T>& e) {
				auto& src = detail::unwrap(e.src);
				int p = 0;
				for (const auto& i : src) {
					p += stack::push(L, i);
				}
				return p;
			}
		};
	} // namespace stack
} // namespace sol

// end of sol/as_returns.hpp

namespace sol {

	struct variadic_results : public std::vector<object> {
		using std::vector<object>::vector;
	};

	namespace stack {
		template <>
		struct pusher<variadic_results> {
			int push(lua_State* L, const variadic_results& e) {
				int p = 0;
				for (const auto& i : e) {
					p += stack::push(L, i);
				}
				return p;
			}
		};
	} // namespace stack

} // namespace sol

// end of sol/variadic_results.hpp

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning( push )
#endif // g++

#if defined(SOL_INSIDE_UNREAL) && SOL_INSIDE_UNREAL
#if defined(SOL_INSIDE_UNREAL_REMOVED_CHECK) && SOL_INSIDE_UNREAL_REMOVED_CHECK
#if defined(DO_CHECK) && DO_CHECK
#define check(expr) { if(UNLIKELY(!(expr))) { FDebug::LogAssertFailedMessage( #expr, __FILE__, __LINE__ ); _DebugBreakAndPromptForRemote(); FDebug::AssertFailed( #expr, __FILE__, __LINE__ ); CA_ASSUME(false); } }
#else
#define check(expr) { CA_ASSUME(expr); }
#endif
#endif 
#endif // Unreal Engine 4 Bullshit

#endif // SOL_HPP
// end of sol.hpp

#endif // SOL_SINGLE_INCLUDE_HPP
