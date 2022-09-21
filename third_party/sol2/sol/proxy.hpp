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

#ifndef SOL_PROXY_HPP
#define SOL_PROXY_HPP

#include "traits.hpp"
#include "function.hpp"
#include "protected_function.hpp"
#include "proxy_base.hpp"

namespace sol {
	template <typename Table, typename Key>
	struct proxy : public proxy_base<proxy<Table, Key>> {
	private:
		typedef meta::condition<meta::is_specialization_of<Key, std::tuple>, Key, std::tuple<meta::condition<std::is_array<meta::unqualified_t<Key>>, Key&, meta::unqualified_t<Key>>>> key_type;

		template <typename T, std::size_t... I>
		decltype(auto) tuple_get(std::index_sequence<I...>) const {
			return tbl.template traverse_get<T>(std::get<I>(key)...);
		}

		template <std::size_t... I, typename T>
		void tuple_set(std::index_sequence<I...>, T&& value) {
			tbl.traverse_set(std::get<I>(key)..., std::forward<T>(value));
		}

		auto setup_table(std::true_type) {
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, tbl.stack_index());
			lua_pop(lua_state(), p.levels);
			return p;
		}

		bool is_valid(std::false_type) {
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			lua_pop(lua_state(), p.levels);
			return p;
		}

	public:
		Table tbl;
		key_type key;

		template <typename T>
		proxy(Table table, T&& k)
		: tbl(table), key(std::forward<T>(k)) {
		}

		template <typename T>
		proxy& set(T&& item) {
			tuple_set(std::make_index_sequence<std::tuple_size<meta::unqualified_t<key_type>>::value>(), std::forward<T>(item));
			return *this;
		}

		template <typename... Args>
		proxy& set_function(Args&&... args) {
			tbl.set_function(key, std::forward<Args>(args)...);
			return *this;
		}

		template <typename U, meta::enable<meta::neg<is_lua_reference_or_proxy<meta::unwrap_unqualified_t<U>>>, meta::is_callable<meta::unwrap_unqualified_t<U>>> = meta::enabler>
		proxy& operator=(U&& other) {
			return set_function(std::forward<U>(other));
		}

		template <typename U, meta::disable<meta::neg<is_lua_reference_or_proxy<meta::unwrap_unqualified_t<U>>>, meta::is_callable<meta::unwrap_unqualified_t<U>>> = meta::enabler>
		proxy& operator=(U&& other) {
			return set(std::forward<U>(other));
		}

		template <typename T>
		proxy& operator=(std::initializer_list<T> other) {
			return set(std::move(other));
		}

		template <typename T>
		decltype(auto) get() const {
			return tuple_get<T>(std::make_index_sequence<std::tuple_size<meta::unqualified_t<key_type>>::value>());
		}

		template <typename T>
		decltype(auto) get_or(T&& otherwise) const {
			typedef decltype(get<T>()) U;
			optional<U> option = get<optional<U>>();
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename D>
		decltype(auto) get_or(D&& otherwise) const {
			optional<T> option = get<optional<T>>();
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		
		template <typename T>
		decltype(auto) get_or_create() {
			return get_or_create<T>(new_table());
		}

		template <typename T, typename Otherwise>
		decltype(auto) get_or_create(Otherwise&& other) {
			if (!this->valid()) {
				this->set(std::forward<Otherwise>(other));
			}
			return get<T>();
		}

		template <typename K>
		decltype(auto) operator[](K&& k) const {
			auto keys = meta::tuplefy(key, std::forward<K>(k));
			return proxy<Table, decltype(keys)>(tbl, std::move(keys));
		}

		template <typename... Ret, typename... Args>
		decltype(auto) call(Args&&... args) {
#if !defined(__clang__) && defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191200000
			// MSVC is ass sometimes
			return get<function>().call<Ret...>(std::forward<Args>(args)...);
#else
			return get<function>().template call<Ret...>(std::forward<Args>(args)...);
#endif
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) {
			return call<>(std::forward<Args>(args)...);
		}

		bool valid() const {
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			lua_pop(lua_state(), p.levels);
			return p;
		}

		int push() const noexcept {
			return push(this->lua_state());
		}

		int push(lua_State* L) const noexcept {
			return get<reference>().push(L);
		}

		type get_type() const {
			type t = type::none;
			auto pp = stack::push_pop(tbl);
			auto p = stack::probe_get_field<std::is_same<meta::unqualified_t<Table>, global_table>::value>(lua_state(), key, lua_gettop(lua_state()));
			if (p) {
				t = type_of(lua_state(), -1);
			}
			lua_pop(lua_state(), p.levels);
			return t;
		}

		lua_State* lua_state() const {
			return tbl.lua_state();
		}

		proxy& force() {
			if (this->valid()) {
				this->set(new_table());
			}
			return *this;
		}
	};

	template <typename Table, typename Key, typename T>
	inline bool operator==(T&& left, const proxy<Table, Key>& right) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() == left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator==(const proxy<Table, Key>& right, T&& left) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() == left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator!=(T&& left, const proxy<Table, Key>& right) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() != left;
	}

	template <typename Table, typename Key, typename T>
	inline bool operator!=(const proxy<Table, Key>& right, T&& left) {
		typedef decltype(stack::get<T>(nullptr, 0)) U;
		return right.template get<optional<U>>() != left;
	}

	template <typename Table, typename Key>
	inline bool operator==(lua_nil_t, const proxy<Table, Key>& right) {
		return !right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator==(const proxy<Table, Key>& right, lua_nil_t) {
		return !right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator!=(lua_nil_t, const proxy<Table, Key>& right) {
		return right.valid();
	}

	template <typename Table, typename Key>
	inline bool operator!=(const proxy<Table, Key>& right, lua_nil_t) {
		return right.valid();
	}

	template <bool b>
	template <typename Super>
	basic_reference<b>& basic_reference<b>::operator=(proxy_base<Super>&& r) {
		basic_reference<b> v = r;
		this->operator=(std::move(v));
		return *this;
	}

	template <bool b>
	template <typename Super>
	basic_reference<b>& basic_reference<b>::operator=(const proxy_base<Super>& r) {
		basic_reference<b> v = r;
		this->operator=(std::move(v));
		return *this;
	}

	namespace stack {
		template <typename Table, typename Key>
		struct pusher<proxy<Table, Key>> {
			static int push(lua_State* L, const proxy<Table, Key>& p) {
				reference r = p;
				return r.push(L);
			}
		};
	} // namespace stack
} // namespace sol

#endif // SOL_PROXY_HPP
