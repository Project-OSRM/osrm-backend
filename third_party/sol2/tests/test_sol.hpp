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

#ifndef SOL_TEST_SOL_HPP
#define SOL_TEST_SOL_HPP

#ifndef SOL_CHECK_ARGUMENTS
#define SOL_CHECK_ARGUMENTS 1
#endif // SOL_CHECK_ARGUMENTS
#ifndef SOL_ENABLE_INTEROP
#define SOL_ENABLE_INTEROP 1
#endif // SOL_ENABLE_INTEROP
// Can't activate until all script/safe_script calls are vetted...
/*#ifndef SOL_DEFAULT_PASS_ON_ERROR
#define SOL_DEFAULT_PASS_ON_ERROR 1
#endif // SOL_DEFAULT_PASS_ON_ERROR
*/

#include <iostream>
#include <cstdlib>

#ifdef TEST_SINGLE
#include <sol_forward.hpp>
#endif // Single
#include <sol.hpp>

struct test_stack_guard {
	lua_State* L;
	int& begintop;
	int& endtop;
	test_stack_guard(lua_State* L, int& begintop, int& endtop)
		: L(L), begintop(begintop), endtop(endtop) {
		begintop = lua_gettop(L);
	}

	void check() {
		if (begintop != endtop) {
			std::abort();
		}
	}

	~test_stack_guard() {
		endtop = lua_gettop(L);
	}
};

#endif // SOL_TEST_SOL_HPP
