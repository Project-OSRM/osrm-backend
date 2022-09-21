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

#ifndef SOL_STACK_CORE_HPP
#define SOL_STACK_CORE_HPP

#include "types.hpp"
#include "inheritance.hpp"
#include "error_handler.hpp"
#include "reference.hpp"
#include "stack_reference.hpp"
#include "tuple.hpp"
#include "traits.hpp"
#include "tie.hpp"
#include "stack_guard.hpp"
#include "demangle.hpp"
#include "forward_detail.hpp"

#include <vector>
#include <forward_list>
#include <string>
#include <algorithm>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <optional>
#endif // C++17

namespace sol {
	namespace detail {
		struct as_reference_tag {};
		template <typename T>
		struct as_pointer_tag {};
		template <typename T>
		struct as_value_tag {};
		template <typename T>
		struct as_table_tag {};

		using unique_destructor = void (*)(void*);
#if 0
		using unique_tag = detail::inheritance_unique_cast_function;
#else
		using unique_tag = const char*;
#endif

		inline void* align(std::size_t alignment, std::size_t size, void*& ptr, std::size_t& space, std::size_t& required_space) {
			// this handels arbitrary alignments...
			// make this into a power-of-2-only?
			// actually can't: this is a C++14-compatible framework,
			// power of 2 alignment is C++17
			std::uintptr_t initial = reinterpret_cast<std::uintptr_t>(ptr);
			std::uintptr_t offby = static_cast<std::uintptr_t>(initial % alignment);
			std::uintptr_t padding = (alignment - offby) % alignment;
			required_space += size + padding;
			if (space < required_space) {
				return nullptr;
			}
			ptr = static_cast<void*>(static_cast<char*>(ptr) + padding);
			space -= padding;
			return ptr;
		}

		inline void* align(std::size_t alignment, std::size_t size, void*& ptr, std::size_t& space) {
			std::size_t required_space = 0;
			return align(alignment, size, ptr, space, required_space);
		}

		template <typename... Args>
		inline std::size_t aligned_space_for(void* alignment = nullptr) {
			char* start = static_cast<char*>(alignment);
			auto specific_align = [&alignment](std::size_t a, std::size_t s) {
				std::size_t space = (std::numeric_limits<std::size_t>::max)();
				alignment = align(a, s, alignment, space);
				alignment = static_cast<void*>(static_cast<char*>(alignment) + s);
			};
			(void)detail::swallow{ int{}, (specific_align(std::alignment_of<Args>::value, sizeof(Args)), int{})... };
			return static_cast<char*>(alignment) - start;
		}

		inline void* align_usertype_pointer(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<void*>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<void*>::value, sizeof(void*), ptr, space);
		}

		template <bool pre_aligned = false>
		inline void* align_usertype_unique_destructor(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<unique_destructor>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_pointer(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(void*));
			}
			if (!use_align::value) {
				return static_cast<void*>(static_cast<void**>(ptr) + 1);
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<unique_destructor>::value, sizeof(unique_destructor), ptr, space);
		}

		template <bool pre_aligned = false>
		inline void* align_usertype_unique_tag(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<unique_tag>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_unique_destructor(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(unique_destructor));
			}
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<unique_tag>::value, sizeof(unique_tag), ptr, space);
		}
		template <typename T, bool pre_aligned = false>
		inline void* align_usertype_unique(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!pre_aligned) {
				ptr = align_usertype_unique_tag(ptr);
				ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(unique_tag));
			}
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<T>::value, sizeof(T), ptr, space);
		}

		template <typename T>
		inline void* align_user(void* ptr) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				return ptr;
			}
			std::size_t space = (std::numeric_limits<std::size_t>::max)();
			return align(std::alignment_of<T>::value, sizeof(T), ptr, space);
		}

		template <typename T>
		inline T** usertype_allocate_pointer(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T** pointerpointer = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));
				return pointerpointer;
			}
			static const std::size_t initial_size = aligned_space_for<T*>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*>(reinterpret_cast<void*>(0x1));

			std::size_t allocated_size = initial_size;
			void* unadjusted = lua_newuserdata(L, initial_size);
			void* adjusted = align(std::alignment_of<T*>::value, sizeof(T*), unadjusted, allocated_size);
			if (adjusted == nullptr) {
				lua_pop(L, 1);
				// what kind of absolute garbage trash allocator are we dealing with?
				// whatever, add some padding in the case of MAXIMAL alignment waste...
				allocated_size = misaligned_size;
				unadjusted = lua_newuserdata(L, allocated_size);
				adjusted = align(std::alignment_of<T*>::value, sizeof(T*), unadjusted, allocated_size);
				if (adjusted == nullptr) {
					// trash allocator can burn in hell
					lua_pop(L, 1);
					//luaL_error(L, "if you are the one that wrote this allocator you should feel bad for doing a worse job than malloc/realloc and should go read some books, yeah?");
					luaL_error(L, "cannot properly align memory for '%s'", detail::demangle<T*>().data());
				}
			}
			return static_cast<T**>(adjusted);
		}

		template <typename T>
		inline T* usertype_allocate(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1 || std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T** pointerpointer = static_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(T)));
				T*& pointerreference = *pointerpointer;
				T* allocationtarget = reinterpret_cast<T*>(pointerpointer + 1);
				pointerreference = allocationtarget;
				return allocationtarget;
			}

			/* the assumption is that `lua_newuserdata` -- unless someone
			passes a specific lua_Alloc that gives us bogus, un-aligned pointers
			-- uses malloc, which tends to hand out more or less aligned pointers to memory
			(most of the time, anyhow)

			but it's not guaranteed, so we have to do a post-adjustment check and increase padding

			we do this preliminarily with compile-time stuff, to see
			if we strike lucky with the allocator and alignment values

			otherwise, we have to re-allocate the userdata and
			over-allocate some space for additional padding because
			compilers are optimized for aligned reads/writes
			(and clang will barf UBsan errors on us for not being aligned)
			*/
			static const std::size_t initial_size = aligned_space_for<T*, T>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*, T>(reinterpret_cast<void*>(0x1));

			void* pointer_adjusted;
			void* data_adjusted;
			auto attempt_alloc = [](lua_State* L, std::size_t allocated_size, void*& pointer_adjusted, void*& data_adjusted) -> bool {
				void* adjusted = lua_newuserdata(L, allocated_size);
				pointer_adjusted = align(std::alignment_of<T*>::value, sizeof(T*), adjusted, allocated_size);
				if (pointer_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				// subtract size of what we're going to allocate there
				allocated_size -= sizeof(T*);
				adjusted = static_cast<void*>(static_cast<char*>(pointer_adjusted) + sizeof(T*));
				data_adjusted = align(std::alignment_of<T>::value, sizeof(T), adjusted, allocated_size);
				if (data_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				return true;
			};
			bool result = attempt_alloc(L, initial_size, pointer_adjusted, data_adjusted);
			if (!result) {
				// we're likely to get something that fails to perform the proper allocation a second time,
				// so we use the suggested_new_size bump to help us out here
				pointer_adjusted = nullptr;
				data_adjusted = nullptr;
				result = attempt_alloc(L, misaligned_size, pointer_adjusted, data_adjusted);
				if (!result) {
					if (pointer_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (pointer section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else {
						luaL_error(L, "aligned allocation of userdata block (data section) for '%s' failed", detail::demangle<T>().c_str());
					}
					return nullptr;
				}
			}

			T** pointerpointer = reinterpret_cast<T**>(pointer_adjusted);
			T*& pointerreference = *pointerpointer;
			T* allocationtarget = reinterpret_cast<T*>(data_adjusted);
			pointerreference = allocationtarget;
			return allocationtarget;
		}

		template <typename T, typename Real>
		inline Real* usertype_unique_allocate(lua_State* L, T**& pref, unique_destructor*& dx, unique_tag*& id) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T*>::value > 1 || std::alignment_of<unique_tag>::value > 1 || std::alignment_of<unique_destructor>::value > 1 || std::alignment_of<Real>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				pref = static_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(detail::unique_destructor) + sizeof(unique_tag) + sizeof(Real)));
				dx = static_cast<detail::unique_destructor*>(static_cast<void*>(pref + 1));
				id = static_cast<unique_tag*>(static_cast<void*>(dx + 1));
				Real* mem = static_cast<Real*>(static_cast<void*>(id + 1));
				return mem;
			}

			static const std::size_t initial_size = aligned_space_for<T*, unique_destructor, unique_tag, Real>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T*, unique_destructor, unique_tag, Real>(reinterpret_cast<void*>(0x1));

			void* pointer_adjusted;
			void* dx_adjusted;
			void* id_adjusted;
			void* data_adjusted;
			auto attempt_alloc = [](lua_State* L, std::size_t allocated_size, void*& pointer_adjusted, void*& dx_adjusted, void*& id_adjusted, void*& data_adjusted) -> bool {
				void* adjusted = lua_newuserdata(L, allocated_size);
				pointer_adjusted = align(std::alignment_of<T*>::value, sizeof(T*), adjusted, allocated_size);
				if (pointer_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(T*);

				adjusted = static_cast<void*>(static_cast<char*>(pointer_adjusted) + sizeof(T*));
				dx_adjusted = align(std::alignment_of<unique_destructor>::value, sizeof(unique_destructor), adjusted, allocated_size);
				if (dx_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(unique_destructor);

				adjusted = static_cast<void*>(static_cast<char*>(dx_adjusted) + sizeof(unique_destructor));

				id_adjusted = align(std::alignment_of<unique_tag>::value, sizeof(unique_tag), adjusted, allocated_size);
				if (id_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				allocated_size -= sizeof(unique_tag);

				adjusted = static_cast<void*>(static_cast<char*>(id_adjusted) + sizeof(unique_tag));
				data_adjusted = align(std::alignment_of<Real>::value, sizeof(Real), adjusted, allocated_size);
				if (data_adjusted == nullptr) {
					lua_pop(L, 1);
					return false;
				}
				return true;
			};
			bool result = attempt_alloc(L, initial_size, pointer_adjusted, dx_adjusted, id_adjusted, data_adjusted);
			if (!result) {
				// we're likely to get something that fails to perform the proper allocation a second time,
				// so we use the suggested_new_size bump to help us out here
				pointer_adjusted = nullptr;
				dx_adjusted = nullptr;
				id_adjusted = nullptr;
				data_adjusted = nullptr;
				result = attempt_alloc(L, misaligned_size, pointer_adjusted, dx_adjusted, id_adjusted, data_adjusted);
				if (!result) {
					if (pointer_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (pointer section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else if (dx_adjusted == nullptr) {
						luaL_error(L, "aligned allocation of userdata block (deleter section) for '%s' failed", detail::demangle<T>().c_str());
					}
					else {
						luaL_error(L, "aligned allocation of userdata block (data section) for '%s' failed", detail::demangle<T>().c_str());
					}
					return nullptr;
				}
			}

			pref = static_cast<T**>(pointer_adjusted);
			dx = static_cast<detail::unique_destructor*>(dx_adjusted);
			id = static_cast<unique_tag*>(id_adjusted);
			Real* mem = static_cast<Real*>(data_adjusted);
			return mem;
		}

		template <typename T>
		inline T* user_allocate(lua_State* L) {
			typedef std::integral_constant<bool,
#if defined(SOL_NO_MEMORY_ALIGNMENT) && SOL_NO_MEMORY_ALIGNMENT
				false
#else
				(std::alignment_of<T>::value > 1)
#endif
				>
				use_align;
			if (!use_align::value) {
				T* pointer = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
				return pointer;
			}

			static const std::size_t initial_size = aligned_space_for<T>(nullptr);
			static const std::size_t misaligned_size = aligned_space_for<T>(reinterpret_cast<void*>(0x1));

			std::size_t allocated_size = initial_size;
			void* unadjusted = lua_newuserdata(L, allocated_size);
			void* adjusted = align(std::alignment_of<T>::value, sizeof(T), unadjusted, allocated_size);
			if (adjusted == nullptr) {
				lua_pop(L, 1);
				// try again, add extra space for alignment padding
				allocated_size = misaligned_size;
				unadjusted = lua_newuserdata(L, allocated_size);
				adjusted = align(std::alignment_of<T>::value, sizeof(T), unadjusted, allocated_size);
				if (adjusted == nullptr) {
					lua_pop(L, 1);
					luaL_error(L, "cannot properly align memory for '%s'", detail::demangle<T>().data());
				}
			}
			return static_cast<T*>(adjusted);
		}

		template <typename T>
		inline int usertype_alloc_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_usertype_pointer(memory);
			T** pdata = static_cast<T**>(memory);
			T* data = *pdata;
			std::allocator<T> alloc{};
			std::allocator_traits<std::allocator<T>>::destroy(alloc, data);
			return 0;
		}

		template <typename T>
		inline int unique_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_usertype_unique_destructor(memory);
			unique_destructor& dx = *static_cast<unique_destructor*>(memory);
			memory = static_cast<void*>(static_cast<char*>(memory) + sizeof(unique_destructor));
			memory = align_usertype_unique_tag<true>(memory);
			memory = static_cast<void*>(static_cast<char*>(memory) + sizeof(unique_tag));
			(dx)(memory);
			return 0;
		}

		template <typename T>
		inline int user_alloc_destruct(lua_State* L) {
			void* memory = lua_touserdata(L, 1);
			memory = align_user<T>(memory);
			T* data = static_cast<T*>(memory);
			std::allocator<T> alloc;
			std::allocator_traits<std::allocator<T>>::destroy(alloc, data);
			return 0;
		}

		template <typename T, typename Real>
		inline void usertype_unique_alloc_destroy(void* memory) {
			memory = align_usertype_unique<Real, true>(memory);
			Real* target = static_cast<Real*>(memory);
			std::allocator<Real> alloc;
			std::allocator_traits<std::allocator<Real>>::destroy(alloc, target);
		}

		template <typename T>
		inline int cannot_destruct(lua_State* L) {
			return luaL_error(L, "cannot call the destructor for '%s': it is either hidden (protected/private) or removed with '= delete' and thusly this type is being destroyed without properly destructing, invoking undefined behavior: please bind a usertype and specify a custom destructor to define the behavior properly", detail::demangle<T>().data());
		}

		template <typename T>
		void reserve(T&, std::size_t) {
		}

		template <typename T, typename Al>
		void reserve(std::vector<T, Al>& arr, std::size_t hint) {
			arr.reserve(hint);
		}

		template <typename T, typename Tr, typename Al>
		void reserve(std::basic_string<T, Tr, Al>& arr, std::size_t hint) {
			arr.reserve(hint);
		}
	} // namespace detail

	namespace stack {

		template <typename T>
		struct extensible {};

		template <typename T, bool global = false, bool raw = false, typename = void>
		struct field_getter;
		template <typename T, typename P, bool global = false, bool raw = false, typename = void>
		struct probe_field_getter;
		template <typename T, bool global = false, bool raw = false, typename = void>
		struct field_setter;
		template <typename T, typename = void>
		struct getter;
		template <typename T, typename = void>
		struct qualified_getter;
		template <typename T, typename = void>
		struct userdata_getter;
		template <typename T, typename = void>
		struct popper;
		template <typename T, typename = void>
		struct pusher;
		template <typename T, type = lua_type_of<T>::value, typename = void>
		struct checker;
		template <typename T, type = lua_type_of<T>::value, typename = void>
		struct qualified_checker;
		template <typename T, typename = void>
		struct userdata_checker;
		template <typename T, typename = void>
		struct check_getter;
		template <typename T, typename = void>
		struct qualified_check_getter;

		struct probe {
			bool success;
			int levels;

			probe(bool s, int l)
			: success(s), levels(l) {
			}

			operator bool() const {
				return success;
			};
		};

		struct record {
			int last;
			int used;

			record()
			: last(), used() {
			}
			void use(int count) {
				last = count;
				used += count;
			}
		};

		namespace stack_detail {
			template <typename T>
			struct strip {
				typedef T type;
			};
			template <typename T>
			struct strip<std::reference_wrapper<T>> {
				typedef T& type;
			};
			template <typename T>
			struct strip<user<T>> {
				typedef T& type;
			};
			template <typename T>
			struct strip<non_null<T>> {
				typedef T type;
			};
			template <typename T>
			using strip_t = typename strip<T>::type;

			template <typename T>
			struct strip_extensible { typedef T type; };

			template <typename T>
			struct strip_extensible<extensible<T>> { typedef T type; };

			template <typename T>
			using strip_extensible_t = typename strip_extensible<T>::type;

			template <typename C>
			static int get_size_hint(const C& c) {
				return static_cast<int>(c.size());
			}

			template <typename V, typename Al>
			static int get_size_hint(const std::forward_list<V, Al>&) {
				// forward_list makes me sad
				return static_cast<int>(32);
			}

			template <typename T>
			inline decltype(auto) unchecked_unqualified_get(lua_State* L, int index, record& tracking) {
				typedef meta::unqualified_t<T> Tu;
				getter<Tu> g{};
				(void)g;
				return g.get(L, index, tracking);
			}

			template <typename T>
			inline decltype(auto) unchecked_get(lua_State* L, int index, record& tracking) {
				qualified_getter<T> g{};
				(void)g;
				return g.get(L, index, tracking);
			}

			template <typename T, typename Arg, typename... Args>
			inline int push_reference(lua_State* L, Arg&& arg, Args&&... args) {
				typedef meta::all<
					std::is_lvalue_reference<T>,
					meta::neg<std::is_const<T>>,
					meta::neg<is_lua_primitive<meta::unqualified_t<T>>>,
					meta::neg<is_unique_usertype<meta::unqualified_t<T>>>>
					use_reference_tag;
				pusher<std::conditional_t<use_reference_tag::value, detail::as_reference_tag, meta::unqualified_t<T>>> p{};
				(void)p;
				return p.push(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
			}

			template <typename T, typename Handler>
			bool check_usertype(std::false_type, lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
				typedef meta::unqualified_t<T> Tu;
				typedef detail::as_value_tag<Tu> detail_t;
				return checker<detail_t, type::userdata>{}.check(types<meta::unqualified_t<T>>(), L, index, indextype, std::forward<Handler>(handler), tracking);
			}

			template <typename T, typename Handler>
			bool check_usertype(std::true_type, lua_State* L, int index, type indextype, Handler&& handler, record& tracking) {
				typedef meta::unqualified_t<std::remove_pointer_t<meta::unqualified_t<T>>> Tu;
				typedef detail::as_pointer_tag<Tu> detail_t;
				return checker<detail_t, type::userdata>{}.check(L, index, indextype, std::forward<Handler>(handler), tracking);
			}
		} // namespace stack_detail

		inline bool maybe_indexable(lua_State* L, int index = -1) {
			type t = type_of(L, index);
			return t == type::userdata || t == type::table;
		}

		inline int top(lua_State* L) {
			return lua_gettop(L);
		}

		inline bool is_main_thread(lua_State* L) {
			int ismainthread = lua_pushthread(L);
			lua_pop(L, 1);
			return ismainthread == 1;
		}

		inline void coroutine_create_guard(lua_State* L) {
			if (is_main_thread(L)) {
				return;
			}
			int stacksize = lua_gettop(L);
			if (stacksize < 1) {
				return;
			}
			if (type_of(L, 1) != type::function) {
				return;
			}
			// well now we're screwed...
			// we can clean the stack and pray it doesn't destroy anything?
			lua_pop(L, stacksize);
		}

		template <typename T, typename... Args>
		inline int push(lua_State* L, T&& t, Args&&... args) {
			return pusher<meta::unqualified_t<T>>{}.push(L, std::forward<T>(t), std::forward<Args>(args)...);
		}

		// overload allows to use a pusher of a specific type, but pass in any kind of args
		template <typename T, typename Arg, typename... Args, typename = std::enable_if_t<!std::is_same<T, Arg>::value>>
		inline int push(lua_State* L, Arg&& arg, Args&&... args) {
			return pusher<meta::unqualified_t<T>>{}.push(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
		}

		template <typename T, typename... Args>
		inline int push_reference(lua_State* L, T&& t, Args&&... args) {
			return stack_detail::push_reference<T>(L, std::forward<T>(t), std::forward<Args>(args)...);
		}

		template <typename T, typename Arg, typename... Args>
		inline int push_reference(lua_State* L, Arg&& arg, Args&&... args) {
			return stack_detail::push_reference<T>(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
		}

		inline int multi_push(lua_State*) {
			// do nothing
			return 0;
		}

		template <typename T, typename... Args>
		inline int multi_push(lua_State* L, T&& t, Args&&... args) {
			int pushcount = push(L, std::forward<T>(t));
			void(detail::swallow{ (pushcount += stack::push(L, std::forward<Args>(args)), 0)... });
			return pushcount;
		}

		inline int multi_push_reference(lua_State*) {
			// do nothing
			return 0;
		}

		template <typename T, typename... Args>
		inline int multi_push_reference(lua_State* L, T&& t, Args&&... args) {
			int pushcount = push_reference(L, std::forward<T>(t));
			void(detail::swallow{ (pushcount += stack::push_reference(L, std::forward<Args>(args)), 0)... });
			return pushcount;
		}

		template <typename T, typename Handler>
		bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
			qualified_checker<T> c;
			// VC++ has a bad warning here: shut it up
			(void)c;
			return c.check(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool check(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		bool unqualified_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef meta::unqualified_t<T> Tu;
			checker<Tu> c;
			// VC++ has a bad warning here: shut it up
			(void)c;
			return c.check(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool unqualified_check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return unqualified_check<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool unqualified_check(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return unqualified_check<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		bool check_usertype(lua_State* L, int index, Handler&& handler, record& tracking) {
			type indextype = type_of(L, index);
			return stack_detail::check_usertype<T>(std::is_pointer<T>(), L, index, indextype, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		bool check_usertype(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check_usertype<T>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T>
		bool check_usertype(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check_usertype<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index, Handler&& handler, record& tracking) {
			typedef meta::unqualified_t<T> Tu;
			check_getter<Tu> cg{};
			(void)cg;
			return cg.get(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return unqualified_check_get<T>(L, index, handler, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_check_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return unqualified_check_get<T>(L, index, handler);
		}

		template <typename T, typename Handler>
		inline decltype(auto) check_get(lua_State* L, int index, Handler&& handler, record& tracking) {
			qualified_check_getter<T> cg{};
			(void)cg;
			return cg.get(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename T, typename Handler>
		inline decltype(auto) check_get(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return check_get<T>(L, index, handler, tracking);
		}

		template <typename T>
		inline decltype(auto) check_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			auto handler = no_panic;
			return check_get<T>(L, index, handler);
		}

		namespace stack_detail {

#if defined(SOL_SAFE_GETTER) && SOL_SAFE_GETTER
			template <typename T>
			inline auto tagged_unqualified_get(types<T>, lua_State* L, int index, record& tracking) -> decltype(stack_detail::unchecked_unqualified_get<T>(L, index, tracking)) {
				if (is_lua_reference<T>::value) {
					return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
				}
				auto op = unqualified_check_get<T>(L, index, type_panic_c_str, tracking);
				return *std::move(op);
			}

			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<optional<T>>(L, index, tracking);
			}

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<std::optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<std::optional<T>>(L, index, tracking);
			}
#endif // shitty optional

			template <typename T>
			inline auto tagged_get(types<T>, lua_State* L, int index, record& tracking) -> decltype(stack_detail::unchecked_get<T>(L, index, tracking)) {
				if (is_lua_reference<T>::value) {
					return stack_detail::unchecked_get<T>(L, index, tracking);
				}
				auto op = check_get<T>(L, index, type_panic_c_str, tracking);
				return *std::move(op);
			}

			template <typename T>
			inline decltype(auto) tagged_get(types<optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<optional<T>>(L, index, tracking);
			}

#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			template <typename T>
			inline decltype(auto) tagged_get(types<std::optional<T>>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<std::optional<T>>(L, index, tracking);
			}
#endif // shitty optional

#else
			template <typename T>
			inline decltype(auto) tagged_unqualified_get(types<T>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_unqualified_get<T>(L, index, tracking);
			}

			template <typename T>
			inline decltype(auto) tagged_get(types<T>, lua_State* L, int index, record& tracking) {
				return stack_detail::unchecked_get<T>(L, index, tracking);
			}
#endif

			template <bool b>
			struct check_types {
				template <typename T, typename... Args, typename Handler>
				static bool check(types<T, Args...>, lua_State* L, int firstargument, Handler&& handler, record& tracking) {
					if (!stack::check<T>(L, firstargument + tracking.used, handler, tracking))
						return false;
					return check(types<Args...>(), L, firstargument, std::forward<Handler>(handler), tracking);
				}

				template <typename Handler>
				static bool check(types<>, lua_State*, int, Handler&&, record&) {
					return true;
				}
			};

			template <>
			struct check_types<false> {
				template <typename... Args, typename Handler>
				static bool check(types<Args...>, lua_State*, int, Handler&&, record&) {
					return true;
				}
			};

		} // namespace stack_detail

		template <bool b, typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return stack_detail::check_types<b>{}.check(types<meta::unqualified_t<Args>...>(), L, index, std::forward<Handler>(handler), tracking);
		}

		template <bool b, typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler) {
			record tracking{};
			return multi_check<b, Args...>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <bool b, typename... Args>
		bool multi_check(lua_State* L, int index) {
			auto handler = no_panic;
			return multi_check<b, Args...>(L, index, handler);
		}

		template <typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler, record& tracking) {
			return multi_check<true, Args...>(L, index, std::forward<Handler>(handler), tracking);
		}

		template <typename... Args, typename Handler>
		bool multi_check(lua_State* L, int index, Handler&& handler) {
			return multi_check<true, Args...>(L, index, std::forward<Handler>(handler));
		}

		template <typename... Args>
		bool multi_check(lua_State* L, int index) {
			return multi_check<true, Args...>(L, index);
		}

		template <typename T>
		inline decltype(auto) get_usertype(lua_State* L, int index, record& tracking) {
#if defined(SOL_SAFE_GETTER) && SOL_SAFE_GETTER
			return stack_detail::tagged_get(types<std::conditional_t<std::is_pointer<T>::value, detail::as_pointer_tag<std::remove_pointer_t<T>>, detail::as_value_tag<T>>>(), L, index, tracking);
#else
			return stack_detail::unchecked_get<std::conditional_t<std::is_pointer<T>::value, detail::as_pointer_tag<std::remove_pointer_t<T>>, detail::as_value_tag<T>>>(L, index, tracking);
#endif
		}

		template <typename T>
		inline decltype(auto) get_usertype(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return get_usertype<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_get(lua_State* L, int index, record& tracking) {
			return stack_detail::tagged_unqualified_get(types<T>(), L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) unqualified_get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return unqualified_get<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) get(lua_State* L, int index, record& tracking) {
			return stack_detail::tagged_get(types<T>(), L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) get(lua_State* L, int index = -lua_size<meta::unqualified_t<T>>::value) {
			record tracking{};
			return get<T>(L, index, tracking);
		}

		template <typename T>
		inline decltype(auto) pop(lua_State* L) {
			return popper<meta::unqualified_t<T>>{}.pop(L);
		}

		template <bool global = false, bool raw = false, typename Key>
		void get_field(lua_State* L, Key&& key) {
			field_getter<meta::unqualified_t<Key>, global, raw>{}.get(L, std::forward<Key>(key));
		}

		template <bool global = false, bool raw = false, typename Key>
		void get_field(lua_State* L, Key&& key, int tableindex) {
			field_getter<meta::unqualified_t<Key>, global, raw>{}.get(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, typename Key>
		void raw_get_field(lua_State* L, Key&& key) {
			get_field<global, true>(L, std::forward<Key>(key));
		}

		template <bool global = false, typename Key>
		void raw_get_field(lua_State* L, Key&& key, int tableindex) {
			get_field<global, true>(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, bool raw = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_get_field(lua_State* L, Key&& key) {
			return probe_field_getter<meta::unqualified_t<Key>, C, global, raw>{}.get(L, std::forward<Key>(key));
		}

		template <bool global = false, bool raw = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_get_field(lua_State* L, Key&& key, int tableindex) {
			return probe_field_getter<meta::unqualified_t<Key>, C, global, raw>{}.get(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_raw_get_field(lua_State* L, Key&& key) {
			return probe_get_field<global, true, C>(L, std::forward<Key>(key));
		}

		template <bool global = false, typename C = detail::non_lua_nil_t, typename Key>
		probe probe_raw_get_field(lua_State* L, Key&& key, int tableindex) {
			return probe_get_field<global, true, C>(L, std::forward<Key>(key), tableindex);
		}

		template <bool global = false, bool raw = false, typename Key, typename Value>
		void set_field(lua_State* L, Key&& key, Value&& value) {
			field_setter<meta::unqualified_t<Key>, global, raw>{}.set(L, std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global = false, bool raw = false, typename Key, typename Value>
		void set_field(lua_State* L, Key&& key, Value&& value, int tableindex) {
			field_setter<meta::unqualified_t<Key>, global, raw>{}.set(L, std::forward<Key>(key), std::forward<Value>(value), tableindex);
		}

		template <bool global = false, typename Key, typename Value>
		void raw_set_field(lua_State* L, Key&& key, Value&& value) {
			set_field<global, true>(L, std::forward<Key>(key), std::forward<Value>(value));
		}

		template <bool global = false, typename Key, typename Value>
		void raw_set_field(lua_State* L, Key&& key, Value&& value, int tableindex) {
			set_field<global, true>(L, std::forward<Key>(key), std::forward<Value>(value), tableindex);
		}

		template <typename T, typename F>
		inline void modify_unique_usertype_as(const stack_reference& obj, F&& f) {
			typedef unique_usertype_traits<T> u_traits;
			void* raw = lua_touserdata(obj.lua_state(), obj.stack_index());
			void* ptr_memory = detail::align_usertype_pointer(raw);
			void* uu_memory = detail::align_usertype_unique<T>(raw);
			T& uu = *static_cast<T*>(uu_memory);
			f(uu);
			*static_cast<void**>(ptr_memory) = static_cast<void*>(u_traits::get(uu));
		}

		template <typename F>
		inline void modify_unique_usertype(const stack_reference& obj, F&& f) {
			typedef meta::bind_traits<meta::unqualified_t<F>> bt;
			typedef typename bt::template arg_at<0> T;
			modify_unique_usertype_as<meta::unqualified_t<T>>(obj, std::forward<F>(f));
		}
	} // namespace stack
} // namespace sol

#endif // SOL_STACK_CORE_HPP
