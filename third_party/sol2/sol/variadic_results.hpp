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

#ifndef SOL_VARIADIC_RESULTS_HPP
#define SOL_VARIADIC_RESULTS_HPP

#include "stack.hpp"
#include "object.hpp"
#include "as_returns.hpp"
#include <vector>

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

#endif // SOL_VARIADIC_RESULTS_HPP
