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

#ifndef SOL_FILTERS_HPP
#define SOL_FILTERS_HPP

#include "traits.hpp"
#include <array>

namespace sol {
	namespace detail {
		struct filter_base_tag {};
	} // namespace detail

	template <int Target, int... In>
	struct static_stack_dependencies : detail::filter_base_tag {};
	typedef static_stack_dependencies<-1, 1> self_dependency;
	template <int... In>
	struct returns_self_with : detail::filter_base_tag {};
	typedef returns_self_with<> returns_self;

	struct stack_dependencies : detail::filter_base_tag {
		int target;
		std::array<int, 64> stack_indices;
		std::size_t len;

		template <typename... Args>
		stack_dependencies(int stack_target, Args&&... args)
		: target(stack_target), stack_indices(), len(sizeof...(Args)) {
			std::size_t i = 0;
			(void)detail::swallow{int(), (stack_indices[i++] = static_cast<int>(std::forward<Args>(args)), int())...};
		}

		int& operator[](std::size_t i) {
			return stack_indices[i];
		}

		const int& operator[](std::size_t i) const {
			return stack_indices[i];
		}

		std::size_t size() const {
			return len;
		}
	};

	template <typename F, typename... Filters>
	struct filter_wrapper {
		typedef std::index_sequence_for<Filters...> indices;

		F value;
		std::tuple<Filters...> filters;

		template <typename Fx, typename... Args, meta::enable<meta::neg<std::is_same<meta::unqualified_t<Fx>, filter_wrapper>>> = meta::enabler>
		filter_wrapper(Fx&& fx, Args&&... args)
		: value(std::forward<Fx>(fx)), filters(std::forward<Args>(args)...) {
		}

		filter_wrapper(const filter_wrapper&) = default;
		filter_wrapper& operator=(const filter_wrapper&) = default;
		filter_wrapper(filter_wrapper&&) = default;
		filter_wrapper& operator=(filter_wrapper&&) = default;
	};

	template <typename F, typename... Args>
	auto filters(F&& f, Args&&... args) {
		return filter_wrapper<std::decay_t<F>, std::decay_t<Args>...>(std::forward<F>(f), std::forward<Args>(args)...);
	}
} // namespace sol

#endif // SOL_FILTERS_HPP
