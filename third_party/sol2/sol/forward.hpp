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

#ifndef SOL_FORWARD_HPP
#define SOL_FORWARD_HPP

#include "feature_test.hpp"

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

#endif // SOL_FORWARD_HPP
