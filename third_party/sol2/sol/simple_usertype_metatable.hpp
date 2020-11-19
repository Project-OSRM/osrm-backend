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

#ifndef SOL_SIMPLE_USERTYPE_METATABLE_HPP
#define SOL_SIMPLE_USERTYPE_METATABLE_HPP

#include "usertype_metatable.hpp"
#include "object.hpp"
#include <vector>
#include <unordered_map>
#include <utility>

namespace sol {

	namespace usertype_detail {
		inline int call_indexing_object(lua_State* L, object& f) {
			int before = lua_gettop(L);
			f.push();
			for (int i = 1; i <= before; ++i) {
				lua_pushvalue(L, i);
			}
			lua_call(L, before, LUA_MULTRET);
			int after = lua_gettop(L);
			return after - before;
		}

		template <typename T, bool is_index, bool toplevel = false, bool has_indexing = false>
		inline int simple_core_indexing_call(lua_State* L) {
			simple_map& sm = toplevel
				? stack::get<user<simple_map>>(L, upvalue_index(simple_metatable_index))
				: stack::pop<user<simple_map>>(L);
			variable_map& variables = sm.variables;
			function_map& functions = sm.functions;
			static const int keyidx = -2 + static_cast<int>(is_index);
			if (toplevel) {
				if (type_of(L, keyidx) != type::string) {
					if (has_indexing) {
						object& indexingfunc = is_index
							? sm.index
							: sm.newindex;
						return call_indexing_object(L, indexingfunc);
					}
					else {
						return is_index
							? indexing_fail<T, is_index>(L)
							: metatable_new_index<T, true>(L);
					}
				}
			}
			string_view accessor = stack::get<string_view>(L, keyidx);
			variable_wrapper* varwrap = nullptr;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view& accessorkey = accessor;
				auto vit = variables.find(accessorkey, string_view_hash(), std::equal_to<string_view>());
#else
				std::string accessorkey(accessor.data(), accessor.size());
				auto vit = variables.find(accessorkey);
#endif // Compatible Hash
				if (vit != variables.cend()) {
					varwrap = vit->second.get();
				}
			}
			if (varwrap != nullptr) {
				return is_index ? varwrap->index(L) : varwrap->new_index(L);
			}
			bool function_failed = false;
			{
#if defined(SOL_UNORDERED_MAP_COMPATIBLE_HASH) && SOL_UNORDERED_MAP_COMPATIBLE_HASH
				string_view& accessorkey = accessor;
				auto fit = functions.find(accessorkey, string_view_hash(), std::equal_to<string_view>());
#else
				std::string accessorkey(accessor.data(), accessor.size());
				auto fit = functions.find(accessorkey);
#endif // Compatible Hash
				if (fit != functions.cend()) {
					object& func = fit->second;
					if (is_index) {
						return stack::push(L, func);
					}
					else {
						function_failed = true;
					}
				}
			}
			if (function_failed) {
				if (has_indexing && !is_toplevel(L)) {
					object& indexingfunc = is_index
						? sm.index
						: sm.newindex;
					return call_indexing_object(L, indexingfunc);
				}
				else {
					return is_index
						? indexing_fail<T, is_index>(L)
						: metatable_new_index<T, true>(L);
				}
			}
			/* Check table storage first for a method that works
			luaL_getmetatable(L, sm.metakey);
			if (type_of(L, -1) != type::lua_nil) {
				stack::get_field<false, true>(L, accessor.c_str(), lua_gettop(L));
				if (type_of(L, -1) != type::lua_nil) {
					// Woo, we found it?
					lua_remove(L, -2);
					return 1;
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			*/

			int ret = 0;
			bool found = false;
			// Otherwise, we need to do propagating calls through the bases
			if (is_index) {
				sm.indexbaseclasspropogation(L, found, ret, accessor);
			}
			else {
				sm.newindexbaseclasspropogation(L, found, ret, accessor);
			}
			if (found) {
				return ret;
			}
			if (toplevel) {
				if (has_indexing && !is_toplevel(L)) {
					object& indexingfunc = is_index
						? sm.index
						: sm.newindex;
					return call_indexing_object(L, indexingfunc);
				}
				else {
					return is_index
						? indexing_fail<T, is_index>(L)
						: metatable_new_index<T, true>(L);
				}
			}
			return -1;
		}

		template <typename T, bool has_indexing = false>
		inline int simple_real_index_call(lua_State* L) {
			return simple_core_indexing_call<T, true, true, has_indexing>(L);
		}

		template <typename T, bool has_indexing = false>
		inline int simple_real_new_index_call(lua_State* L) {
			return simple_core_indexing_call<T, false, true, has_indexing>(L);
		}

		template <typename T, bool has_indexing = false>
		inline int simple_index_call(lua_State* L) {
#if defined(__clang__)
			return detail::trampoline(L, &simple_real_index_call<T, has_indexing>);
#else
			return detail::typed_static_trampoline<decltype(&simple_real_index_call<T, has_indexing>), (&simple_real_index_call<T, has_indexing>)>(L);
#endif
		}

		template <typename T, bool has_indexing = false>
		inline int simple_new_index_call(lua_State* L) {
#if defined(__clang__)
			return detail::trampoline(L, &simple_real_new_index_call<T, has_indexing>);
#else
			return detail::typed_static_trampoline<decltype(&simple_real_new_index_call<T, has_indexing>), (&simple_real_new_index_call<T, has_indexing>)>(L);
#endif
		}
	} // namespace usertype_detail

	struct simple_tag {
	} const simple{};

	template <typename T>
	struct simple_usertype_metatable : usertype_detail::registrar {
	public:
		usertype_detail::function_map registrations;
		usertype_detail::variable_map varmap;
		object callconstructfunc;
		object indexfunc;
		object newindexfunc;
		lua_CFunction indexbase;
		lua_CFunction newindexbase;
		usertype_detail::base_walk indexbaseclasspropogation;
		usertype_detail::base_walk newindexbaseclasspropogation;
		void* baseclasscheck;
		void* baseclasscast;
		bool mustindex;
		bool secondarymeta;
		std::array<bool, 32> properties;

		template <typename N>
		void insert(N&& n, object&& o) {
			std::string key = usertype_detail::make_string(std::forward<N>(n));
			int is_indexer = static_cast<int>(usertype_detail::is_indexer(n));
			if (is_indexer == 1) {
				indexfunc = o;
				mustindex = true;
			}
			else if (is_indexer == 2) {
				newindexfunc = o;
				mustindex = true;
			}
			auto hint = registrations.find(key);
			if (hint == registrations.cend()) {
				registrations.emplace_hint(hint, std::move(key), std::move(o));
				return;
			}
			hint->second = std::move(o);
		}

		template <typename N, typename F, typename... Args>
		void insert_prepare(std::true_type, lua_State* L, N&&, F&& f, Args&&... args) {
			object o = make_object<F>(L, std::forward<F>(f), function_detail::call_indicator(), std::forward<Args>(args)...);
			callconstructfunc = std::move(o);
		}

		template <typename N, typename F, typename... Args>
		void insert_prepare(std::false_type, lua_State* L, N&& n, F&& f, Args&&... args) {
			object o = make_object<F>(L, std::forward<F>(f), std::forward<Args>(args)...);
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename F>
		void add_member_function(std::true_type, lua_State* L, N&& n, F&& f) {
			insert_prepare(std::is_same<meta::unqualified_t<N>, call_construction>(), L, std::forward<N>(n), std::forward<F>(f), function_detail::class_indicator<T>());
		}

		template <typename N, typename F>
		void add_member_function(std::false_type, lua_State* L, N&& n, F&& f) {
			insert_prepare(std::is_same<meta::unqualified_t<N>, call_construction>(), L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::enable<meta::is_callable<meta::unwrap_unqualified_t<F>>> = meta::enabler>
		void add_function(lua_State* L, N&& n, F&& f) {
			object o = make_object(L, as_function_reference(std::forward<F>(f)));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename F, meta::disable<meta::is_callable<meta::unwrap_unqualified_t<F>>> = meta::enabler>
		void add_function(lua_State* L, N&& n, F&& f) {
			add_member_function(std::is_member_pointer<meta::unwrap_unqualified_t<F>>(), L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::disable<is_variable_binding<meta::unqualified_t<F>>> = meta::enabler>
		void add(lua_State* L, N&& n, F&& f) {
			add_function(L, std::forward<N>(n), std::forward<F>(f));
		}

		template <typename N, typename F, meta::enable<is_variable_binding<meta::unqualified_t<F>>> = meta::enabler>
		void add(lua_State*, N&& n, F&& f) {
			mustindex = true;
			secondarymeta = true;
			std::string key = usertype_detail::make_string(std::forward<N>(n));
			auto o = std::make_unique<usertype_detail::callable_binding<T, std::decay_t<F>>>(std::forward<F>(f));
			auto hint = varmap.find(key);
			if (hint == varmap.cend()) {
				varmap.emplace_hint(hint, std::move(key), std::move(o));
				return;
			}
			hint->second = std::move(o);
		}

		template <typename N, typename... Fxs>
		void add(lua_State* L, N&& n, constructor_wrapper<Fxs...> c) {
			object o(L, in_place_type<detail::tagged<T, constructor_wrapper<Fxs...>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename... Lists>
		void add(lua_State* L, N&& n, constructor_list<Lists...> c) {
			object o(L, in_place_type<detail::tagged<T, constructor_list<Lists...>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N>
		void add(lua_State* L, N&& n, destructor_wrapper<void> c) {
			object o(L, in_place_type<detail::tagged<T, destructor_wrapper<void>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename N, typename Fx>
		void add(lua_State* L, N&& n, destructor_wrapper<Fx> c) {
			object o(L, in_place_type<detail::tagged<T, destructor_wrapper<Fx>>>, std::move(c));
			if (std::is_same<meta::unqualified_t<N>, call_construction>::value) {
				callconstructfunc = std::move(o);
				return;
			}
			insert(std::forward<N>(n), std::move(o));
		}

		template <typename... Bases>
		void add(lua_State*, base_classes_tag, bases<Bases...>) {
			static_assert(sizeof(usertype_detail::base_walk) <= sizeof(void*), "size of function pointer is greater than sizeof(void*); cannot work on this platform. Please file a bug report.");
			static_assert(!meta::any_same<T, Bases...>::value, "base classes cannot list the original class as part of the bases");
			if (sizeof...(Bases) < 1) {
				return;
			}
			mustindex = true;
			(void)detail::swallow{0, ((detail::has_derived<Bases>::value = true), 0)...};

			static_assert(sizeof(void*) <= sizeof(detail::inheritance_check_function), "The size of this data pointer is too small to fit the inheritance checking function: Please file a bug report.");
			static_assert(sizeof(void*) <= sizeof(detail::inheritance_cast_function), "The size of this data pointer is too small to fit the inheritance checking function: Please file a bug report.");
			baseclasscheck = reinterpret_cast<void*>(&detail::inheritance<T, Bases...>::type_check);
			baseclasscast = reinterpret_cast<void*>(&detail::inheritance<T, Bases...>::type_cast);
			indexbaseclasspropogation = usertype_detail::walk_all_bases<true, Bases...>;
			newindexbaseclasspropogation = usertype_detail::walk_all_bases<false, Bases...>;
		}

	private:
		template <std::size_t... I, typename Tuple>
		simple_usertype_metatable(detail::verified_tag, std::index_sequence<I...>, lua_State* L, Tuple&& args)
		: callconstructfunc(lua_nil), indexfunc(lua_nil), newindexfunc(lua_nil), indexbase(&usertype_detail::simple_core_indexing_call<T, true>), newindexbase(&usertype_detail::simple_core_indexing_call<T, false>), indexbaseclasspropogation(usertype_detail::walk_all_bases<true>), newindexbaseclasspropogation(&usertype_detail::walk_all_bases<false>), baseclasscheck(nullptr), baseclasscast(nullptr), mustindex(false), secondarymeta(false), properties() {
			properties.fill(false);

			(void)detail::swallow{0,
				(add(L, detail::forward_get<I * 2>(args), detail::forward_get<I * 2 + 1>(args)), 0)...};
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::verified_tag v, Args&&... args)
		: simple_usertype_metatable(v, std::make_index_sequence<sizeof...(Args) / 2>(), L, std::forward_as_tuple(std::forward<Args>(args)...)) {
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::add_destructor_tag, Args&&... args)
		: simple_usertype_metatable(L, detail::verified, std::forward<Args>(args)..., "__gc", default_destructor) {
		}

		template <typename... Args>
		simple_usertype_metatable(lua_State* L, detail::check_destructor_tag, Args&&... args)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_destructible<T>, meta::neg<detail::has_destructor<Args...>>>, detail::add_destructor_tag, detail::verified_tag>(), std::forward<Args>(args)...) {
		}

	public:
		simple_usertype_metatable(lua_State* L)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_default_constructible<T>>, decltype(default_constructor), detail::check_destructor_tag>()) {
		}

		template <typename Arg, typename... Args, meta::disable_any<meta::any_same<meta::unqualified_t<Arg>, detail::verified_tag, detail::add_destructor_tag, detail::check_destructor_tag>, meta::is_specialization_of<meta::unqualified_t<Arg>, constructors>, meta::is_specialization_of<meta::unqualified_t<Arg>, constructor_wrapper>> = meta::enabler>
		simple_usertype_metatable(lua_State* L, Arg&& arg, Args&&... args)
		: simple_usertype_metatable(L, meta::condition<meta::all<std::is_default_constructible<T>, meta::neg<detail::has_constructor<Args...>>>, decltype(default_constructor), detail::check_destructor_tag>(), std::forward<Arg>(arg), std::forward<Args>(args)...) {
		}

		template <typename... Args, typename... CArgs>
		simple_usertype_metatable(lua_State* L, constructors<CArgs...> constructorlist, Args&&... args)
		: simple_usertype_metatable(L, detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		template <typename... Args, typename... Fxs>
		simple_usertype_metatable(lua_State* L, constructor_wrapper<Fxs...> constructorlist, Args&&... args)
		: simple_usertype_metatable(L, detail::check_destructor_tag(), std::forward<Args>(args)..., "new", constructorlist) {
		}

		simple_usertype_metatable(const simple_usertype_metatable&) = default;
		simple_usertype_metatable(simple_usertype_metatable&&) = default;
		simple_usertype_metatable& operator=(const simple_usertype_metatable&) = default;
		simple_usertype_metatable& operator=(simple_usertype_metatable&&) = default;

		virtual int push_um(lua_State* L) override {
			return stack::push(L, std::move(*this));
		}
	};

	namespace stack {
		template <typename T>
		struct pusher<simple_usertype_metatable<T>> {
			typedef simple_usertype_metatable<T> umt_t;

			static usertype_detail::simple_map& make_cleanup(lua_State* L, umt_t& umx) {
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
				stack::push<user<usertype_detail::simple_map>>(L, metatable_key, uniquegcmetakey, &usertype_traits<T>::metatable()[0],
					umx.indexbaseclasspropogation, umx.newindexbaseclasspropogation,
					std::move(umx.indexfunc), std::move(umx.newindexfunc),
					std::move(umx.varmap), std::move(umx.registrations));
				stack_reference stackvarmap(L, -1);
				stack::set_field<true>(L, gcmetakey, stackvarmap);
				stackvarmap.pop();

				stack::get_field<true>(L, gcmetakey);
				usertype_detail::simple_map& varmap = stack::pop<user<usertype_detail::simple_map>>(L);
				return varmap;
			}

			static int push(lua_State* L, umt_t&& umx) {
				bool hasindex = umx.indexfunc.valid();
				bool hasnewindex = umx.newindexfunc.valid();
				auto& varmap = make_cleanup(L, umx);
				auto& properties = umx.properties;
				auto sic = hasindex ? &usertype_detail::simple_index_call<T, true> : &usertype_detail::simple_index_call<T, false>;
				auto snic = hasnewindex ? &usertype_detail::simple_new_index_call<T, true> : &usertype_detail::simple_new_index_call<T, false>;

				lua_createtable(L, 0, 2);
				stack_reference type_table(L, -1);

				stack::set_field(L, "name", detail::demangle<T>(), type_table.stack_index());
				stack::set_field(L, "is", &usertype_detail::is_check<T>, type_table.stack_index());

				auto safety_check = [&](const std::string& first) {
					for (std::size_t j = 0; j < properties.size(); ++j) {
						meta_function mf = static_cast<meta_function>(j);
						const std::string& mfname = to_string(mf);
						bool& prop = properties[j];
						if (mfname != first)
							continue;
						switch (mf) {
						case meta_function::construct:
							if (prop) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
								assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
								throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
							}
							break;
						case meta_function::garbage_collect:
							if (prop) {
#if defined(SOL_NO_EXCEPTIONS) && SOL_NO_EXCEPTIONS
								assert(false && "sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#else
								throw error("sol: 2 separate constructor (new) functions were set on this type. Please specify only 1 sol::meta_function::construct/'new' type AND wrap the function in a sol::factories/initializers call, as shown by the documentation and examples, otherwise you may create problems");
#endif
							}
							return;
						default:
							break;
						}
						prop = true;
						break;
					}
				};

				for (auto& kvp : varmap.functions) {
					auto& first = std::get<0>(kvp);
					safety_check(first);
				}

				auto register_kvp = [&](std::size_t meta_index, stack_reference& t, const std::string& first, object& second) {
					meta_function mf = meta_function::construct;
					for (std::size_t j = 0; j < properties.size(); ++j) {
						mf = static_cast<meta_function>(j);
						const std::string& mfname = to_string(mf);
						bool& prop = properties[j];
						if (mfname != first)
							continue;
						switch (mf) {
						case meta_function::index:
							umx.indexfunc = second;
							break;
						case meta_function::new_index:
							umx.newindexfunc = second;
							break;
						default:
							break;
						}
						prop = true;
						break;
					}
					switch (meta_index) {
					case 0:
						if (mf == meta_function::garbage_collect) {
							return;
						}
						break;
					case 1:
						if (mf == meta_function::garbage_collect) {
							stack::set_field(L, first, detail::unique_destruct<T>, t.stack_index());
							return;
						}
						break;
					case 2:
					default:
						break;
					}
					stack::set_field(L, first, second, t.stack_index());
				};
				for (std::size_t i = 0; i < 3; ++i) {
					const char* metakey = nullptr;
					switch (i) {
					case 0:
						metakey = &usertype_traits<T*>::metatable()[0];
						break;
					case 1:
						metakey = &usertype_traits<detail::unique_usertype<T>>::metatable()[0];
						break;
					case 2:
					default:
						metakey = &usertype_traits<T>::metatable()[0];
						break;
					}
					luaL_newmetatable(L, metakey);
					stack_reference t(L, -1);
					stack::set_field(L, meta_function::type, type_table, t.stack_index());

					for (auto& kvp : varmap.functions) {
						auto& first = std::get<0>(kvp);
						auto& second = std::get<1>(kvp);
						register_kvp(i, t, first, second);
					}
					luaL_Reg opregs[34]{};
					int opregsindex = 0;
					auto prop_fx = [&](meta_function mf) { return !properties[static_cast<int>(mf)]; };
					usertype_detail::insert_default_registrations<T>(opregs, opregsindex, prop_fx);
					t.push();
					luaL_setfuncs(L, opregs, 0);
					t.pop();

					if (umx.baseclasscheck != nullptr) {
						stack::set_field(L, detail::base_class_check_key(), umx.baseclasscheck, t.stack_index());
					}
					if (umx.baseclasscast != nullptr) {
						stack::set_field(L, detail::base_class_cast_key(), umx.baseclasscast, t.stack_index());
					}

					// Base class propagation features
					stack::set_field(L, detail::base_class_index_propogation_key(), umx.indexbase, t.stack_index());
					stack::set_field(L, detail::base_class_new_index_propogation_key(), umx.newindexbase, t.stack_index());

					if (umx.mustindex) {
						// use indexing function
						stack::set_field(L, meta_function::index,
							make_closure(sic,
								nullptr,
								make_light(varmap)),
							t.stack_index());
						stack::set_field(L, meta_function::new_index,
							make_closure(snic,
								nullptr,
								make_light(varmap)),
							t.stack_index());
					}
					else {
						// Metatable indexes itself
						stack::set_field(L, meta_function::index, t, t.stack_index());
					}
					// metatable on the metatable
					// for call constructor purposes and such
					lua_createtable(L, 0, 2 * static_cast<int>(umx.secondarymeta) + static_cast<int>(umx.callconstructfunc.valid()));
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (umx.callconstructfunc.valid()) {
						stack::set_field(L, meta_function::call_function, umx.callconstructfunc, metabehind.stack_index());
					}
					if (umx.secondarymeta) {
						stack::set_field(L, meta_function::index,
							make_closure(sic,
								nullptr,
								make_light(varmap)),
							metabehind.stack_index());
						stack::set_field(L, meta_function::new_index,
							make_closure(snic,
								nullptr,
								make_light(varmap)),
							metabehind.stack_index());
					}
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();

					t.pop();
				}

				// Now for the shim-table that actually gets pushed
				luaL_newmetatable(L, &usertype_traits<T>::user_metatable()[0]);
				stack_reference t(L, -1);
				stack::set_field(L, meta_function::type, type_table, t.stack_index());

				for (auto& kvp : varmap.functions) {
					auto& first = std::get<0>(kvp);
					auto& second = std::get<1>(kvp);
					register_kvp(2, t, first, second);
				}
				{
					lua_createtable(L, 0, 2 + static_cast<int>(umx.callconstructfunc.valid()));
					stack_reference metabehind(L, -1);
					stack::set_field(L, meta_function::type, type_table, metabehind.stack_index());
					if (umx.callconstructfunc.valid()) {
						stack::set_field(L, meta_function::call_function, umx.callconstructfunc, metabehind.stack_index());
					}
					// use indexing function
					stack::set_field(L, meta_function::index,
						make_closure(sic,
							nullptr,
							make_light(varmap),
							nullptr,
							nullptr,
							usertype_detail::toplevel_magic),
						metabehind.stack_index());
					stack::set_field(L, meta_function::new_index,
						make_closure(snic,
							nullptr,
							make_light(varmap),
							nullptr,
							nullptr,
							usertype_detail::toplevel_magic),
						metabehind.stack_index());
					stack::set_field(L, metatable_key, metabehind, t.stack_index());
					metabehind.pop();
				}

				lua_remove(L, type_table.stack_index());

				// Don't pop the table when we're done;
				// return it
				return 1;
			}
		};
	} // namespace stack
} // namespace sol

#endif // SOL_SIMPLE_USERTYPE_METATABLE_HPP
