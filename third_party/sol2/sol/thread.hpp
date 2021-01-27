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

#ifndef SOL_THREAD_HPP
#define SOL_THREAD_HPP

#include "reference.hpp"
#include "stack.hpp"

namespace sol {
	struct lua_thread_state {
		lua_State* L;

		lua_thread_state(lua_State* Ls)
		: L(Ls) {
		}

		lua_State* lua_state() const noexcept {
			return L;
		}
		operator lua_State*() const noexcept {
			return lua_state();
		}
		lua_State* operator->() const noexcept {
			return lua_state();
		}
	};

	namespace stack {
		template <>
		struct pusher<lua_thread_state> {
			int push(lua_State*, lua_thread_state lts) {
				lua_pushthread(lts.L);
				return 1;
			}
		};

		template <>
		struct getter<lua_thread_state> {
			lua_thread_state get(lua_State* L, int index, record& tracking) {
				tracking.use(1);
				lua_thread_state lts( lua_tothread(L, index) );
				return lts;
			}
		};

		template <>
		struct check_getter<lua_thread_state> {
			template <typename Handler>
			optional<lua_thread_state> get(lua_State* L, int index, Handler&& handler, record& tracking) {
				lua_thread_state lts( lua_tothread(L, index) );
				if (lts.lua_state() == nullptr) {
					handler(L, index, type::thread, type_of(L, index), "value is not a valid thread type");
					return nullopt;
				}
				tracking.use(1);
				return lts;
			}
		};
	} // namespace stack

	template <typename base_t>
	class basic_thread : public base_t {
	public:
		using base_t::lua_state;

		basic_thread() noexcept = default;
		basic_thread(const basic_thread&) = default;
		basic_thread(basic_thread&&) = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_thread>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_thread(T&& r)
		: base_t(std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(const stack_reference& r)
		: basic_thread(r.lua_state(), r.stack_index()){};
		basic_thread(stack_reference&& r)
		: basic_thread(r.lua_state(), r.stack_index()){};
		basic_thread& operator=(const basic_thread&) = default;
		basic_thread& operator=(basic_thread&&) = default;
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_thread(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, int index = -1)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_thread>(L, index, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, ref_index index)
		: base_t(L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_thread(lua_State* L, lua_State* actualthread)
		: basic_thread(L, lua_thread_state{ actualthread }) {
		}
		basic_thread(lua_State* L, this_state actualthread)
		: basic_thread(L, lua_thread_state{ actualthread.L }) {
		}
		basic_thread(lua_State* L, lua_thread_state actualthread)
		: base_t(L, -stack::push(L, actualthread)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_thread>(lua_state(), -1, handler);
#endif // Safety
			if (!is_stack_based<base_t>::value) {
				lua_pop(lua_state(), 1);
			}
		}

		state_view state() const {
			return state_view(this->thread_state());
		}

		bool is_main_thread() const {
			return stack::is_main_thread(this->thread_state());
		}

		lua_State* thread_state() const {
			auto pp = stack::push_pop(*this);
			lua_State* lthread = lua_tothread(lua_state(), -1);
			return lthread;
		}

		thread_status status() const {
			lua_State* lthread = thread_state();
			auto lstat = static_cast<thread_status>(lua_status(lthread));
			if (lstat == thread_status::ok) {
				lua_Debug ar;
				if (lua_getstack(lthread, 0, &ar) > 0)
					return thread_status::ok;
				else if (lua_gettop(lthread) == 0)
					return thread_status::dead;
				else
					return thread_status::yielded;
			}
			return lstat;
		}

		basic_thread create() {
			return create(lua_state());
		}

		static basic_thread create(lua_State* L) {
			lua_newthread(L);
			basic_thread result(L);
			if (!is_stack_based<base_t>::value) {
				lua_pop(L, 1);
			}
			return result;
		}
	};

	typedef basic_thread<reference> thread;
	typedef basic_thread<stack_reference> stack_thread;
} // namespace sol

#endif // SOL_THREAD_HPP
