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

#ifndef SOL_USERTYPE_METATABLE_HPP
#define SOL_USERTYPE_METATABLE_HPP

#include "wrapper.hpp"
#include "call.hpp"
#include "stack.hpp"
#include "types.hpp"
#include "stack_reference.hpp"
#include "usertype_traits.hpp"
#include "inheritance.hpp"
#include "raii.hpp"
#include "deprecate.hpp"
#include "object.hpp"
#include "container_usertype_metatable.hpp"
#include "usertype_core.hpp"
#include <unordered_map>
#include <cstdio>
#include <sstream>
#include <cassert>
#include <bitset>

namespace sol {

	struct usertype_metatable_core;

	namespace usertype_detail {
		const int metatable_index = 2;
		const int metatable_core_index = 3;
		const int filler_index = 4;
		const int magic_index = 5;

		const int simple_metatable_index = 2;
		const int index_function_index = 3;
		const int newindex_function_index = 4;

		typedef void (*base_walk)(lua_State*, bool&, int&, string_view&);
		typedef int (*member_search)(lua_State*, void*, usertype_metatable_core&, int);

		struct call_information {
			member_search index;
			member_search new_index;
			int runtime_target;

			call_information(member_search index, member_search newindex)
			: call_information(index, newindex, -1) {
			}
			call_information(member_search index, member_search newindex, int runtimetarget)
			: index(index), new_index(newindex), runtime_target(runtimetarget) {
			}
		};
		
		typedef map_t<std::string, call_information> mapping_t;

		struct variable_wrapper {
			virtual int index(lua_State* L) = 0;
			virtual int new_index(lua_State* L) = 0;
			virtual ~variable_wrapper(){};
		};

		template <typename T, typename F>
		struct callable_binding : variable_wrapper {
			F fx;

			template <typename Arg>
			callable_binding(Arg&& arg)
			: fx(std::forward<Arg>(arg)) {
			}

			virtual int index(lua_State* L) override {
				return call_detail::call_wrapped<T, true, true>(L, fx);
			}

			virtual int new_index(lua_State* L) override {
				return call_detail::call_wrapped<T, false, true>(L, fx);
			}
		};

		typedef map_t<std::string, std::unique_ptr<variable_wrapper>> variable_map;
		typedef map_t<std::string, object> function_map;

		struct simple_map {
			const char* metakey;
			variable_map variables;
			function_map functions;
			object index;
			object newindex;
			base_walk indexbaseclasspropogation;
			base_walk newindexbaseclasspropogation;

			simple_map(const char* mkey, base_walk index, base_walk newindex, object i, object ni, variable_map&& vars, function_map&& funcs)
			: metakey(mkey), variables(std::move(vars)), functions(std::move(funcs)), index(std::move(i)), newindex(std::move(ni)), indexbaseclasspropogation(index), newindexbaseclasspropogation(newindex) {
			}
		};
	} // namespace usertype_detail

	struct usertype_metatable_core {
		usertype_detail::mapping_t mapping;
		lua_CFunction indexfunc;
		lua_CFunction newindexfunc;
		std::vector<object> runtime;
		bool mustindex;

		usertype_metatable_core(lua_CFunction ifx, lua_CFunction nifx)
		: mapping(), indexfunc(ifx), newindexfunc(nifx), runtime(), mustindex(false) {
		}

		usertype_metatable_core(const usertype_metatable_core&) = default;
		usertype_metatable_core(usertype_metatable_core&&) = default;
		usertype_metatable_core& operator=(const usertype_metatable_core&) = default;
		usertype_metatable_core& operator=(usertype_metatable_core&&) = default;
	};

	namespace usertype_detail {
		const lua_Integer toplevel_magic = static_cast<lua_Integer>(0xCCC2CCC1);

		inline int is_indexer(string_view s) {
			if (s == to_string(meta_function::index)) {
				return 1;
			}
			else if (s == to_string(meta_function::new_index)) {
				return 2;
			}
			return 0;
		}

		inline int is_indexer(meta_function mf) {
			if (mf == meta_function::index) {
				return 1;
			}
			else if (mf == meta_function::new_index) {
				return 2;
			}
			return 0;
		}

		inline int is_indexer(call_construction) {
			return 0;
		}

		inline int is_indexer(base_classes_tag) {
			return 0;
		}

		inline auto make_string_view(string_view s) {
			return s;
		}

		inline auto make_string_view(call_construction) {
			return string_view(to_string(meta_function::call_function));
		}

		inline auto make_string_view(meta_function mf) {
			return string_view(to_string(mf));
		}

		inline auto make_string_view(base_classes_tag) {
			return string_view(detail::base_class_cast_key());
		}

		template <typename Arg>
		inline std::string make_string(Arg&& arg) {
			string_view s = make_string_view(arg);
			return std::string(s.data(), s.size());
		}

		template <typename N>
		inline luaL_Reg make_reg(N&& n, lua_CFunction f) {
			luaL_Reg l{make_string_view(std::forward<N>(n)).data(), f};
			return l;
		}

		struct registrar {
			registrar() = default;
			registrar(const registrar&) = default;
			registrar(registrar&&) = default;
			registrar& operator=(const registrar&) = default;
			registrar& operator=(registrar&&) = default;
			virtual int push_um(lua_State* L) = 0;
			virtual ~registrar() {
			}
		};

		inline bool is_toplevel(lua_State* L, int index = magic_index) {
			int isnum = 0;
			lua_Integer magic = lua_tointegerx(L, upvalue_index(index), &isnum);
			return isnum != 0 && magic == toplevel_magic;
		}

		inline int runtime_object_call(lua_State* L, void*, usertype_metatable_core& umc, int runtimetarget) {
			std::vector<object>& runtime = umc.runtime;
			object& runtimeobj = runtime[runtimetarget];
			return stack::push(L, runtimeobj);
		}

		template <typename T, bool is_index>
		inline int indexing_fail(lua_State* L) {
			if (is_index) {
#if 0 //defined(SOL_SAFE_USERTYPE) && SOL_SAFE_USERTYPE
				auto maybeaccessor = stack::get<optional<string_view>>(L, is_index ? -1 : -2);
				string_view accessor = maybeaccessor.value_or(string_detail::string_shim("(unknown)"));
				return luaL_error(L, "sol: attempt to index (get) nil value \"%s\" on userdata (bad (misspelled?) key name or does not exist)", accessor.data());
#else
				if (is_toplevel(L)) {
					if (lua_getmetatable(L, 1) == 1) {
						int metatarget = lua_gettop(L);
						stack::get_field(L, stack_reference(L, raw_index(2)), metatarget);
						return 1;
					}
				}
				// With runtime extensibility, we can't hard-error things. They have to return nil, like regular table types, unfortunately...
				return stack::push(L, lua_nil);
#endif
			}
			else {
				auto maybeaccessor = stack::get<optional<string_view>>(L, is_index ? -1 : -2);
				string_view accessor = maybeaccessor.value_or(string_view("(unknown)"));
				return luaL_error(L, "sol: attempt to index (set) nil value \"%s\" on userdata (bad (misspelled?) key name or does not exist)", accessor.data());
			}
		}

		int runtime_new_index(lua_State* L, void*, usertype_metatable_core&, int runtimetarget);

		template <typename T, bool is_simple>
		inline int metatable_new_index(lua_State* L) {
			if (is_toplevel(L)) {
				auto non_indexable = [&L]() {
					if (is_simple) {
						simple_map& sm = stack::get<user<simple_map>>(L, upvalue_index(simple_metatable_index));
						function_map& functions = sm.functions;
						optional<string_view> maybeaccessor = stack::get<optional<string_view>>(L, 2);
						if (!maybeaccessor) {
							return;
						}
						string_view& accessor_view = maybeaccessor.value();
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
						auto preexistingit = functions.find(accessor_view, string_view_hash(), std::equal_to<string_view>());
#else
						std::string accessor(accessor_view.data(), accessor_view.size());
						auto preexistingit = functions.find(accessor);
#endif
						if (preexistingit == functions.cend()) {
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
							std::string accessor(accessor_view.data(), accessor_view.size());
#endif
							functions.emplace_hint(preexistingit, std::move(accessor), object(L, 3));
						}
						else {
							preexistingit->second = object(L, 3);
						}
						return;
					}
					usertype_metatable_core& umc = stack::get<light<usertype_metatable_core>>(L, upvalue_index(metatable_core_index));
					bool mustindex = umc.mustindex;
					if (!mustindex)
						return;
					optional<string_view> maybeaccessor = stack::get<optional<string_view>>(L, 2);
					if (!maybeaccessor) {
						return;
					}
					string_view& accessor_view = maybeaccessor.value();
					mapping_t& mapping = umc.mapping;
					std::vector<object>& runtime = umc.runtime;
					int target = static_cast<int>(runtime.size());
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
					auto preexistingit = mapping.find(accessor_view, string_view_hash(), std::equal_to<string_view>());
#else
					std::string accessor(accessor_view.data(), accessor_view.size());
					auto preexistingit = mapping.find(accessor);
#endif
					if (preexistingit == mapping.cend()) {
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
						std::string accessor(accessor_view.data(), accessor_view.size());
#endif
						runtime.emplace_back(L, 3);
						mapping.emplace_hint(mapping.cend(), std::move(accessor), call_information(&runtime_object_call, &runtime_new_index, target));
					}
					else {
						target = preexistingit->second.runtime_target;
						runtime[target] = object(L, 3);
						preexistingit->second = call_information(&runtime_object_call, &runtime_new_index, target);
					}
				};
				non_indexable();
				for (std::size_t i = 0; i < 4; lua_settop(L, 3), ++i) {
					const char* metakey = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 2:
						metakey = &usertype_traits<T>::metatable()[0];
						luaL_getmetatable(L, metakey);
						break;
					case 3:
					default:
						metakey = &usertype_traits<T>::user_metatable()[0];
						{
							luaL_getmetatable(L, metakey);
							lua_getmetatable(L, -1);
						}
						break;
					}
					int tableindex = lua_gettop(L);
					if (type_of(L, tableindex) == type::lua_nil) {
						continue;
					}
					stack::set_field<false, true>(L, stack_reference(L, raw_index(2)), stack_reference(L, raw_index(3)), tableindex);
				}
				lua_settop(L, 0);
				return 0;
			}
			return indexing_fail<T, false>(L);
		}

		inline int runtime_new_index(lua_State* L, void*, usertype_metatable_core& umc, int runtimetarget) {
			std::vector<object>& runtime = umc.runtime;
			object& runtimeobj = runtime[runtimetarget];
			runtimeobj = object(L, 3);
			return 0;
		}

		template <bool is_index, typename Base>
		static void walk_single_base(lua_State* L, bool& found, int& ret, string_view&) {
			if (found)
				return;
			const char* metakey = &usertype_traits<Base>::metatable()[0];
			const char* gcmetakey = &usertype_traits<Base>::gc_table()[0];
			const char* basewalkkey = is_index ? detail::base_class_index_propogation_key() : detail::base_class_new_index_propogation_key();

			luaL_getmetatable(L, metakey);
			if (type_of(L, -1) == type::lua_nil) {
				lua_pop(L, 1);
				return;
			}

			stack::get_field(L, basewalkkey);
			if (type_of(L, -1) == type::lua_nil) {
				lua_pop(L, 2);
				return;
			}
			lua_CFunction basewalkfunc = stack::pop<lua_CFunction>(L);
			lua_pop(L, 1);

			stack::get_field<true>(L, gcmetakey);
			int value = basewalkfunc(L);
			if (value > -1) {
				found = true;
				ret = value;
			}
		}

		template <bool is_index, typename... Bases>
		static void walk_all_bases(lua_State* L, bool& found, int& ret, string_view& accessor) {
			(void)L;
			(void)found;
			(void)ret;
			(void)accessor;
			(void)detail::swallow{0, (walk_single_base<is_index, Bases>(L, found, ret, accessor), 0)...};
		}
	} // namespace usertype_detail

	template <typename T>
	struct clean_type {
		typedef std::conditional_t<std::is_array<meta::unqualified_t<T>>::value, T&, std::decay_t<T>> type;
	};

	template <typename T>
	using clean_type_t = typename clean_type<T>::type;

	template <typename T, typename IndexSequence, typename... Tn>
	struct usertype_metatable : usertype_detail::registrar {};

	template <typename T, std::size_t... I, typename... Tn>
	struct usertype_metatable<T, std::index_sequence<I...>, Tn...> : usertype_metatable_core, usertype_detail::registrar {
		typedef std::make_index_sequence<sizeof...(I) * 2> indices;
		typedef std::index_sequence<I...> half_indices;
		typedef std::array<luaL_Reg, sizeof...(Tn) / 2 + 1 + 31> regs_t;
		typedef std::tuple<Tn...> RawTuple;
		typedef std::tuple<clean_type_t<Tn>...> Tuple;
		template <std::size_t Idx>
		struct check_binding : is_variable_binding<meta::unqualified_tuple_element_t<Idx, Tuple>> {};
		Tuple functions;
		lua_CFunction destructfunc;
		lua_CFunction callconstructfunc;
		lua_CFunction indexbase;
		lua_CFunction newindexbase;
		usertype_detail::base_walk indexbaseclasspropogation;
		usertype_detail::base_walk newindexbaseclasspropogation;
		void* baseclasscheck;
		void* baseclasscast;
		bool secondarymeta;
		std::bitset<32> properties;

		template <std::size_t Idx, meta::enable<std::is_same<lua_CFunction, meta::unqualified_tuple_element<Idx + 1, RawTuple>>> = meta::enabler>
		lua_CFunction make_func() const {
			return std::get<Idx + 1>(functions);
		}

		template <std::size_t Idx, meta::disable<std::is_same<lua_CFunction, meta::unqualified_tuple_element<Idx + 1, RawTuple>>> = meta::enabler>
		lua_CFunction make_func() const {
			const auto& name = std::get<Idx>(functions);
			return (usertype_detail::make_string_view(name) == "__newindex") ? &call<Idx + 1, false> : &call<Idx + 1, true>;
		}

		static bool contains_variable() {
			typedef meta::any<check_binding<(I * 2 + 1)>...> has_variables;
			return has_variables::value;
		}

		bool contains_index() const {
			bool idx = false;
			(void)detail::swallow{0, ((idx |= (usertype_detail::is_indexer(std::get<I * 2>(functions)) != 0)), 0)...};
			return idx;
		}

		int finish_regs(regs_t& l, int& index) {
			auto prop_fx = [&](meta_function mf) { return !properties[static_cast<int>(mf)]; };
			usertype_detail::insert_default_registrations<T>(l, index, prop_fx);
			if (destructfunc != nullptr) {
				l[index] = luaL_Reg{to_string(meta_function::garbage_collect).c_str(), destructfunc};
				++index;
			}
			return index;
		}

		template <std::size_t Idx, typename F>
		void make_regs(regs_t&, int&, call_construction, F&&) {
			callconstructfunc = call<Idx + 1>;
			secondarymeta = true;
		}

		template <std::size_t, typename... Bases>
		void make_regs(regs_t&, int&, base_classes_tag, bases<Bases...>) {
			static_assert(!meta::any_same<T, Bases...>::value, "base classes cannot list the original class as part of the bases");
			if (sizeof...(Bases) < 1) {
				return;
			}
			mustindex = true;
			(void)detail::swallow{0, ((detail::has_derived<Bases>::value = true), 0)...};

			static_assert(sizeof(void*) <= sizeof(detail::inheritance_check_function), "The size of this data pointer is too small to fit the inheritance checking function: file a bug report.");
			static_assert(sizeof(void*) <= sizeof(detail::inheritance_cast_function), "The size of this data pointer is too small to fit the inheritance checking function: file a bug report.");
			baseclasscheck = (void*)&detail::inheritance<T, Bases...>::type_check;
			baseclasscast = (void*)&detail::inheritance<T, Bases...>::type_cast;
			indexbaseclasspropogation = usertype_detail::walk_all_bases<true, Bases...>;
			newindexbaseclasspropogation = usertype_detail::walk_all_bases<false, Bases...>;
		}

		template <std::size_t Idx, typename N, typename F, typename = std::enable_if_t<!meta::any_same<meta::unqualified_t<N>, base_classes_tag, call_construction>::value>>
		void make_regs(regs_t& l, int& index, N&& n, F&&) {
			if (is_variable_binding<meta::unqualified_t<F>>::value) {
				return;
			}
			luaL_Reg reg = usertype_detail::make_reg(std::forward<N>(n), make_func<Idx>());
			for (std::size_t i = 0; i < properties.size(); ++i) {
				meta_function mf = static_cast<meta_function>(i);
				const std::string& mfname = to_string(mf);
				if (mfname == reg.name) {
					switch (mf) {
					case meta_function::construct:
						if (properties[i]) {
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
							throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
							assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
						}
						break;
					case meta_function::garbage_collect:
						if (destructfunc != nullptr) {
#if !(defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS)
							throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
							assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
						}
						destructfunc = reg.func;
						return;
					case meta_function::index:
						indexfunc = reg.func;
						mustindex = true;
						properties.set(i);
						return;
					case meta_function::new_index:
						newindexfunc = reg.func;
						mustindex = true;
						properties.set(i);
						return;
					default:
						break;
					}
					properties.set(i);
					break;
				}
			}
			l[index] = reg;
			++index;
		}

		template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == sizeof...(Tn)>>
		usertype_metatable(Args&&... args)
		: usertype_metatable_core(&usertype_detail::indexing_fail<T, true>, &usertype_detail::metatable_new_index<T, false>), usertype_detail::registrar(), functions(std::forward<Args>(args)...), destructfunc(nullptr), callconstructfunc(nullptr), indexbase(&core_indexing_call<true>), newindexbase(&core_indexing_call<false>), indexbaseclasspropogation(usertype_detail::walk_all_bases<true>), newindexbaseclasspropogation(usertype_detail::walk_all_bases<false>), baseclasscheck(nullptr), baseclasscast(nullptr), secondarymeta(contains_variable()), properties() {
			properties.reset();
			std::initializer_list<typename usertype_detail::mapping_t::value_type> ilist{{std::pair<std::string, usertype_detail::call_information>(usertype_detail::make_string(std::get<I * 2>(functions)),
				usertype_detail::call_information(&usertype_metatable::real_find_call<I * 2, I * 2 + 1, true>,
					&usertype_metatable::real_find_call<I * 2, I * 2 + 1, false>))}...};
			this->mapping.insert(ilist);
			for (const auto& n : meta_function_names()) {
				this->mapping.erase(n);
			}
			this->mustindex = contains_variable() || contains_index();
		}

		usertype_metatable(const usertype_metatable&) = default;
		usertype_metatable(usertype_metatable&&) = default;
		usertype_metatable& operator=(const usertype_metatable&) = default;
		usertype_metatable& operator=(usertype_metatable&&) = default;

		template <std::size_t I0, std::size_t I1, bool is_index>
		static int real_find_call(lua_State* L, void* um, usertype_metatable_core&, int) {
			auto& f = *static_cast<usertype_metatable*>(um);
			if (is_variable_binding<decltype(std::get<I1>(f.functions))>::value) {
				return real_call_with<I1, is_index, true>(L, f);
			}
			// set up upvalues
			// for a chained call
			int upvalues = 0;
			upvalues += stack::push(L, nullptr);
			upvalues += stack::push(L, light<usertype_metatable>(f));
			auto cfunc = &call<I1, is_index>;
			return stack::push(L, c_closure(cfunc, upvalues));
		}

		template <bool is_index>
		static int real_meta_call(lua_State* L, void* um, int) {
			auto& f = *static_cast<usertype_metatable*>(um);
			return is_index ? f.indexfunc(L) : f.newindexfunc(L);
		}

		template <bool is_index, bool toplevel = false, bool is_meta_bound = false>
		static int core_indexing_call(lua_State* L) {
			usertype_metatable& f = toplevel
				? static_cast<usertype_metatable&>(stack::get<light<usertype_metatable>>(L, upvalue_index(usertype_detail::metatable_index)))
				: static_cast<usertype_metatable&>(stack::pop<user<usertype_metatable>>(L));
			static const int keyidx = -2 + static_cast<int>(is_index);
			if (toplevel && stack::get<type>(L, keyidx) != type::string) {
				return is_index ? f.indexfunc(L) : f.newindexfunc(L);
			}
			int runtime_target = 0;
			usertype_detail::member_search member = nullptr;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view name = stack::get<string_view>(L, keyidx);
				auto memberit = f.mapping.find(name, string_view_hash(), std::equal_to<string_view>());
#else
				std::string name = stack::get<std::string>(L, keyidx);
				auto memberit = f.mapping.find(name);
#endif
				if (memberit != f.mapping.cend()) {
					const usertype_detail::call_information& ci = memberit->second;
					member = is_index ? ci.index : ci.new_index;
					runtime_target = ci.runtime_target;
				}
			}
			if (member != nullptr) {
				return (member)(L, static_cast<void*>(&f), static_cast<usertype_metatable_core&>(f), runtime_target);
			}
			if (is_meta_bound && toplevel && !is_index) {
				return usertype_detail::metatable_new_index<T, false>(L);
			}
			string_view accessor = stack::get<string_view>(L, keyidx);
			int ret = 0;
			bool found = false;
			// Otherwise, we need to do propagating calls through the bases
			if (is_index)
				f.indexbaseclasspropogation(L, found, ret, accessor);
			else
				f.newindexbaseclasspropogation(L, found, ret, accessor);
			if (found) {
				return ret;
			}
			if (is_meta_bound) {
				return is_index ? usertype_detail::indexing_fail<T, is_index>(L) : usertype_detail::metatable_new_index<T, false>(L);
			}
			return toplevel ? (is_index ? f.indexfunc(L) : f.newindexfunc(L)) : -1;
		}

		static int real_index_call(lua_State* L) {
			return core_indexing_call<true, true>(L);
		}

		static int real_new_index_call(lua_State* L) {
			return core_indexing_call<false, true>(L);
		}

		static int real_meta_index_call(lua_State* L) {
			return core_indexing_call<true, true, true>(L);
		}

		static int real_meta_new_index_call(lua_State* L) {
			return core_indexing_call<false, true, true>(L);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int real_call(lua_State* L) {
			usertype_metatable& f = stack::get<light<usertype_metatable>>(L, upvalue_index(usertype_detail::metatable_index));
			return real_call_with<Idx, is_index, is_variable>(L, f);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int real_call_with(lua_State* L, usertype_metatable& um) {
			typedef meta::unqualified_tuple_element_t<Idx - 1, Tuple> K;
			typedef meta::unqualified_tuple_element_t<Idx, Tuple> F;
			static const int boost = !detail::is_non_factory_constructor<F>::value
					&& std::is_same<K, call_construction>::value
				? 1
				: 0;
			auto& f = std::get<Idx>(um.functions);
			return call_detail::call_wrapped<T, is_index, is_variable, boost>(L, f);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_call<Idx, is_index, is_variable>), (&real_call<Idx, is_index, is_variable>)>(L);
		}

		template <std::size_t Idx, bool is_index = true, bool is_variable = false>
		static int call_with(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_call_with<Idx, is_index, is_variable>), (&real_call_with<Idx, is_index, is_variable>)>(L);
		}

		static int index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_index_call), (&real_index_call)>(L);
		}

		static int new_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_new_index_call), (&real_new_index_call)>(L);
		}

		static int meta_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_meta_index_call), (&real_meta_index_call)>(L);
		}

		static int meta_new_index_call(lua_State* L) {
			return detail::typed_static_trampoline<decltype(&real_meta_new_index_call), (&real_meta_new_index_call)>(L);
		}

		virtual int push_um(lua_State* L) override {
			return stack::push(L, std::move(*this));
		}

		~usertype_metatable() override {
		}
	};

	namespace stack {

		template <typename T, std::size_t... I, typename... Args>
		struct pusher<usertype_metatable<T, std::index_sequence<I...>, Args...>> {
			typedef usertype_metatable<T, std::index_sequence<I...>, Args...> umt_t;
			typedef typename umt_t::regs_t regs_t;

			static umt_t& make_cleanup(lua_State* L, umt_t&& umx) {
				// ensure some sort of uniqueness
				static int uniqueness = 0;
				std::string uniquegcmetakey = usertype_traits<T>::user_gc_metatable();
				// std::to_string doesn't exist in android still, with NDK, so this bullshit
				// is necessary
				// thanks, Android :v
				int appended = snprintf(nullptr, 0, "%d", uniqueness);
				std::size_t insertionpoint = uniquegcmetakey.length() - 1;
				uniquegcmetakey.append(appended, '\0');
				char* uniquetarget = &uniquegcmetakey[insertionpoint];
				snprintf(uniquetarget, uniquegcmetakey.length(), "%d", uniqueness);
				++uniqueness;

				const char* gcmetakey = &usertype_traits<T>::gc_table()[0];
				// Make sure userdata's memory is properly in lua first,
				// otherwise all the light userdata we make later will become invalid
				stack::push<user<umt_t>>(L, metatable_key, uniquegcmetakey, std::move(umx));
				// Create the top level thing that will act as our deleter later on
				stack_reference umt(L, -1);
				stack::set_field<true>(L, gcmetakey, umt);
				umt.pop();

				stack::get_field<true>(L, gcmetakey);
				umt_t& target_umt = stack::pop<user<umt_t>>(L);
				return target_umt;
			}

			static int push(lua_State* L, umt_t&& umx) {

				umt_t& um = make_cleanup(L, std::move(umx));
				usertype_metatable_core& umc = um;
				regs_t value_table{{}};
				int lastreg = 0;
				(void)detail::swallow{0, (um.template make_regs<(I * 2)>(value_table, lastreg, std::get<(I * 2)>(um.functions), std::get<(I * 2 + 1)>(um.functions)), 0)...};
				um.finish_regs(value_table, lastreg);
				value_table[lastreg] = {nullptr, nullptr};
				regs_t ref_table = value_table;
				regs_t unique_table = value_table;
				bool hasdestructor = !value_table.empty() && to_string(meta_function::garbage_collect) == value_table[lastreg - 1].name;
				if (hasdestructor) {
					ref_table[lastreg - 1] = {nullptr, nullptr};
				}
				unique_table[lastreg - 1] = {value_table[lastreg - 1].name, detail::unique_destruct<T>};

				lua_createtable(L, 0, 2);
				stack_reference type_table(L, -1);

				stack::set_field(L, "name", detail::demangle<T>(), type_table.stack_index());
				stack::set_field(L, "is", &usertype_detail::is_check<T>, type_table.stack_index());

				// Now use um
				const bool& mustindex = umc.mustindex;
				for (std::size_t i = 0; i < 3; ++i) {
					// Pointer types, AKA "references" from C++
					const char* metakey = nullptr;
					luaL_Reg* metaregs = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						metaregs = ref_table.data();
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						metaregs = unique_table.data();
						break;
					case 2:
					default:
						metakey = &usertype_traits<T>::metatable()[0];
						metaregs = value_table.data();
						break;
					}
					luaL_newmetatable(L, metakey);
					stack_reference t(L, -1);
					stack::set_field(L, meta_function::type, type_table, t.stack_index());
					int upvalues = 0;
					upvalues += stack::push(L, nullptr);
					upvalues += stack::push(L, make_light(um));
					luaL_setfuncs(L, metaregs, upvalues);

					if (um.baseclasscheck != nullptr) {
						stack::set_field(L, detail::base_class_check_key(), um.baseclasscheck, t.stack_index());
					}
					if (um.baseclasscast != nullptr) {
						stack::set_field(L, detail::base_class_cast_key(), um.baseclasscast, t.stack_index());
					}

					stack::set_field(L, detail::base_class_index_propogation_key(), make_closure(um.indexbase, nullptr, make_light(um), make_light(umc)), t.stack_index());
					stack::set_field(L, detail::base_class_new_index_propogation_key(), make_closure(um.newindexbase, nullptr, make_light(um), make_light(umc)), t.stack_index());

					if (mustindex) {
						// Basic index pushing: specialize
						// index and newindex to give variables and stuff
						stack::set_field(L, meta_function::index, make_closure(umt_t::index_call, nullptr, make_light(um), make_light(umc)), t.stack_index());
						stack::set_field(L, meta_function::new_index, make_closure(umt_t::new_index_call, nullptr, make_light(um), make_light(umc)), t.stack_index());
					}
					else {
						// If there's only functions, we can use the fast index version
						stack::set_field(L, meta_function::index, t, t.stack_index());
					}
					// metatable on the metatable
					// for call constructor purposes and such
					lua_createtable(L, 0, 3);
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (um.callconstructfunc != nullptr) {
						stack::set_field(L, meta_function::call_function, make_closure(um.callconstructfunc, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}
					if (um.secondarymeta) {
						stack::set_field(L, meta_function::index, make_closure(umt_t::index_call, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
						stack::set_field(L, meta_function::new_index, make_closure(umt_t::new_index_call, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}
					// type information needs to be present on the behind-tables too

					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
					// We want to just leave the table
					// in the registry only, otherwise we return it
					t.pop();
				}

				// Now for the shim-table that actually gets assigned to the name
				luaL_newmetatable(L, &usertype_traits<T>::user_metatable()[0]);
				stack_reference t(L, -1);
				stack::set_field(L, meta_function::type, type_table, t.stack_index());
				int upvalues = 0;
				upvalues += stack::push(L, nullptr);
				upvalues += stack::push(L, make_light(um));
				luaL_setfuncs(L, value_table.data(), upvalues);
				{
					lua_createtable(L, 0, 3);
					stack_reference metabehind(L, -1);
					// type information needs to be present on the behind-tables too
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (um.callconstructfunc != nullptr) {
						stack::set_field(L, meta_function::call_function, make_closure(um.callconstructfunc, nullptr, make_light(um), make_light(umc)), metabehind.stack_index());
					}

					stack::set_field(L, meta_function::index, make_closure(umt_t::meta_index_call, nullptr, make_light(um), make_light(umc), nullptr, usertype_detail::toplevel_magic), metabehind.stack_index());
					stack::set_field(L, meta_function::new_index, make_closure(umt_t::meta_new_index_call, nullptr, make_light(um), make_light(umc), nullptr, usertype_detail::toplevel_magic), metabehind.stack_index());
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
				}

				lua_remove(L, type_table.stack_index());

				return 1;
			}
		};

	} // namespace stack

} // namespace sol

#endif // SOL_USERTYPE_METATABLE_HPP
