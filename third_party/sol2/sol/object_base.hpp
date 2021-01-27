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

#ifndef SOL_OBJECT_BASE_HPP
#define SOL_OBJECT_BASE_HPP

#include "reference.hpp"
#include "stack.hpp"

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

#endif // SOL_OBJECT_BASE_HPP
