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

#ifndef SOL_STACK_REFERENCE_HPP
#define SOL_STACK_REFERENCE_HPP

#include "types.hpp"

namespace sol {
	namespace detail {
		inline bool xmovable(lua_State* leftL, lua_State* rightL) {
			if (rightL == nullptr || leftL == nullptr || leftL == rightL) {
				return false;
			}
			const void* leftregistry = lua_topointer(leftL, LUA_REGISTRYINDEX);
			const void* rightregistry = lua_topointer(rightL, LUA_REGISTRYINDEX);
			return leftregistry == rightregistry;
		}
	} // namespace detail

	class stack_reference {
	private:
		lua_State* luastate = nullptr;
		int index = 0;

	protected:
		int registry_index() const noexcept {
			return LUA_NOREF;
		}

	public:
		stack_reference() noexcept = default;
		stack_reference(lua_nil_t) noexcept
		: stack_reference(){};
		stack_reference(lua_State* L, lua_nil_t) noexcept
		: luastate(L), index(0) {
		}
		stack_reference(lua_State* L, int i) noexcept
		: stack_reference(L, absolute_index(L, i)) {
		}
		stack_reference(lua_State* L, absolute_index i) noexcept
		: luastate(L), index(i) {
		}
		stack_reference(lua_State* L, raw_index i) noexcept
		: luastate(L), index(i) {
		}
		stack_reference(lua_State* L, ref_index i) noexcept = delete;
		stack_reference(lua_State* L, const reference& r) noexcept = delete;
		stack_reference(lua_State* L, const stack_reference& r) noexcept
		: luastate(L) {
			if (!r.valid()) {
				index = 0;
				return;
			}
			int i = r.stack_index();
			if (detail::xmovable(lua_state(), r.lua_state())) {
				lua_pushvalue(r.lua_state(), r.index);
				lua_xmove(r.lua_state(), luastate, 1);
				i = absolute_index(luastate, -1);
			}
			index = i;
		}
		stack_reference(stack_reference&& o) noexcept = default;
		stack_reference& operator=(stack_reference&&) noexcept = default;
		stack_reference(const stack_reference&) noexcept = default;
		stack_reference& operator=(const stack_reference&) noexcept = default;

		int push() const noexcept {
			return push(lua_state());
		}

		int push(lua_State* Ls) const noexcept {
			if (lua_state() == nullptr) {
				lua_pushnil(Ls);
				return 1;
			}
			lua_pushvalue(lua_state(), index);
			if (Ls != lua_state()) {
				lua_xmove(lua_state(), Ls, 1);
			}
			return 1;
		}

		void pop() const noexcept {
			pop(lua_state());
		}

		void pop(lua_State* Ls, int n = 1) const noexcept {
			lua_pop(Ls, n);
		}

		int stack_index() const noexcept {
			return index;
		}

		const void* pointer() const noexcept {
			const void* vp = lua_topointer(lua_state(), stack_index());
			return vp;
		}

		type get_type() const noexcept {
			int result = lua_type(lua_state(), index);
			return static_cast<type>(result);
		}

		lua_State* lua_state() const noexcept {
			return luastate;
		}

		bool valid() const noexcept {
			type t = get_type();
			return t != type::lua_nil && t != type::none;
		}
	};

	inline bool operator==(const stack_reference& l, const stack_reference& r) {
		return lua_compare(l.lua_state(), l.stack_index(), r.stack_index(), LUA_OPEQ) == 0;
	}

	inline bool operator!=(const stack_reference& l, const stack_reference& r) {
		return !operator==(l, r);
	}

	inline bool operator==(const stack_reference& lhs, const lua_nil_t&) {
		return !lhs.valid();
	}

	inline bool operator==(const lua_nil_t&, const stack_reference& rhs) {
		return !rhs.valid();
	}

	inline bool operator!=(const stack_reference& lhs, const lua_nil_t&) {
		return lhs.valid();
	}

	inline bool operator!=(const lua_nil_t&, const stack_reference& rhs) {
		return rhs.valid();
	}
} // namespace sol

#endif // SOL_STACK_REFERENCE_HPP
