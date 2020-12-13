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

#ifndef SOL_STATE_HPP
#define SOL_STATE_HPP

#include "state_view.hpp"
#include "thread.hpp"

namespace sol {

	class state : private std::unique_ptr<lua_State, detail::state_deleter>, public state_view {
	private:
		typedef std::unique_ptr<lua_State, detail::state_deleter> unique_base;

	public:
		state(lua_CFunction panic = default_at_panic)
		: unique_base(luaL_newstate()), state_view(unique_base::get()) {
			set_default_state(unique_base::get(), panic);
		}

		state(lua_CFunction panic, lua_Alloc alfunc, void* alpointer = nullptr)
		: unique_base(lua_newstate(alfunc, alpointer)), state_view(unique_base::get()) {
			set_default_state(unique_base::get(), panic);
		}

		state(const state&) = delete;
		state(state&&) = default;
		state& operator=(const state&) = delete;
		state& operator=(state&& that) {
			state_view::operator=(std::move(that));
			unique_base::operator=(std::move(that));
			return *this;
		}

		using state_view::get;

		~state() {
		}
	};
} // namespace sol

#endif // SOL_STATE_HPP
