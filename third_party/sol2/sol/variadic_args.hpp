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

#ifndef SOL_VARIADIC_ARGS_HPP
#define SOL_VARIADIC_ARGS_HPP

#include "stack.hpp"
#include "stack_proxy.hpp"
#include "stack_iterator.hpp"
#include <limits>
#include <iterator>

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

#endif // SOL_VARIADIC_ARGS_HPP
