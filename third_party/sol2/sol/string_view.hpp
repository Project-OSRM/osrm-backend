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

#ifndef SOL_STRING_VIEW_HPP
#define SOL_STRING_VIEW_HPP

#include "feature_test.hpp"
#include <cstddef>
#include <string>
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
#include <string_view>
#endif // C++17 features
#include <functional>
#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
#include <boost/functional/hash.hpp>
#endif

namespace sol {
#if defined(SOL_CXX17_FEATURES) && SOL_CXX17_FEATURES
	template <typename C, typename T = std::char_traits<C>>
	using basic_string_view = std::basic_string_view<C, T>;
	typedef std::string_view string_view;
	typedef std::wstring_view wstring_view;
	typedef std::u16string_view u16string_view;
	typedef std::u32string_view u32string_view;
	typedef std::hash<std::string_view> string_view_hash;
#else
	template <typename Char, typename Traits = std::char_traits<Char>>
	struct basic_string_view {
		std::size_t s;
		const Char* p;

		basic_string_view(const std::string& r)
		: basic_string_view(r.data(), r.size()) {
		}
		basic_string_view(const Char* ptr)
		: basic_string_view(ptr, Traits::length(ptr)) {
		}
		basic_string_view(const Char* ptr, std::size_t sz)
		: s(sz), p(ptr) {
		}

		static int compare(const Char* lhs_p, std::size_t lhs_sz, const Char* rhs_p, std::size_t rhs_sz) {
			int result = Traits::compare(lhs_p, rhs_p, lhs_sz < rhs_sz ? lhs_sz : rhs_sz);
			if (result != 0)
				return result;
			if (lhs_sz < rhs_sz)
				return -1;
			if (lhs_sz > rhs_sz)
				return 1;
			return 0;
		}

		const Char* begin() const {
			return p;
		}

		const Char* end() const {
			return p + s;
		}

		const Char* cbegin() const {
			return p;
		}

		const Char* cend() const {
			return p + s;
		}

		const Char* data() const {
			return p;
		}

		std::size_t size() const {
			return s;
		}

		std::size_t length() const {
			return size();
		}

		operator std::basic_string<Char, Traits>() const {
			return std::basic_string<Char, Traits>(data(), size());
		}

		bool operator==(const basic_string_view& r) const {
			return compare(p, s, r.data(), r.size()) == 0;
		}

		bool operator==(const Char* r) const {
			return compare(r, Traits::length(r), p, s) == 0;
		}

		bool operator==(const std::basic_string<Char, Traits>& r) const {
			return compare(r.data(), r.size(), p, s) == 0;
		}

		bool operator!=(const basic_string_view& r) const {
			return !(*this == r);
		}

		bool operator!=(const char* r) const {
			return !(*this == r);
		}

		bool operator!=(const std::basic_string<Char, Traits>& r) const {
			return !(*this == r);
		}
	};

	template <typename Ch, typename Tr = std::char_traits<Ch>>
	struct basic_string_view_hash {
		typedef basic_string_view<Ch, Tr> argument_type;
		typedef std::size_t result_type;

		template <typename Al>
		result_type operator()(const std::basic_string<Ch, Tr, Al>& r) const {
			return (*this)(argument_type(r.c_str(), r.size()));
		}

		result_type operator()(const argument_type& r) const {
#if defined(SOL_USE_BOOST) && SOL_USE_BOOST
			return boost::hash_range(r.begin(), r.end());
#else
			// Modified, from libstdc++
			// An implementation attempt at Fowler No Voll, 1a.
			// Supposedly, used in MSVC,
			// GCC (libstdc++) uses MurmurHash of some sort for 64-bit though...?
			// But, well. Can't win them all, right?
			// This should normally only apply when NOT using boost,
			// so this should almost never be tapped into...
			std::size_t hash = 0;
			const unsigned char* cptr = reinterpret_cast<const unsigned char*>(r.data());
			for (std::size_t sz = r.size(); sz != 0; --sz) {
				hash ^= static_cast<size_t>(*cptr++);
				hash *= static_cast<size_t>(1099511628211ULL);
			}
			return hash; 
#endif
		}
	};
} // namespace sol

namespace std {
	template <typename Ch, typename Tr>
	struct hash< ::sol::basic_string_view<Ch, Tr> > : ::sol::basic_string_view_hash<Ch, Tr> {};
} // namespace std

namespace sol {
	using string_view = basic_string_view<char>;
	using wstring_view = basic_string_view<wchar_t>;
	using u16string_view = basic_string_view<char16_t>;
	using u32string_view = basic_string_view<char32_t>;
	using string_view_hash = std::hash<string_view>;
#endif // C++17 Support
} // namespace sol

#endif // SOL_STRING_VIEW_HPP
