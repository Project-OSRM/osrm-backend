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

#ifndef SOL_ENVIRONMENT_HPP
#define SOL_ENVIRONMENT_HPP

#include "table.hpp"

namespace sol {

	template <typename base_type>
	struct basic_environment : basic_table<base_type> {
	private:
		typedef basic_table<base_type> base_t;

	public:
		using base_t::lua_state;

		basic_environment() noexcept = default;
		basic_environment(const basic_environment&) = default;
		basic_environment(basic_environment&&) = default;
		basic_environment& operator=(const basic_environment&) = default;
		basic_environment& operator=(basic_environment&&) = default;
		basic_environment(const stack_reference& r)
		: basic_environment(r.lua_state(), r.stack_index()) {
		}
		basic_environment(stack_reference&& r)
		: basic_environment(r.lua_state(), r.stack_index()) {
		}

		basic_environment(lua_State* L, new_table nt)
		: base_t(L, std::move(nt)) {
		}
		template <bool b>
		basic_environment(lua_State* L, new_table t, const basic_reference<b>& fallback)
		: basic_environment(L, std::move(t)) {
			stack_table mt(L, new_table(0, 1));
			mt.set(meta_function::index, fallback);
			this->set(metatable_key, mt);
			mt.pop();
		}

		basic_environment(env_t, const stack_reference& extraction_target)
		: base_t(detail::no_safety, extraction_target.lua_state(), (stack::push_environment_of(extraction_target), -1)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<env_t>(this->lua_state(), -1, handler);
#endif // Safety
			lua_pop(this->lua_state(), 2);
		}
		template <bool b>
		basic_environment(env_t, const basic_reference<b>& extraction_target)
		: base_t(detail::no_safety, extraction_target.lua_state(), (stack::push_environment_of(extraction_target), -1)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<env_t>(this->lua_state(), -1, handler);
#endif // Safety
			lua_pop(this->lua_state(), 2);
		}
		basic_environment(lua_State* L, int index = -1)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_environment>(L, index, handler);
#endif // Safety
		}
		basic_environment(lua_State* L, ref_index index)
		: base_t(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_environment>(L, -1, handler);
#endif // Safety
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_environment>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_environment(T&& r) noexcept
		: base_t(detail::no_safety, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_environment<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_environment>(lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_environment(lua_nil_t r) noexcept
		: base_t(detail::no_safety, r) {
		}

		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_environment(lua_State* L, T&& r) noexcept
			: base_t(detail::no_safety, L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_environment<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_environment>(lua_state(), -1, handler);
			}
#endif // Safety
		}

		template <typename T>
		void set_on(const T& target) const {
			lua_State* L = target.lua_state();
			auto pp = stack::push_pop(target);
#if SOL_LUA_VERSION < 502
			// Use lua_setfenv
			this->push();
			lua_setfenv(L, -2);
#else
			// Use upvalues as explained in Lua 5.2 and beyond's manual
			this->push();
			const char* name = lua_setupvalue(L, -2, 1);
			if (name == nullptr) {
				this->pop();
			}
#endif
		}
	};

	template <typename T, typename E>
	void set_environment(const basic_environment<E>& env, const T& target) {
		env.set_on(target);
	}

	template <typename E = reference, typename T>
	basic_environment<E> get_environment(const T& target) {
		lua_State* L = target.lua_state();
		auto pp = stack::pop_n(L, stack::push_environment_of(target));
		return basic_environment<E>(L, -1);
	}

	struct this_environment {
		optional<environment> env;

		this_environment()
		: env(nullopt) {
		}
		this_environment(environment e)
		: env(std::move(e)) {
		}
		this_environment(const this_environment&) = default;
		this_environment(this_environment&&) = default;
		this_environment& operator=(const this_environment&) = default;
		this_environment& operator=(this_environment&&) = default;

		explicit operator bool() const {
			return static_cast<bool>(env);
		}

		operator optional<environment>&() {
			return env;
		}

		operator const optional<environment>&() const {
			return env;
		}

		operator environment&() {
			return env.value();
		}

		operator const environment&() const {
			return env.value();
		}
	};

	namespace stack {
		template <>
		struct getter<env_t> {
			static environment get(lua_State* L, int index, record& tracking) {
				tracking.use(1);
				return get_environment(stack_reference(L, raw_index(index)));
			}
		};

		template <>
		struct getter<this_environment> {
			static this_environment get(lua_State* L, int, record& tracking) {
				tracking.use(0);
				lua_Debug info;
				// Level 0 means current function (this C function, which may or may not be useful for us?)
				// Level 1 means next call frame up the stack. (Can be nothing if function called directly from C++ with lua_p/call)
				int pre_stack_size = lua_gettop(L);
				if (lua_getstack(L, 1, &info) != 1) {
					if (lua_getstack(L, 0, &info) != 1) {
						lua_settop(L, pre_stack_size);
						return this_environment();
					}
				}
				if (lua_getinfo(L, "f", &info) == 0) {
					lua_settop(L, pre_stack_size);
					return this_environment();
				}

				stack_reference f(L, -1);
				environment env(env_key, f);
				if (!env.valid()) {
					lua_settop(L, pre_stack_size);
					return this_environment();
				}
				return this_environment(std::move(env));
			}
		};
	} // namespace stack
} // namespace sol

#endif // SOL_ENVIRONMENT_HPP
