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

#ifndef SOL_CONFIG_HPP
#define SOL_CONFIG_HPP

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

// If this is defined, turn on all the safety checks automatically
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

// If we are in debug, turn on certain safety checks
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

// Print any exceptions / errors that occur
// This is normally off due to relying on
// <iostream> to get std::cerr / std::cout
// so it is only defined in debug modes
#if !defined(SOL_PRINT_ERRORS)
// off by default here
#define SOL_PRINT_ERRORS 0
#endif

// The default on_error handler should not throw/assert/print/abort,
// but simply pass the value through back to the user
// problematic due to not having a stable [[nodiscard]] attribute in C++11,
// off by default
#if !defined(SOL_DEFAULT_PASS_ON_ERROR)
// off by default here
#define SOL_DEFAULT_PASS_ON_ERROR 0
#endif

// Interop allows userdata from external systems 
// with external memory layout and metatable names
// to be registered. It costs something to perform 
// the checker / differentiation for sol2 usertypes versus
// external ones however, so this is off by default
#if !defined(SOL_ENABLE_INTEROP)
// off by default here
#define SOL_ENABLE_INTEROP 0
#endif

// Mac OSX and Objective C define a nil keyword
// we cannot use that for the sol::type::nil define
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) || defined(__OBJC__) || defined(nil)
#if !defined(SOL_NO_NIL)
#define SOL_NO_NIL 1
#endif
#endif // avoiding nil defines / keywords

// If we are using boost, there are a number of optimizations we can perform
// Boost has unordered_map with Compatible Key and CompatibleHash
// Boost also has a decent optional, and we check for this later
// and define our optional to use boost in that scenario
#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
#ifndef SOL_UNORDERED_MAP_COMPATIBLE_HASH
#define SOL_UNORDERED_MAP_COMPATIBLE_HASH 1
#endif // SOL_UNORDERED_MAP_COMPATIBLE_HASH
#endif 

// For strings being serialized and
// deserialized from and to utf8
// this controls the size of the buffer we create on the stack,
// in # of utf8 code units
// a buffer of 1KB covers almost all the strings 
// we care about: anything larger and the user should know better
#ifndef SOL_STACK_STRING_OPTIMIZATION_SIZE
#define SOL_STACK_STRING_OPTIMIZATION_SIZE 1024
#endif // Optimized conversion routines using a KB or so off the stack

#endif // SOL_CONFIG_HPP
