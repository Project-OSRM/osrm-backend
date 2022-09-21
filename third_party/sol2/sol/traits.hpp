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

#ifndef SOL_TRAITS_HPP
#define SOL_TRAITS_HPP

#include "tuple.hpp"
#include "bind_traits.hpp"
#include "string_view.hpp"

#include <type_traits>
#include <cstdint>
#include <memory>
#include <functional>
#include <array>
#include <iterator>
#include <iosfwd>


namespace sol {
	template <std::size_t I>
	using index_value = std::integral_constant<std::size_t, I>;

	namespace meta {
		typedef std::array<char, 1> sfinae_yes_t;
		typedef std::array<char, 2> sfinae_no_t;

		template <typename T>
		struct identity { typedef T type; };

		template <typename T>
		using identity_t = typename identity<T>::type;

		template <typename... Args>
		struct is_tuple : std::false_type {};

		template <typename... Args>
		struct is_tuple<std::tuple<Args...>> : std::true_type {};

		template <typename T>
		struct is_builtin_type : std::integral_constant<bool, std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_array<T>::value> {};

		template <typename T>
		struct unwrapped {
			typedef T type;
		};

		template <typename T>
		struct unwrapped<std::reference_wrapper<T>> {
			typedef T type;
		};

		template <typename T>
		using unwrapped_t = typename unwrapped<T>::type;

		template <typename T>
		struct unwrap_unqualified : unwrapped<unqualified_t<T>> {};

		template <typename T>
		using unwrap_unqualified_t = typename unwrap_unqualified<T>::type;

		template <typename T>
		struct remove_member_pointer;

		template <typename R, typename T>
		struct remove_member_pointer<R T::*> {
			typedef R type;
		};

		template <typename R, typename T>
		struct remove_member_pointer<R T::*const> {
			typedef R type;
		};

		template <typename T>
		using remove_member_pointer_t = remove_member_pointer<T>;

		namespace meta_detail {
			template <typename T, template <typename...> class Templ>
			struct is_specialization_of : std::false_type {};
			template <typename... T, template <typename...> class Templ>
			struct is_specialization_of<Templ<T...>, Templ> : std::true_type {};
		}

		template <typename T, template <typename...> class Templ>
		using is_specialization_of = meta_detail::is_specialization_of<std::remove_cv_t<T>, Templ>;

		template <class T, class...>
		struct all_same : std::true_type {};

		template <class T, class U, class... Args>
		struct all_same<T, U, Args...> : std::integral_constant<bool, std::is_same<T, U>::value && all_same<T, Args...>::value> {};

		template <class T, class...>
		struct any_same : std::false_type {};

		template <class T, class U, class... Args>
		struct any_same<T, U, Args...> : std::integral_constant<bool, std::is_same<T, U>::value || any_same<T, Args...>::value> {};

		template <bool B>
		using boolean = std::integral_constant<bool, B>;

		template <typename T>
		using invoke_t = typename T::type;

		template <typename T>
		using invoke_b = boolean<T::value>;

		template <typename T>
		using neg = boolean<!T::value>;

		template <typename Condition, typename Then, typename Else>
		using condition = std::conditional_t<Condition::value, Then, Else>;

		template <typename... Args>
		struct all : boolean<true> {};

		template <typename T, typename... Args>
		struct all<T, Args...> : condition<T, all<Args...>, boolean<false>> {};

		template <typename... Args>
		struct any : boolean<false> {};

		template <typename T, typename... Args>
		struct any<T, Args...> : condition<T, boolean<true>, any<Args...>> {};

		enum class enable_t {
			_
		};

		constexpr const auto enabler = enable_t::_;

		template <bool value, typename T = void>
		using disable_if_t = std::enable_if_t<!value, T>;

		template <typename... Args>
		using enable = std::enable_if_t<all<Args...>::value, enable_t>;

		template <typename... Args>
		using disable = std::enable_if_t<neg<all<Args...>>::value, enable_t>;

		template <typename... Args>
		using enable_any = std::enable_if_t<any<Args...>::value, enable_t>;

		template <typename... Args>
		using disable_any = std::enable_if_t<neg<any<Args...>>::value, enable_t>;

		template <typename V, typename... Vs>
		struct find_in_pack_v : boolean<false> {};

		template <typename V, typename Vs1, typename... Vs>
		struct find_in_pack_v<V, Vs1, Vs...> : any<boolean<(V::value == Vs1::value)>, find_in_pack_v<V, Vs...>> {};

		namespace meta_detail {
			template <std::size_t I, typename T, typename... Args>
			struct index_in_pack : std::integral_constant<std::size_t, SIZE_MAX> {};

			template <std::size_t I, typename T, typename T1, typename... Args>
			struct index_in_pack<I, T, T1, Args...> : std::conditional_t<std::is_same<T, T1>::value, std::integral_constant<std::ptrdiff_t, I>, index_in_pack<I + 1, T, Args...>> {};
		} // namespace meta_detail

		template <typename T, typename... Args>
		struct index_in_pack : meta_detail::index_in_pack<0, T, Args...> {};

		template <typename T, typename List>
		struct index_in : meta_detail::index_in_pack<0, T, List> {};

		template <typename T, typename... Args>
		struct index_in<T, types<Args...>> : meta_detail::index_in_pack<0, T, Args...> {};

		template <std::size_t I, typename... Args>
		struct at_in_pack {};

		template <std::size_t I, typename... Args>
		using at_in_pack_t = typename at_in_pack<I, Args...>::type;

		template <std::size_t I, typename Arg, typename... Args>
		struct at_in_pack<I, Arg, Args...> : std::conditional<I == 0, Arg, at_in_pack_t<I - 1, Args...>> {};

		template <typename Arg, typename... Args>
		struct at_in_pack<0, Arg, Args...> { typedef Arg type; };

		namespace meta_detail {
			template <std::size_t Limit, std::size_t I, template <typename...> class Pred, typename... Ts>
			struct count_for_pack : std::integral_constant<std::size_t, 0> {};
			template <std::size_t Limit, std::size_t I, template <typename...> class Pred, typename T, typename... Ts>
						struct count_for_pack<Limit, I, Pred, T, Ts...> : std::conditional_t < sizeof...(Ts)
					== 0
				|| Limit<2,
					   std::integral_constant<std::size_t, I + static_cast<std::size_t>(Limit != 0 && Pred<T>::value)>,
					   count_for_pack<Limit - 1, I + static_cast<std::size_t>(Pred<T>::value), Pred, Ts...>> {};
			template <std::size_t I, template <typename...> class Pred, typename... Ts>
			struct count_2_for_pack : std::integral_constant<std::size_t, 0> {};
			template <std::size_t I, template <typename...> class Pred, typename T, typename U, typename... Ts>
			struct count_2_for_pack<I, Pred, T, U, Ts...> : std::conditional_t<sizeof...(Ts) == 0,
													   std::integral_constant<std::size_t, I + static_cast<std::size_t>(Pred<T>::value)>,
													   count_2_for_pack<I + static_cast<std::size_t>(Pred<T>::value), Pred, Ts...>> {};
		} // namespace meta_detail

		template <template <typename...> class Pred, typename... Ts>
		struct count_for_pack : meta_detail::count_for_pack<sizeof...(Ts), 0, Pred, Ts...> {};

		template <template <typename...> class Pred, typename List>
		struct count_for;

		template <template <typename...> class Pred, typename... Args>
		struct count_for<Pred, types<Args...>> : count_for_pack<Pred, Args...> {};

		template <std::size_t Limit, template <typename...> class Pred, typename... Ts>
		struct count_for_to_pack : meta_detail::count_for_pack<Limit, 0, Pred, Ts...> {};

		template <template <typename...> class Pred, typename... Ts>
		struct count_2_for_pack : meta_detail::count_2_for_pack<0, Pred, Ts...> {};

		template <typename... Args>
		struct return_type {
			typedef std::tuple<Args...> type;
		};

		template <typename T>
		struct return_type<T> {
			typedef T type;
		};

		template <>
		struct return_type<> {
			typedef void type;
		};

		template <typename... Args>
		using return_type_t = typename return_type<Args...>::type;

		namespace meta_detail {
			template <typename>
			struct always_true : std::true_type {};
			struct is_invokable_tester {
				template <typename Fun, typename... Args>
				static always_true<decltype(std::declval<Fun>()(std::declval<Args>()...))> test(int);
				template <typename...>
				static std::false_type test(...);
			};
		} // namespace meta_detail

		template <typename T>
		struct is_invokable;
		template <typename Fun, typename... Args>
		struct is_invokable<Fun(Args...)> : decltype(meta_detail::is_invokable_tester::test<Fun, Args...>(0)) {};

		namespace meta_detail {

			template <typename T, typename = void>
			struct is_callable : std::is_function<std::remove_pointer_t<T>> {};

			template <typename T>
			struct is_callable<T, std::enable_if_t<std::is_final<unqualified_t<T>>::value 
				&& std::is_class<unqualified_t<T>>::value
				&&  std::is_same<decltype(void(&T::operator())), void>::value>> {

			};

			template <typename T>
			struct is_callable<T, std::enable_if_t<!std::is_final<unqualified_t<T>>::value && std::is_class<unqualified_t<T>>::value && std::is_destructible<unqualified_t<T>>::value>> {
				using yes = char;
				using no = struct { char s[2]; };

				struct F {
					void operator()();
				};
				struct Derived : T, F {};
				template <typename U, U>
				struct Check;

				template <typename V>
				static no test(Check<void (F::*)(), &V::operator()>*);

				template <typename>
				static yes test(...);

				static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
			};

			template <typename T>
			struct is_callable<T, std::enable_if_t<!std::is_final<unqualified_t<T>>::value && std::is_class<unqualified_t<T>>::value && !std::is_destructible<unqualified_t<T>>::value>> {
				using yes = char;
				using no = struct { char s[2]; };

				struct F {
					void operator()();
				};
				struct Derived : T, F {
					~Derived() = delete;
				};
				template <typename U, U>
				struct Check;

				template <typename V>
				static no test(Check<void (F::*)(), &V::operator()>*);

				template <typename>
				static yes test(...);

				static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
			};

			struct has_begin_end_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename B = decltype(std::declval<U&>().begin()),
					typename E = decltype(std::declval<U&>().end())>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_key_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::key_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_mapped_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::mapped_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_value_type_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::value_type>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_iterator_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::iterator>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			struct has_key_value_pair_impl {
				template <typename T, typename U = unqualified_t<T>,
					typename V = typename U::value_type,
					typename F = decltype(std::declval<V&>().first),
					typename S = decltype(std::declval<V&>().second)>
				static std::true_type test(int);

				template <typename...>
				static std::false_type test(...);
			};

			template <typename T>
			struct has_push_back_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().push_back(std::declval<std::add_rvalue_reference_t<typename C::value_type>>())) *);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_insert_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().insert(std::declval<std::add_rvalue_reference_t<typename C::const_iterator>>(), std::declval<std::add_rvalue_reference_t<typename C::value_type>>()))*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_insert_after_test {
			private:
				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().insert_after(std::declval<std::add_rvalue_reference_t<typename C::const_iterator>>(), std::declval<std::add_rvalue_reference_t<typename C::value_type>>()))*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_size_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().size())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_max_size_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().max_size())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};

			template <typename T>
			struct has_to_string_test {
			private:
				typedef std::array<char, 1> sfinae_yes_t;
				typedef std::array<char, 2> sfinae_no_t;

				template <typename C>
				static sfinae_yes_t test(decltype(std::declval<C>().to_string())*);
				template <typename C>
				static sfinae_no_t test(...);

			public:
				static const bool value = sizeof(test<T>(0)) == sizeof(sfinae_yes_t);
			};
#if defined(_MSC_VER) && _MSC_VER <= 1910
			template <typename T, typename U, typename = decltype(std::declval<T&>() < std::declval<U&>())>
			std::true_type supports_op_less_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_less_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() == std::declval<U&>())>
			std::true_type supports_op_equal_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_equal_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() <= std::declval<U&>())>
			std::true_type supports_op_less_equal_test(std::reference_wrapper<T>, std::reference_wrapper<U>);
			std::false_type supports_op_less_equal_test(...);
			template <typename T, typename OS, typename = decltype(std::declval<OS&>() << std::declval<T&>())>
			std::true_type supports_ostream_op(std::reference_wrapper<T>, std::reference_wrapper<OS>);
			std::false_type supports_ostream_op(...);
			template <typename T, typename = decltype(to_string(std::declval<T&>()))>
			std::true_type supports_adl_to_string(std::reference_wrapper<T>);
			std::false_type supports_adl_to_string(...);
#else
			template <typename T, typename U, typename = decltype(std::declval<T&>() < std::declval<U&>())>
			std::true_type supports_op_less_test(const T&, const U&);
			std::false_type supports_op_less_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() == std::declval<U&>())>
			std::true_type supports_op_equal_test(const T&, const U&);
			std::false_type supports_op_equal_test(...);
			template <typename T, typename U, typename = decltype(std::declval<T&>() <= std::declval<U&>())>
			std::true_type supports_op_less_equal_test(const T&, const U&);
			std::false_type supports_op_less_equal_test(...);
			template <typename T, typename OS, typename = decltype(std::declval<OS&>() << std::declval<T&>())>
			std::true_type supports_ostream_op(const T&, const OS&);
			std::false_type supports_ostream_op(...);
			template <typename T, typename = decltype(to_string(std::declval<T&>()))>
			std::true_type supports_adl_to_string(const T&);
			std::false_type supports_adl_to_string(...);
#endif

			template <typename T, bool b>
			struct is_matched_lookup_impl : std::false_type {};
			template <typename T>
			struct is_matched_lookup_impl<T, true> : std::is_same<typename T::key_type, typename T::value_type> {};
		} // namespace meta_detail

#if defined(_MSC_VER) && _MSC_VER <= 1910
		template <typename T, typename U = T>
		using supports_op_less = decltype(meta_detail::supports_op_less_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = T>
		using supports_op_equal = decltype(meta_detail::supports_op_equal_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = T>
		using supports_op_less_equal = decltype(meta_detail::supports_op_less_equal_test(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T, typename U = std::ostream>
		using supports_ostream_op = decltype(meta_detail::supports_ostream_op(std::ref(std::declval<T&>()), std::ref(std::declval<U&>())));
		template <typename T>
		using supports_adl_to_string = decltype(meta_detail::supports_adl_to_string(std::ref(std::declval<T&>())));
#else
		template <typename T, typename U = T>
		using supports_op_less = decltype(meta_detail::supports_op_less_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = T>
		using supports_op_equal = decltype(meta_detail::supports_op_equal_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = T>
		using supports_op_less_equal = decltype(meta_detail::supports_op_less_equal_test(std::declval<T&>(), std::declval<U&>()));
		template <typename T, typename U = std::ostream>
		using supports_ostream_op = decltype(meta_detail::supports_ostream_op(std::declval<T&>(), std::declval<U&>()));
		template <typename T>
		using supports_adl_to_string = decltype(meta_detail::supports_adl_to_string(std::declval<T&>()));
#endif
		template <typename T>
		using supports_to_string_member = meta::boolean<meta_detail::has_to_string_test<T>::value>;

		template <typename T>
		struct is_callable : boolean<meta_detail::is_callable<T>::value> {};

		template <typename T>
		struct has_begin_end : decltype(meta_detail::has_begin_end_impl::test<T>(0)) {};

		template <typename T>
		struct has_key_value_pair : decltype(meta_detail::has_key_value_pair_impl::test<T>(0)) {};

		template <typename T>
		struct has_key_type : decltype(meta_detail::has_key_type_impl::test<T>(0)) {};

		template <typename T>
		struct has_mapped_type : decltype(meta_detail::has_mapped_type_impl::test<T>(0)) {};

		template <typename T>
		struct has_iterator : decltype(meta_detail::has_iterator_impl::test<T>(0)) {};

		template <typename T>
		struct has_value_type : decltype(meta_detail::has_value_type_impl::test<T>(0)) {};

		template <typename T>
		using has_push_back = meta::boolean<meta_detail::has_push_back_test<T>::value>;

		template <typename T>
		using has_max_size = meta::boolean<meta_detail::has_max_size_test<T>::value>;

		template <typename T>
		using has_insert = meta::boolean<meta_detail::has_insert_test<T>::value>;

		template <typename T>
		using has_insert_after = meta::boolean<meta_detail::has_insert_after_test<T>::value>;

		template <typename T>
		using has_size = meta::boolean<meta_detail::has_size_test<T>::value || meta_detail::has_size_test<const T>::value>;

		template <typename T>
		struct is_associative : meta::all<has_key_type<T>, has_key_value_pair<T>, has_mapped_type<T>> {};

		template <typename T>
		struct is_lookup : meta::all<has_key_type<T>, has_value_type<T>> {};

		template <typename T>
		struct is_matched_lookup : meta_detail::is_matched_lookup_impl<T, is_lookup<T>::value> {};

		template <typename T>
		using is_string_like = any<
			is_specialization_of<meta::unqualified_t<T>, std::basic_string>,
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			is_specialization_of<meta::unqualified_t<T>, std::basic_string_view>,
#else
			is_specialization_of<meta::unqualified_t<T>, basic_string_view>,
#endif
			meta::all<std::is_array<unqualified_t<T>>, meta::any_same<meta::unqualified_t<std::remove_all_extents_t<meta::unqualified_t<T>>>, char, char16_t, char32_t, wchar_t>>
		>;

		template <typename T>
		using is_string_constructible = any<
			meta::all<std::is_array<unqualified_t<T>>, std::is_same<meta::unqualified_t<std::remove_all_extents_t<meta::unqualified_t<T>>>, char>>,
			std::is_same<unqualified_t<T>, const char*>, 
			std::is_same<unqualified_t<T>, char>, std::is_same<unqualified_t<T>, std::string>, std::is_same<unqualified_t<T>, std::initializer_list<char>>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
			, std::is_same<unqualified_t<T>, std::string_view>
#endif
			>;

		template <typename T>
		struct is_pair : std::false_type {};

		template <typename T1, typename T2>
		struct is_pair<std::pair<T1, T2>> : std::true_type {};

		template <typename T>
		using is_c_str = any<
			std::is_same<std::decay_t<unqualified_t<T>>, const char*>,
			std::is_same<std::decay_t<unqualified_t<T>>, char*>,
			std::is_same<unqualified_t<T>, std::string>>;

		template <typename T>
		struct is_move_only : all<
							  neg<std::is_reference<T>>,
							  neg<std::is_copy_constructible<unqualified_t<T>>>,
							  std::is_move_constructible<unqualified_t<T>>> {};

		template <typename T>
		using is_not_move_only = neg<is_move_only<T>>;

		namespace meta_detail {
			template <typename T, meta::disable<meta::is_specialization_of<meta::unqualified_t<T>, std::tuple>> = meta::enabler>
			decltype(auto) force_tuple(T&& x) {
				return std::tuple<std::decay_t<T>>(std::forward<T>(x));
			}

			template <typename T, meta::enable<meta::is_specialization_of<meta::unqualified_t<T>, std::tuple>> = meta::enabler>
			decltype(auto) force_tuple(T&& x) {
				return std::forward<T>(x);
			}
		} // namespace meta_detail

		template <typename... X>
		decltype(auto) tuplefy(X&&... x) {
			return std::tuple_cat(meta_detail::force_tuple(std::forward<X>(x))...);
		}

		template <typename T, typename = void>
		struct iterator_tag {
			using type = std::input_iterator_tag;
		};

		template <typename T>
		struct iterator_tag<T, std::conditional_t<false, typename T::iterator_category, void>> {
			using type = typename T::iterator_category;
		};

	} // namespace meta

	namespace detail {
		template <typename T>
		struct is_pointer_like : std::is_pointer<T> {};
		template <typename T, typename D>
		struct is_pointer_like<std::unique_ptr<T, D>> : std::true_type {};
		template <typename T>
		struct is_pointer_like<std::shared_ptr<T>> : std::true_type {};

		template <std::size_t I, typename Tuple>
		decltype(auto) forward_get(Tuple&& tuple) {
			return std::forward<meta::tuple_element_t<I, Tuple>>(std::get<I>(tuple));
		}

		template <std::size_t... I, typename Tuple>
		auto forward_tuple_impl(std::index_sequence<I...>, Tuple&& tuple) -> decltype(std::tuple<decltype(forward_get<I>(tuple))...>(forward_get<I>(tuple)...)) {
			return std::tuple<decltype(forward_get<I>(tuple))...>(std::move(std::get<I>(tuple))...);
		}

		template <typename Tuple>
		auto forward_tuple(Tuple&& tuple) {
			auto x = forward_tuple_impl(std::make_index_sequence<std::tuple_size<meta::unqualified_t<Tuple>>::value>(), std::forward<Tuple>(tuple));
			return x;
		}

		template <typename T>
		auto unwrap(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T>
		T& unwrap(std::reference_wrapper<T> arg) {
			return arg.get();
		}

		template <typename T, meta::enable<meta::neg<is_pointer_like<meta::unqualified_t<T>>>> = meta::enabler>
		auto deref(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T, meta::enable<is_pointer_like<meta::unqualified_t<T>>> = meta::enabler>
		inline auto deref(T&& item) -> decltype(*std::forward<T>(item)) {
			return *std::forward<T>(item);
		}

		template <typename T, meta::disable<is_pointer_like<meta::unqualified_t<T>>, meta::neg<std::is_pointer<meta::unqualified_t<T>>>> = meta::enabler>
		auto deref_non_pointer(T&& item) -> decltype(std::forward<T>(item)) {
			return std::forward<T>(item);
		}

		template <typename T, meta::enable<is_pointer_like<meta::unqualified_t<T>>, meta::neg<std::is_pointer<meta::unqualified_t<T>>>> = meta::enabler>
		inline auto deref_non_pointer(T&& item) -> decltype(*std::forward<T>(item)) {
			return *std::forward<T>(item);
		}

		template <typename T>
		inline T* ptr(T& val) {
			return std::addressof(val);
		}

		template <typename T>
		inline T* ptr(std::reference_wrapper<T> val) {
			return std::addressof(val.get());
		}

		template <typename T>
		inline T* ptr(T* val) {
			return val;
		}
	} // namespace detail
} // namespace sol

#endif // SOL_TRAITS_HPP
