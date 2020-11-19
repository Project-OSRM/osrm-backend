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

#ifndef SOL_PROTECTED_FUNCTION_HPP
#define SOL_PROTECTED_FUNCTION_HPP

#include "reference.hpp"
#include "stack.hpp"
#include "protected_function_result.hpp"
#include "unsafe_function.hpp"
#include "protected_handler.hpp"
#include <cstdint>
#include <algorithm>

namespace sol {
	template <typename base_t, bool aligned = false, typename handler_t = reference>
	class basic_protected_function : public base_t {
	public:
		typedef is_stack_based<handler_t> is_stack_handler;

		static handler_t get_default_handler(lua_State* L) {
			return detail::get_default_handler<handler_t, is_main_threaded<base_t>::value>(L);
		}

		template <typename T>
		static void set_default_handler(const T& ref) {
			detail::set_default_handler(ref.lua_state(), ref);
		}

	private:
		template <bool b>
		call_status luacall(std::ptrdiff_t argcount, std::ptrdiff_t resultcount, detail::protected_handler<b, handler_t>& h) const {
			return static_cast<call_status>(lua_pcall(lua_state(), static_cast<int>(argcount), static_cast<int>(resultcount), h.stackindex));
		}

		template <std::size_t... I, bool b, typename... Ret>
		auto invoke(types<Ret...>, std::index_sequence<I...>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, sizeof...(Ret), h);
			return stack::pop<std::tuple<Ret...>>(lua_state());
		}

		template <std::size_t I, bool b, typename Ret>
		Ret invoke(types<Ret>, std::index_sequence<I>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, 1, h);
			return stack::pop<Ret>(lua_state());
		}

		template <std::size_t I, bool b>
		void invoke(types<void>, std::index_sequence<I>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			luacall(n, 0, h);
		}

		template <bool b>
		protected_function_result invoke(types<>, std::index_sequence<>, std::ptrdiff_t n, detail::protected_handler<b, handler_t>& h) const {
			int stacksize = lua_gettop(lua_state());
			int poststacksize = stacksize;
			int firstreturn = 1;
			int returncount = 0;
			call_status code = call_status::ok;
#if !defined(SOL_NO_EXCEPTIONS) || !SOL_NO_EXCEPTIONS
			auto onexcept = [&](optional<const std::exception&> maybe_ex, const char* error) {
				h.stackindex = 0;
				if (b) {
					h.target.push();
					detail::call_exception_handler(lua_state(), maybe_ex, error);
					lua_call(lua_state(), 1, 1);
				}
				else {
					detail::call_exception_handler(lua_state(), maybe_ex, error);
				}
			};
			(void)onexcept;
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION) || (defined(SOL_LUAJIT) && SOL_LUAJIT)
			try {
#endif // Safe Exception Propagation
#endif // No Exceptions
				firstreturn = (std::max)(1, static_cast<int>(stacksize - n - static_cast<int>(h.valid())));
				code = luacall(n, LUA_MULTRET, h);
				poststacksize = lua_gettop(lua_state()) - static_cast<int>(h.valid());
				returncount = poststacksize - (firstreturn - 1);
#ifndef SOL_NO_EXCEPTIONS
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION) || (defined(SOL_LUAJIT) && SOL_LUAJIT)
			}
			// Handle C++ errors thrown from C++ functions bound inside of lua
			catch (const char* error) {
				onexcept(optional<const std::exception&>(nullopt), error);
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
			catch (const std::string& error) {
				onexcept(optional<const std::exception&>(nullopt), error.c_str());
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
			catch (const std::exception& error) {
				onexcept(optional<const std::exception&>(error), error.what());
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
#if (!defined(SOL_EXCEPTIONS_SAFE_PROPAGATION) || !SOL_NO_EXCEPTIONS_SAFE_PROPAGATION)
			// LuaJIT cannot have the catchall when the safe propagation is on
			// but LuaJIT will swallow all C++ errors 
			// if we don't at least catch std::exception ones
			catch (...) {
				onexcept(optional<const std::exception&>(nullopt), "caught (...) unknown error during protected_function call");
				firstreturn = lua_gettop(lua_state());
				return protected_function_result(lua_state(), firstreturn, 0, 1, call_status::runtime);
			}
#endif // LuaJIT
#else
			// do not handle exceptions: they can be propogated into C++ and keep all type information / rich information
#endif // Safe Exception Propagation
#endif // Exceptions vs. No Exceptions
			return protected_function_result(lua_state(), firstreturn, returncount, returncount, code);
		}

	public:
		using base_t::lua_state;

		handler_t error_handler;

		basic_protected_function() = default;
		template <typename T, meta::enable<meta::neg<std::is_same<meta::unqualified_t<T>, basic_protected_function>>, meta::neg<std::is_base_of<proxy_base_tag, meta::unqualified_t<T>>>, meta::neg<std::is_same<base_t, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(T&& r) noexcept
		: base_t(std::forward<T>(r)), error_handler(get_default_handler(r.lua_state())) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_function<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_protected_function>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_protected_function(const basic_protected_function&) = default;
		basic_protected_function& operator=(const basic_protected_function&) = default;
		basic_protected_function(basic_protected_function&&) = default;
		basic_protected_function& operator=(basic_protected_function&&) = default;
		basic_protected_function(const basic_function<base_t>& b)
		: basic_protected_function(b, get_default_handler(b.lua_state())) {
		}
		basic_protected_function(basic_function<base_t>&& b)
		: basic_protected_function(std::move(b), get_default_handler(b.lua_state())) {
		}
		basic_protected_function(const basic_function<base_t>& b, handler_t eh)
		: base_t(b), error_handler(std::move(eh)) {
		}
		basic_protected_function(basic_function<base_t>&& b, handler_t eh)
		: base_t(std::move(b)), error_handler(std::move(eh)) {
		}
		basic_protected_function(const stack_reference& r)
		: basic_protected_function(r.lua_state(), r.stack_index(), get_default_handler(r.lua_state())) {
		}
		basic_protected_function(stack_reference&& r)
		: basic_protected_function(r.lua_state(), r.stack_index(), get_default_handler(r.lua_state())) {
		}
		basic_protected_function(const stack_reference& r, handler_t eh)
		: basic_protected_function(r.lua_state(), r.stack_index(), std::move(eh)) {
		}
		basic_protected_function(stack_reference&& r, handler_t eh)
		: basic_protected_function(r.lua_state(), r.stack_index(), std::move(eh)) {
		}

		template <typename Super>
		basic_protected_function(const proxy_base<Super>& p)
		: basic_protected_function(p, get_default_handler(p.lua_state())) {
		}
		template <typename Super>
		basic_protected_function(proxy_base<Super>&& p)
		: basic_protected_function(std::move(p), get_default_handler(p.lua_state())) {
		}
		template <typename Proxy, typename Handler, meta::enable<std::is_base_of<proxy_base_tag, meta::unqualified_t<Proxy>>, meta::neg<is_lua_index<meta::unqualified_t<Handler>>>> = meta::enabler>
		basic_protected_function(Proxy&& p, Handler&& eh)
		: basic_protected_function(detail::force_cast<base_t>(p), std::forward<Handler>(eh)) {
		}

		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(lua_State* L, T&& r)
		: basic_protected_function(L, std::forward<T>(r), get_default_handler(L)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_protected_function(lua_State* L, T&& r, handler_t eh)
		: base_t(L, std::forward<T>(r)), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_protected_function>(lua_state(), -1, handler);
#endif // Safety
		}
		
		basic_protected_function(lua_nil_t n)
			: base_t(n), error_handler(n) {
		}

		basic_protected_function(lua_State* L, int index = -1)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, int index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, absolute_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, absolute_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, raw_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, raw_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_protected_function>(L, index, handler);
#endif // Safety
		}
		basic_protected_function(lua_State* L, ref_index index)
		: basic_protected_function(L, index, get_default_handler(L)) {
		}
		basic_protected_function(lua_State* L, ref_index index, handler_t eh)
		: base_t(L, index), error_handler(std::move(eh)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_protected_function>(lua_state(), -1, handler);
#endif // Safety
		}

		template <typename... Args>
		protected_function_result operator()(Args&&... args) const {
			return call<>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) operator()(types<Ret...>, Args&&... args) const {
			return call<Ret...>(std::forward<Args>(args)...);
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) const {
			if (!aligned) {
				// we do not expect the function to already be on the stack: push it
				if (error_handler.valid()) {
					detail::protected_handler<true, handler_t> h(error_handler);
					base_t::push();
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
				else {
					detail::protected_handler<false, handler_t> h(error_handler);
					base_t::push();
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
			}
			else {
				// the function is already on the stack at the right location
				if (error_handler.valid()) {
					// the handler will be pushed onto the stack manually,
					// since it's not already on the stack this means we need to push our own
					// function on the stack too and swap things to be in-place
					if (!is_stack_handler::value) {
						// so, we need to remove the function at the top and then dump the handler out ourselves
						base_t::push();
					}
					detail::protected_handler<true, handler_t> h(error_handler);
					if (!is_stack_handler::value) {
						lua_replace(lua_state(), -3);
						h.stackindex = lua_absindex(lua_state(), -2);
					}
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
				else {
					detail::protected_handler<false, handler_t> h(error_handler);
					int pushcount = stack::multi_push_reference(lua_state(), std::forward<Args>(args)...);
					return invoke(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), pushcount, h);
				}
			}
		}
	};
} // namespace sol

#endif // SOL_FUNCTION_HPP
