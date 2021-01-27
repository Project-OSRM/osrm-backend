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

#ifndef SOL_REFERENCE_HPP
#define SOL_REFERENCE_HPP

#include "types.hpp"
#include "stack_reference.hpp"

namespace sol {
	namespace detail {
		inline const char (&default_main_thread_name())[9] {
			static const char name[9] = "sol.\xF0\x9F\x93\x8C";
			return name;
		}
	} // namespace detail

	namespace stack {
		inline void remove(lua_State* L, int rawindex, int count) {
			if (count < 1)
				return;
			int top = lua_gettop(L);
			if (top < 1) {
				return;
			}
			if (rawindex == -count || top == rawindex) {
				// Slice them right off the top
				lua_pop(L, static_cast<int>(count));
				return;
			}

			// Remove each item one at a time using stack operations
			// Probably slower, maybe, haven't benchmarked,
			// but necessary
			int index = lua_absindex(L, rawindex);
			if (index < 0) {
				index = lua_gettop(L) + (index + 1);
			}
			int last = index + count;
			for (int i = index; i < last; ++i) {
				lua_remove(L, index);
			}
		}

		struct push_popper_at {
			lua_State* L;
			int index;
			int count;
			push_popper_at(lua_State* luastate, int index = -1, int count = 1)
			: L(luastate), index(index), count(count) {
			}
			~push_popper_at() {
				remove(L, index, count);
			}
		};

		template <bool top_level>
		struct push_popper_n {
			lua_State* L;
			int t;
			push_popper_n(lua_State* luastate, int x)
			: L(luastate), t(x) {
			}
			push_popper_n(const push_popper_n&) = delete;
			push_popper_n(push_popper_n&&) = default;
			push_popper_n& operator=(const push_popper_n&) = delete;
			push_popper_n& operator=(push_popper_n&&) = default;
			~push_popper_n() {
				lua_pop(L, t);
			}
		};
		template <>
		struct push_popper_n<true> {
			push_popper_n(lua_State*, int) {
			}
		};
		template <bool, typename T, typename = void>
		struct push_popper {
			T t;
			push_popper(T x)
			: t(x) {
				t.push();
			}
			~push_popper() {
				t.pop();
			}
		};
		template <typename T, typename C>
		struct push_popper<true, T, C> {
			push_popper(T) {
			}
			~push_popper() {
			}
		};
		template <typename T>
		struct push_popper<false, T, std::enable_if_t<std::is_base_of<stack_reference, meta::unqualified_t<T>>::value>> {
			push_popper(T) {
			}
			~push_popper() {
			}
		};

		template <bool top_level = false, typename T>
		push_popper<top_level, T> push_pop(T&& x) {
			return push_popper<top_level, T>(std::forward<T>(x));
		}
		template <typename T>
		push_popper_at push_pop_at(T&& x) {
			int c = x.push();
			lua_State* L = x.lua_state();
			return push_popper_at(L, lua_absindex(L, -c), c);
		}
		template <bool top_level = false>
		push_popper_n<top_level> pop_n(lua_State* L, int x) {
			return push_popper_n<top_level>(L, x);
		}
	} // namespace stack

	inline lua_State* main_thread(lua_State* L, lua_State* backup_if_unsupported = nullptr) {
#if SOL_LUA_VERSION < 502
		if (L == nullptr)
			return backup_if_unsupported;
		lua_getglobal(L, detail::default_main_thread_name());
		auto pp = stack::pop_n(L, 1);
		if (type_of(L, -1) == type::thread) {
			return lua_tothread(L, -1);
		}
		return backup_if_unsupported;
#else
		if (L == nullptr)
			return backup_if_unsupported;
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
		lua_State* Lmain = lua_tothread(L, -1);
		lua_pop(L, 1);
		return Lmain;
#endif // Lua 5.2+ has the main thread getter
	}

	namespace detail {
		struct global_tag {
		} const global_{};
		struct no_safety_tag {
		} const no_safety{};

		template <bool b>
		inline lua_State* pick_main_thread(lua_State* L, lua_State* backup_if_unsupported = nullptr) {
			(void)L;
			(void)backup_if_unsupported;
			if (b) {
				return main_thread(L, backup_if_unsupported);
			}
			return L;
		}
	} // namespace detail

	template <bool main_only = false>
	class basic_reference {
	private:
		template <bool o_main_only>
		friend class basic_reference;
		lua_State* luastate = nullptr; // non-owning
		int ref = LUA_NOREF;

		int copy() const noexcept {
			if (ref == LUA_NOREF)
				return LUA_NOREF;
			push();
			return luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}

		template <bool r_main_only>
		void copy_assign(const basic_reference<r_main_only>& r) {
			if (valid()) {
				deref();
			}
			if (r.ref == LUA_REFNIL) {
				luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF) {
				luastate = r.luastate;
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
			ref = r.copy();
		}

		template <bool r_main_only>
		void move_assign(basic_reference<r_main_only>&& r) {
			if (valid()) {
				deref();
			}
			if (r.ref == LUA_REFNIL) {
				luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF) {
				luastate = r.luastate;
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}

			luastate = detail::pick_main_thread < main_only && !r_main_only > (r.lua_state(), r.lua_state());
			ref = r.ref;
			r.ref = LUA_NOREF;
			r.luastate = nullptr;
		}

	protected:
		basic_reference(lua_State* L, detail::global_tag) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			lua_pushglobaltable(lua_state());
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}

		int stack_index() const noexcept {
			return -1;
		}

		void deref() const noexcept {
			luaL_unref(lua_state(), LUA_REGISTRYINDEX, ref);
		}

	public:
		basic_reference() noexcept = default;
		basic_reference(lua_nil_t) noexcept
		: basic_reference() {
		}
		basic_reference(const stack_reference& r) noexcept
		: basic_reference(r.lua_state(), r.stack_index()) {
		}
		basic_reference(stack_reference&& r) noexcept
		: basic_reference(r.lua_state(), r.stack_index()) {
		}
		template <bool r_main_only>
		basic_reference(lua_State* L, const basic_reference<r_main_only>& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (r.ref == LUA_REFNIL) {
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF || lua_state() == nullptr) {
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			ref = r.copy();
		}

		template <bool r_main_only>
		basic_reference(lua_State* L, basic_reference<r_main_only>&& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (r.ref == LUA_REFNIL) {
				ref = LUA_REFNIL;
				return;
			}
			if (r.ref == LUA_NOREF || lua_state() == nullptr) {
				ref = LUA_NOREF;
				return;
			}
			if (detail::xmovable(lua_state(), r.lua_state())) {
				r.push(lua_state());
				ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
				return;
			}
			ref = r.ref;
			r.ref = LUA_NOREF;
			r.luastate = nullptr;
		}

		basic_reference(lua_State* L, const stack_reference& r) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			if (lua_state() == nullptr || r.lua_state() == nullptr || r.get_type() == type::none) {
				ref = LUA_NOREF;
				return;
			}
			if (r.get_type() == type::lua_nil) {
				ref = LUA_REFNIL;
				return;
			}
			if (lua_state() != r.lua_state() && !detail::xmovable(lua_state(), r.lua_state())) {
				return;
			}
			r.push(lua_state());
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, int index = -1) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			// use L to stick with that state's execution stack
			lua_pushvalue(L, index);
			ref = luaL_ref(L, LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, ref_index index) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
			lua_rawgeti(lua_state(), LUA_REGISTRYINDEX, index.index);
			ref = luaL_ref(lua_state(), LUA_REGISTRYINDEX);
		}
		basic_reference(lua_State* L, lua_nil_t) noexcept
		: luastate(detail::pick_main_thread<main_only>(L, L)) {
		}

		~basic_reference() noexcept {
			if (lua_state() == nullptr || ref == LUA_NOREF)
				return;
			deref();
		}

		basic_reference(const basic_reference& o) noexcept
		: luastate(o.lua_state()), ref(o.copy()) {
		}

		basic_reference(basic_reference&& o) noexcept
		: luastate(o.lua_state()), ref(o.ref) {
			o.luastate = nullptr;
			o.ref = LUA_NOREF;
		}

		basic_reference(const basic_reference<!main_only>& o) noexcept
		: luastate(detail::pick_main_thread < main_only && !main_only > (o.lua_state(), o.lua_state())), ref(o.copy()) {
		}

		basic_reference(basic_reference<!main_only>&& o) noexcept
		: luastate(detail::pick_main_thread < main_only && !main_only > (o.lua_state(), o.lua_state())), ref(o.ref) {
			o.luastate = nullptr;
			o.ref = LUA_NOREF;
		}

		basic_reference& operator=(basic_reference&& r) noexcept {
			move_assign(std::move(r));
			return *this;
		}

		basic_reference& operator=(const basic_reference& r) noexcept {
			copy_assign(r);
			return *this;
		}

		basic_reference& operator=(basic_reference<!main_only>&& r) noexcept {
			move_assign(std::move(r));
			return *this;
		}

		basic_reference& operator=(const basic_reference<!main_only>& r) noexcept {
			copy_assign(r);
			return *this;
		}

		basic_reference& operator=(const lua_nil_t&) noexcept {
			if (valid()) {
				deref();
			}
			luastate = nullptr;
			ref = LUA_NOREF;
			return *this;
		}

		template <typename Super>
		basic_reference& operator=(proxy_base<Super>&& r);

		template <typename Super>
		basic_reference& operator=(const proxy_base<Super>& r);

		int push() const noexcept {
			return push(lua_state());
		}

		int push(lua_State* Ls) const noexcept {
			if (lua_state() == nullptr) {
				lua_pushnil(Ls);
				return 1;
			}
			lua_rawgeti(lua_state(), LUA_REGISTRYINDEX, ref);
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

		int registry_index() const noexcept {
			return ref;
		}

		bool valid() const noexcept {
			return !(ref == LUA_NOREF || ref == LUA_REFNIL);
		}

		const void* pointer() const noexcept {
			int si = push();
			const void* vp = lua_topointer(lua_state(), -si);
			lua_pop(this->lua_state(), si);
			return vp;
		}

		explicit operator bool() const noexcept {
			return valid();
		}

		type get_type() const noexcept {
			auto pp = stack::push_pop(*this);
			int result = lua_type(lua_state(), -1);
			return static_cast<type>(result);
		}

		lua_State* lua_state() const noexcept {
			return luastate;
		}
	};

	template <bool lb, bool rb>
	inline bool operator==(const basic_reference<lb>& l, const basic_reference<rb>& r) {
		auto ppl = stack::push_pop(l);
		auto ppr = stack::push_pop(r);
		return lua_compare(l.lua_state(), -1, -2, LUA_OPEQ) == 1;
	}

	template <bool lb, bool rb>
	inline bool operator!=(const basic_reference<lb>& l, const basic_reference<rb>& r) {
		return !operator==(l, r);
	}

	template <bool lb>
	inline bool operator==(const basic_reference<lb>& lhs, const lua_nil_t&) {
		return !lhs.valid();
	}

	template <bool rb>
	inline bool operator==(const lua_nil_t&, const basic_reference<rb>& rhs) {
		return !rhs.valid();
	}

	template <bool lb>
	inline bool operator!=(const basic_reference<lb>& lhs, const lua_nil_t&) {
		return lhs.valid();
	}

	template <bool rb>
	inline bool operator!=(const lua_nil_t&, const basic_reference<rb>& rhs) {
		return rhs.valid();
	}
} // namespace sol

#endif // SOL_REFERENCE_HPP
