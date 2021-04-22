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

#ifndef SOL_TABLE_CORE_HPP
#define SOL_TABLE_CORE_HPP

#include "proxy.hpp"
#include "stack.hpp"
#include "function_types.hpp"
#include "usertype.hpp"
#include "table_iterator.hpp"
#include "types.hpp"
#include "object_base.hpp"

namespace sol {
	namespace detail {
		template <std::size_t n>
		struct clean {
			lua_State* L;
			clean(lua_State* luastate)
			: L(luastate) {
			}
			~clean() {
				lua_pop(L, static_cast<int>(n));
			}
		};
		struct ref_clean {
			lua_State* L;
			int& n;
			ref_clean(lua_State* luastate, int& n)
			: L(luastate), n(n) {
			}
			~ref_clean() {
				lua_pop(L, static_cast<int>(n));
			}
		};
		inline int fail_on_newindex(lua_State* L) {
			return luaL_error(L, "sol: cannot modify the elements of an enumeration table");
		}
	} // namespace detail

	const new_table create = new_table{};

	template <bool top_level, typename base_type>
	class basic_table_core : public basic_object_base<base_type> {
		typedef basic_object_base<base_type> base_t;
		friend class state;
		friend class state_view;

		template <typename... Args>
		using is_global = meta::all<meta::boolean<top_level>, meta::is_c_str<Args>...>;

		template <typename Fx>
		void for_each(std::true_type, Fx&& fx) const {
			auto pp = stack::push_pop(*this);
			stack::push(base_t::lua_state(), lua_nil);
			while (lua_next(base_t::lua_state(), -2)) {
				object key(base_t::lua_state(), -2);
				object value(base_t::lua_state(), -1);
				std::pair<object&, object&> keyvalue(key, value);
				auto pn = stack::pop_n(base_t::lua_state(), 1);
				fx(keyvalue);
			}
		}

		template <typename Fx>
		void for_each(std::false_type, Fx&& fx) const {
			auto pp = stack::push_pop(*this);
			stack::push(base_t::lua_state(), lua_nil);
			while (lua_next(base_t::lua_state(), -2)) {
				object key(base_t::lua_state(), -2);
				object value(base_t::lua_state(), -1);
				auto pn = stack::pop_n(base_t::lua_state(), 1);
				fx(key, value);
			}
		}

		template <bool raw, typename Ret0, typename Ret1, typename... Ret, std::size_t... I, typename Keys>
		auto tuple_get(types<Ret0, Ret1, Ret...>, std::index_sequence<0, 1, I...>, Keys&& keys) const
			-> decltype(stack::pop<std::tuple<Ret0, Ret1, Ret...>>(nullptr)) {
			typedef decltype(stack::pop<std::tuple<Ret0, Ret1, Ret...>>(nullptr)) Tup;
			return Tup(
				traverse_get_optional<top_level, raw, Ret0>(meta::is_optional<meta::unqualified_t<Ret0>>(), detail::forward_get<0>(keys)),
				traverse_get_optional<top_level, raw, Ret1>(meta::is_optional<meta::unqualified_t<Ret1>>(), detail::forward_get<1>(keys)),
				traverse_get_optional<top_level, raw, Ret>(meta::is_optional<meta::unqualified_t<Ret>>(), detail::forward_get<I>(keys))...);
		}

		template <bool raw, typename Ret, std::size_t I, typename Keys>
		decltype(auto) tuple_get(types<Ret>, std::index_sequence<I>, Keys&& keys) const {
			return traverse_get_optional<top_level, raw, Ret>(meta::is_optional<meta::unqualified_t<Ret>>(), detail::forward_get<I>(keys));
		}

		template <bool raw, typename Pairs, std::size_t... I>
		void tuple_set(std::index_sequence<I...>, Pairs&& pairs) {
			auto pp = stack::push_pop < top_level && (is_global<decltype(detail::forward_get<I * 2>(pairs))...>::value) > (*this);
			void(detail::swallow{ (stack::set_field<top_level, raw>(base_t::lua_state(),
								   detail::forward_get<I * 2>(pairs),
								   detail::forward_get<I * 2 + 1>(pairs),
								   lua_gettop(base_t::lua_state())),
				0)... });
		}

		template <bool global, bool raw, typename T, typename Key>
		decltype(auto) traverse_get_deep(Key&& key) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			return stack::get<T>(base_t::lua_state());
		}

		template <bool global, bool raw, typename T, typename Key, typename... Keys>
		decltype(auto) traverse_get_deep(Key&& key, Keys&&... keys) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			return traverse_get_deep<false, raw, T>(std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, std::size_t I, typename Key>
		decltype(auto) traverse_get_deep_optional(int& popcount, Key&& key) const {
			typedef decltype(stack::get<T>(base_t::lua_state())) R;
			auto p = stack::probe_get_field<global, raw, T>(base_t::lua_state(), std::forward<Key>(key), lua_gettop(base_t::lua_state()));
			popcount += p.levels;
			if (!p.success)
				return R(nullopt);
			return stack::get<T>(base_t::lua_state());
		}

		template <bool global, bool raw, typename T, std::size_t I, typename Key, typename... Keys>
		decltype(auto) traverse_get_deep_optional(int& popcount, Key&& key, Keys&&... keys) const {
			auto p = I > 0 ? stack::probe_get_field<global>(base_t::lua_state(), std::forward<Key>(key), -1) : stack::probe_get_field<global>(base_t::lua_state(), std::forward<Key>(key), lua_gettop(base_t::lua_state()));
			popcount += p.levels;
			if (!p.success)
				return T(nullopt);
			return traverse_get_deep_optional<false, raw, T, I + 1>(popcount, std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, typename... Keys>
		decltype(auto) traverse_get_optional(std::false_type, Keys&&... keys) const {
			detail::clean<sizeof...(Keys)> c(base_t::lua_state());
			return traverse_get_deep<global, raw, T>(std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename T, typename... Keys>
		decltype(auto) traverse_get_optional(std::true_type, Keys&&... keys) const {
			int popcount = 0;
			detail::ref_clean c(base_t::lua_state(), popcount);
			return traverse_get_deep_optional<global, raw, T, 0>(popcount, std::forward<Keys>(keys)...);
		}

		template <bool global, bool raw, typename Key, typename Value>
		void traverse_set_deep(Key&& key, Value&& value) const {
			stack::set_field<global, raw>(base_t::lua_state(), std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global, bool raw, typename Key, typename... Keys>
		void traverse_set_deep(Key&& key, Keys&&... keys) const {
			stack::get_field<global, raw>(base_t::lua_state(), std::forward<Key>(key));
			traverse_set_deep<false, raw>(std::forward<Keys>(keys)...);
		}

		basic_table_core(lua_State* L, detail::global_tag t) noexcept
		: base_t(L, t) {
		}

	protected:
		basic_table_core(detail::no_safety_tag, lua_nil_t n)
		: base_t(n) {
		}
		basic_table_core(detail::no_safety_tag, lua_State* L, int index)
		: base_t(L, index) {
		}
		basic_table_core(detail::no_safety_tag, lua_State* L, ref_index index)
		: base_t(L, index) {
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_table_core>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(detail::no_safety_tag, T&& r) noexcept
		: base_t(std::forward<T>(r)) {
		}
		template <typename T, meta::enable<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(detail::no_safety_tag, lua_State*L, T&& r) noexcept
		: base_t(L, std::forward<T>(r)) {
		}

	public:
		typedef basic_table_iterator<base_type> iterator;
		typedef iterator const_iterator;

		using base_t::lua_state;

		basic_table_core() noexcept = default;
		basic_table_core(const basic_table_core&) = default;
		basic_table_core(basic_table_core&&) = default;
		basic_table_core& operator=(const basic_table_core&) = default;
		basic_table_core& operator=(basic_table_core&&) = default;
		basic_table_core(const stack_reference& r)
		: basic_table_core(r.lua_state(), r.stack_index()) {
		}
		basic_table_core(stack_reference&& r)
		: basic_table_core(r.lua_state(), r.stack_index()) {
		}
		template <typename T, meta::enable_any<is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(lua_State* L, T&& r)
		: base_t(L, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_table_core>(lua_state(), -1, handler);
#endif // Safety
		}
		basic_table_core(lua_State* L, const new_table& nt)
		: base_t(L, -stack::push(L, nt)) {
			if (!is_stack_based<meta::unqualified_t<base_type>>::value) {
				lua_pop(L, 1);
			}
		}
		basic_table_core(lua_State* L, int index = -1)
		: basic_table_core(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			constructor_handler handler{};
			stack::check<basic_table_core>(L, index, handler);
#endif // Safety
		}
		basic_table_core(lua_State* L, ref_index index)
		: basic_table_core(detail::no_safety, L, index) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			auto pp = stack::push_pop(*this);
			constructor_handler handler{};
			stack::check<basic_table_core>(lua_state(), -1, handler);
#endif // Safety
		}
		template <typename T, meta::enable<meta::neg<meta::any_same<meta::unqualified_t<T>, basic_table_core>>, meta::neg<std::is_same<base_type, stack_reference>>, meta::neg<std::is_same<lua_nil_t, meta::unqualified_t<T>>>, is_lua_reference<meta::unqualified_t<T>>> = meta::enabler>
		basic_table_core(T&& r) noexcept
		: basic_table_core(detail::no_safety, std::forward<T>(r)) {
#if defined(SOL_SAFE_REFERENCES) && SOL_SAFE_REFERENCES
			if (!is_table<meta::unqualified_t<T>>::value) {
				auto pp = stack::push_pop(*this);
				constructor_handler handler{};
				stack::check<basic_table_core>(base_t::lua_state(), -1, handler);
			}
#endif // Safety
		}
		basic_table_core(lua_nil_t r) noexcept
		: basic_table_core(detail::no_safety, r) {
		}

		iterator begin() const {
			return iterator(*this);
		}

		iterator end() const {
			return iterator();
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		template <typename... Ret, typename... Keys>
		decltype(auto) get(Keys&&... keys) const {
			static_assert(sizeof...(Keys) == sizeof...(Ret), "number of keys and number of return types do not match");
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return tuple_get<false>(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), std::forward_as_tuple(std::forward<Keys>(keys)...));
		}

		template <typename T, typename Key>
		decltype(auto) get_or(Key&& key, T&& otherwise) const {
			typedef decltype(get<T>("")) U;
			optional<U> option = get<optional<U>>(std::forward<Key>(key));
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename Key, typename D>
		decltype(auto) get_or(Key&& key, D&& otherwise) const {
			optional<T> option = get<optional<T>>(std::forward<Key>(key));
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		template <typename T, typename... Keys>
		decltype(auto) traverse_get(Keys&&... keys) const {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return traverse_get_optional<top_level, false, T>(meta::is_optional<meta::unqualified_t<T>>(), std::forward<Keys>(keys)...);
		}

		template <typename... Keys>
		basic_table_core& traverse_set(Keys&&... keys) {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			auto pn = stack::pop_n(base_t::lua_state(), static_cast<int>(sizeof...(Keys) - 2));
			traverse_set_deep<top_level, false>(std::forward<Keys>(keys)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& set(Args&&... args) {
			tuple_set<false>(std::make_index_sequence<sizeof...(Args) / 2>(), std::forward_as_tuple(std::forward<Args>(args)...));
			return *this;
		}

		template <typename... Ret, typename... Keys>
		decltype(auto) raw_get(Keys&&... keys) const {
			static_assert(sizeof...(Keys) == sizeof...(Ret), "number of keys and number of return types do not match");
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return tuple_get<true>(types<Ret...>(), std::make_index_sequence<sizeof...(Ret)>(), std::forward_as_tuple(std::forward<Keys>(keys)...));
		}

		template <typename T, typename Key>
		decltype(auto) raw_get_or(Key&& key, T&& otherwise) const {
			typedef decltype(raw_get<T>("")) U;
			optional<U> option = raw_get<optional<U>>(std::forward<Key>(key));
			if (option) {
				return static_cast<U>(option.value());
			}
			return static_cast<U>(std::forward<T>(otherwise));
		}

		template <typename T, typename Key, typename D>
		decltype(auto) raw_get_or(Key&& key, D&& otherwise) const {
			optional<T> option = raw_get<optional<T>>(std::forward<Key>(key));
			if (option) {
				return static_cast<T>(option.value());
			}
			return static_cast<T>(std::forward<D>(otherwise));
		}

		template <typename T, typename... Keys>
		decltype(auto) traverse_raw_get(Keys&&... keys) const {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			return traverse_get_optional<top_level, true, T>(meta::is_optional<meta::unqualified_t<T>>(), std::forward<Keys>(keys)...);
		}

		template <typename... Keys>
		basic_table_core& traverse_raw_set(Keys&&... keys) {
			auto pp = stack::push_pop<is_global<Keys...>::value>(*this);
			auto pn = stack::pop_n(base_t::lua_state(), static_cast<int>(sizeof...(Keys) - 2));
			traverse_set_deep<top_level, true>(std::forward<Keys>(keys)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& raw_set(Args&&... args) {
			tuple_set<true>(std::make_index_sequence<sizeof...(Args) / 2>(), std::forward_as_tuple(std::forward<Args>(args)...));
			return *this;
		}

		template <typename T>
		basic_table_core& set_usertype(usertype<T>& user) {
			return set_usertype(usertype_traits<T>::name(), user);
		}

		template <typename Key, typename T>
		basic_table_core& set_usertype(Key&& key, usertype<T>& user) {
			return set(std::forward<Key>(key), user);
		}

		template <typename Class, typename... Args>
		basic_table_core& new_usertype(const std::string& name, Args&&... args) {
			usertype<Class> utype(std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		basic_table_core& new_usertype(const std::string& name, Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return new_usertype<Class>(name, ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		basic_table_core& new_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			usertype<Class> utype(ctor, std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return new_simple_usertype<Class>(name, ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		basic_table_core& new_simple_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), ctor, std::forward<Args>(args)...);
			set_usertype(name, utype);
			return *this;
		}

		template <typename Class, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), std::forward<Args>(args)...);
			return utype;
		}

		template <typename Class, typename CTor0, typename... CTor, typename... Args>
		simple_usertype<Class> create_simple_usertype(Args&&... args) {
			constructors<types<CTor0, CTor...>> ctor{};
			return create_simple_usertype<Class>(ctor, std::forward<Args>(args)...);
		}

		template <typename Class, typename... CArgs, typename... Args>
		simple_usertype<Class> create_simple_usertype(constructors<CArgs...> ctor, Args&&... args) {
			simple_usertype<Class> utype(base_t::lua_state(), ctor, std::forward<Args>(args)...);
			return utype;
		}

		template <bool read_only = true, typename... Args>
		table new_enum(const string_view& name, Args&&... args) {
			table target = create_with(std::forward<Args>(args)...);
			if (read_only) {
				table x = create_with(
					meta_function::new_index, detail::fail_on_newindex,
					meta_function::index, target);
				table shim = create_named(name, metatable_key, x);
				return shim;
			}
			else {
				set(name, target);
				return target;
			}
		}

		template <typename T, bool read_only = true>
		table new_enum(const string_view& name, std::initializer_list<std::pair<string_view, T>> items) {
			table target = create(static_cast<int>(items.size()), static_cast<int>(0));
			for (const auto& kvp : items) {
				target.set(kvp.first, kvp.second);
			}
			if (read_only) {
				table x = create_with(
					meta_function::new_index, detail::fail_on_newindex,
					meta_function::index, target);
				table shim = create_named(name, metatable_key, x);
				return shim;
			}
			else {
				set(name, target);
				return target;
			}
		}

		template <typename Fx>
		void for_each(Fx&& fx) const {
			typedef meta::is_invokable<Fx(std::pair<object, object>)> is_paired;
			for_each(is_paired(), std::forward<Fx>(fx));
		}

		size_t size() const {
			auto pp = stack::push_pop(*this);
			lua_len(base_t::lua_state(), -1);
			return stack::pop<size_t>(base_t::lua_state());
		}

		bool empty() const {
			return cbegin() == cend();
		}

		template <typename T>
		proxy<basic_table_core&, T> operator[](T&& key) & {
			return proxy<basic_table_core&, T>(*this, std::forward<T>(key));
		}

		template <typename T>
		proxy<const basic_table_core&, T> operator[](T&& key) const& {
			return proxy<const basic_table_core&, T>(*this, std::forward<T>(key));
		}

		template <typename T>
		proxy<basic_table_core, T> operator[](T&& key) && {
			return proxy<basic_table_core, T>(*this, std::forward<T>(key));
		}

		template <typename Sig, typename Key, typename... Args>
		basic_table_core& set_function(Key&& key, Args&&... args) {
			set_fx(types<Sig>(), std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename Key, typename... Args>
		basic_table_core& set_function(Key&& key, Args&&... args) {
			set_fx(types<>(), std::forward<Key>(key), std::forward<Args>(args)...);
			return *this;
		}

		template <typename... Args>
		basic_table_core& add(Args&&... args) {
			auto pp = stack::push_pop(*this);
			(void)detail::swallow{ 0,
				(stack::set_ref(base_t::lua_state(), std::forward<Args>(args)), 0)... };
			return *this;
		}

	private:
		template <typename R, typename... Args, typename Fx, typename Key, typename = std::result_of_t<Fx(Args...)>>
		void set_fx(types<R(Args...)>, Key&& key, Fx&& fx) {
			set_resolved_function<R(Args...)>(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, meta::enable<meta::is_specialization_of<meta::unqualified_t<Fx>, overload_set>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx) {
			set(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, typename... Args, meta::disable<meta::is_specialization_of<meta::unqualified_t<Fx>, overload_set>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference(std::forward<Fx>(fx), std::forward<Args>(args)...));
		}

		template <typename... Sig, typename... Args, typename Key>
		void set_resolved_function(Key&& key, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference<function_sig<Sig...>>(std::forward<Args>(args)...));
		}

	public:
		static inline table create(lua_State* L, int narr = 0, int nrec = 0) {
			lua_createtable(L, narr, nrec);
			table result(L);
			lua_pop(L, 1);
			return result;
		}

		template <typename Key, typename Value, typename... Args>
		static inline table create(lua_State* L, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			lua_createtable(L, narr, nrec);
			table result(L);
			result.set(std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
			lua_pop(L, 1);
			return result;
		}

		template <typename... Args>
		static inline table create_with(lua_State* L, Args&&... args) {
			static_assert(sizeof...(Args) % 2 == 0, "You must have an even number of arguments for a key, value ... list.");
			static const int narr = static_cast<int>(meta::count_2_for_pack<std::is_integral, Args...>::value);
			return create(L, narr, static_cast<int>((sizeof...(Args) / 2) - narr), std::forward<Args>(args)...);
		}

		table create(int narr = 0, int nrec = 0) {
			return create(base_t::lua_state(), narr, nrec);
		}

		template <typename Key, typename Value, typename... Args>
		table create(int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			return create(base_t::lua_state(), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
		}

		template <typename Name>
		table create(Name&& name, int narr = 0, int nrec = 0) {
			table x = create(base_t::lua_state(), narr, nrec);
			this->set(std::forward<Name>(name), x);
			return x;
		}

		template <typename Name, typename Key, typename Value, typename... Args>
		table create(Name&& name, int narr, int nrec, Key&& key, Value&& value, Args&&... args) {
			table x = create(base_t::lua_state(), narr, nrec, std::forward<Key>(key), std::forward<Value>(value), std::forward<Args>(args)...);
			this->set(std::forward<Name>(name), x);
			return x;
		}

		template <typename... Args>
		table create_with(Args&&... args) {
			return create_with(base_t::lua_state(), std::forward<Args>(args)...);
		}

		template <typename Name, typename... Args>
		table create_named(Name&& name, Args&&... args) {
			static const int narr = static_cast<int>(meta::count_2_for_pack<std::is_integral, Args...>::value);
			return create(std::forward<Name>(name), narr, (sizeof...(Args) / 2) - narr, std::forward<Args>(args)...);
		}
	};
} // namespace sol

#endif // SOL_TABLE_CORE_HPP
