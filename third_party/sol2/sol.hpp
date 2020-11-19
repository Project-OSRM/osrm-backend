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
// we'll just let this alone for now
#elif defined _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4503 ) // decorated name horse shit
#pragma warning( disable : 4702 ) // unreachable code
#pragma warning( disable: 4127 ) // 'conditional expression is constant' yeah that's the point your old compilers don't have `if constexpr` you jerk
#pragma warning( disable: 4505 ) // some other nonsense warning
#endif // clang++ vs. g++ vs. VC++

#include "sol/forward.hpp"
#include "sol/state.hpp"
#include "sol/object.hpp"
#include "sol/function.hpp"
#include "sol/protected_function.hpp"
#include "sol/state.hpp"
#include "sol/coroutine.hpp"
#include "sol/variadic_args.hpp"
#include "sol/variadic_results.hpp"

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
