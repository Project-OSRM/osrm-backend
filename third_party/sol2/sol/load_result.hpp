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

#ifndef SOL_LOAD_RESULT_HPP
#define SOL_LOAD_RESULT_HPP

#include "stack.hpp"
#include "function.hpp"
#include "proxy_base.hpp"
#include <cstdint>

namespace sol {
	struct load_result : public proxy_base<load_result> {
	private:
		lua_State* L;
		int index;
		int returncount;
		int popcount;
		load_status err;

		template <typename T>
		decltype(auto) tagged_get(types<optional<T>>) const {
			if (!valid()) {
				return optional<T>(nullopt);
			}
			return stack::get<optional<T>>(L, index);
		}

		template <typename T>
		decltype(auto) tagged_get(types<T>) const {
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES != 0
			if (!valid()) {
				type_panic_c_str(L, index, type_of(L, index), type::none);
			}
#endif // Check Argument Safety
			return stack::get<T>(L, index);
		}

		optional<error> tagged_get(types<optional<error>>) const {
			if (valid()) {
				return nullopt;
			}
			return error(detail::direct_error, stack::get<std::string>(L, index));
		}

		error tagged_get(types<error>) const {
#if defined(SOL_SAFE_PROXIES) && SOL_SAFE_PROXIES != 0
			if (valid()) {
				type_panic_c_str(L, index, type_of(L, index), type::none, "expecting an error type (a string, from Lua)");
			}
#endif // Check Argument Safety
			return error(detail::direct_error, stack::get<std::string>(L, index));
		}

	public:
		load_result() = default;
		load_result(lua_State* Ls, int stackindex = -1, int retnum = 0, int popnum = 0, load_status lerr = load_status::ok) noexcept
		: L(Ls), index(stackindex), returncount(retnum), popcount(popnum), err(lerr) {
		}
		load_result(const load_result&) = default;
		load_result& operator=(const load_result&) = default;
		load_result(load_result&& o) noexcept
		: L(o.L), index(o.index), returncount(o.returncount), popcount(o.popcount), err(o.err) {
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.L = nullptr;
			o.index = 0;
			o.returncount = 0;
			o.popcount = 0;
			o.err = load_status::syntax;
		}
		load_result& operator=(load_result&& o) noexcept {
			L = o.L;
			index = o.index;
			returncount = o.returncount;
			popcount = o.popcount;
			err = o.err;
			// Must be manual, otherwise destructor will screw us
			// return count being 0 is enough to keep things clean
			// but we will be thorough
			o.L = nullptr;
			o.index = 0;
			o.returncount = 0;
			o.popcount = 0;
			o.err = load_status::syntax;
			return *this;
		}

		load_status status() const noexcept {
			return err;
		}

		bool valid() const noexcept {
			return status() == load_status::ok;
		}

		template <typename T>
		T get() const {
			return tagged_get(types<meta::unqualified_t<T>>());
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) {
#if !defined(__clang__) && defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191200000
			// MSVC is ass sometimes
			return get<protected_function>().call<Ret...>(std::forward<Args>(args)...);
#else
			return get<protected_function>().template call<Ret...>(std::forward<Args>(args)...);
#endif
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}

		lua_State* lua_state() const noexcept {
			return L;
		};
		int stack_index() const noexcept {
			return index;
		};

		~load_result() {
			stack::remove(L, index, popcount);
		}
	};
} // namespace sol

#endif // SOL_LOAD_RESULT_HPP
