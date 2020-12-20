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

#ifndef SOL_USERDATA_HPP
#define SOL_USERDATA_HPP

#include "object_base.hpp"
#include "table.hpp"

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

#endif // SOL_USERDATA_HPP
