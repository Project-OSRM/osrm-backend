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

#ifndef SOL_AS_ARGS_HPP
#define SOL_AS_ARGS_HPP

#include "stack.hpp"

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

#endif // SOL_AS_ARGS_HPP
