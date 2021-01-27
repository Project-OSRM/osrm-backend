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

#ifndef SOL_UNSAFE_FUNCTION_RESULT_HPP
#define SOL_UNSAFE_FUNCTION_RESULT_HPP

#include "reference.hpp"
#include "tuple.hpp"
#include "stack.hpp"
#include "proxy_base.hpp"
#include "stack_iterator.hpp"
#include "stack_proxy.hpp"
#include <cstdint>

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

#endif // SOL_UNSAFE_FUNCTION_RESULT_HPP
