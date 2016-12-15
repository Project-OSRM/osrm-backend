// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_STACKTRACE_HPP
#define BOOST_STACKTRACE_STACKTRACE_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/core/explicit_operator_bool.hpp>

#include <iosfwd>
#include <string>

#include <boost/stacktrace/stacktrace_fwd.hpp>
#include <boost/stacktrace/detail/backend.hpp>
#include <boost/stacktrace/frame.hpp>
#include <boost/stacktrace/const_iterator.hpp>


namespace boost { namespace stacktrace {

/// Class that on construction copies minimal information about call stack into its internals and provides access to that information.
/// @tparam Depth Max stack frames count that this class may hold. Equal to basic_stacktrace::max_depth.
template <std::size_t Depth>
class basic_stacktrace {
    /// @cond
    void* impl_[Depth ? Depth : 1];
    boost::stacktrace::detail::backend back_;
    /// @endcond

public:
    /// Max stack frames count that this class may hold. Equal to Depth template parameter.
    BOOST_STATIC_CONSTEXPR std::size_t max_depth = Depth;

    typedef frame                                   reference;

    typedef boost::stacktrace::const_iterator       iterator;
    typedef boost::stacktrace::const_iterator       const_iterator;
    typedef std::reverse_iterator<iterator>         reverse_iterator;
    typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

    /// @brief Stores the current function call sequence inside the class.
    ///
    /// @b Complexity: O(N) where N is call sequence length, O(1) for noop backend.
    ///
    /// @b Async-Handler-Safety: Depends on backend, see "Build, Macros and Backends" section.
    BOOST_FORCEINLINE basic_stacktrace() BOOST_NOEXCEPT
        : impl_()
        , back_(impl_, Depth)
    {}

    /// @b Complexity: O(st.size())
    ///
    /// @b Async-Handler-Safety: Safe.
    basic_stacktrace(const basic_stacktrace& st) BOOST_NOEXCEPT
        : impl_()
        , back_(st.back_, impl_)
    {}

    /// @b Complexity: O(st.size())
    ///
    /// @b Async-Handler-Safety: Safe.
    basic_stacktrace& operator=(const basic_stacktrace& st) BOOST_NOEXCEPT {
        back_ = st.back_;

        return *this;
    }

    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    ~basic_stacktrace() BOOST_NOEXCEPT {}

    /// @returns Number of function names stored inside the class.
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    std::size_t size() const BOOST_NOEXCEPT {
        return back_.size();
    }

    /// @param frame_no Zero based index of frame to return. 0
    /// is the function index where stacktrace was constructed and
    /// index close to this->size() contains function `main()`.
    /// @returns frame that references the actual frame info, stored inside *this.
    ///
    /// @b Complexity: Amortized O(1), O(1) for noop backend.
    ///
    /// @b Async-Handler-Safety: Safe.
    frame operator[](std::size_t frame_no) const BOOST_NOEXCEPT {
        return *(cbegin() + frame_no);
    }


    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_iterator begin() const BOOST_NOEXCEPT { return const_iterator(&back_, 0); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_iterator cbegin() const BOOST_NOEXCEPT { return const_iterator(&back_, 0); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_iterator end() const BOOST_NOEXCEPT { return const_iterator(&back_, size()); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_iterator cend() const BOOST_NOEXCEPT { return const_iterator(&back_, size()); }

    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_reverse_iterator rbegin() const BOOST_NOEXCEPT { return const_reverse_iterator( const_iterator(&back_, 0) ); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_reverse_iterator crbegin() const BOOST_NOEXCEPT { return const_reverse_iterator( const_iterator(&back_, 0) ); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_reverse_iterator rend() const BOOST_NOEXCEPT { return const_reverse_iterator( const_iterator(&back_, size()) ); }
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    const_reverse_iterator crend() const BOOST_NOEXCEPT { return const_reverse_iterator( const_iterator(&back_, size()) ); }


    /// @brief Allows to check that stack trace capturing was successful.
    /// @returns `true` if `this->size() != 0`
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()


    /// @brief Allows to check that stack trace failed.
    /// @returns `true` if `this->size() == 0`
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    bool empty() const BOOST_NOEXCEPT { return !size(); }

    /// @brief Compares stacktraces for less, order is platform dependant.
    ///
    /// @b Complexity: Amortized O(1); worst case O(size())
    ///
    /// @b Async-Handler-Safety: Safe.
    bool operator< (const basic_stacktrace& rhs) const BOOST_NOEXCEPT {
        return back_ < rhs.back_;
    }

    /// @brief Compares stacktraces for equality.
    ///
    /// @b Complexity: Amortized O(1); worst case O(size())
    ///
    /// @b Async-Handler-Safety: Safe.
    bool operator==(const basic_stacktrace& rhs) const BOOST_NOEXCEPT {
        return back_ == rhs.back_;
    }

    /// @brief Returns hashed code of the stacktrace.
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    std::size_t hash_code() const BOOST_NOEXCEPT { return back_.hash_code(); }

    /// @cond
    bool operator!() const BOOST_NOEXCEPT { return !size(); }
    /// @endcond
};


/// Comparison operators that provide platform dependant ordering and have amortized O(1) complexity; O(size()) worst case complexity; are Async-Handler-Safe.
template <std::size_t Depth>
bool operator> (const basic_stacktrace<Depth>& lhs, const basic_stacktrace<Depth>& rhs) BOOST_NOEXCEPT {
    return rhs < lhs;
}

template <std::size_t Depth>
bool operator<=(const basic_stacktrace<Depth>& lhs, const basic_stacktrace<Depth>& rhs) BOOST_NOEXCEPT {
    return !(lhs > rhs);
}

template <std::size_t Depth>
bool operator>=(const basic_stacktrace<Depth>& lhs, const basic_stacktrace<Depth>& rhs) BOOST_NOEXCEPT {
    return !(lhs < rhs);
}

template <std::size_t Depth>
bool operator!=(const basic_stacktrace<Depth>& lhs, const basic_stacktrace<Depth>& rhs) BOOST_NOEXCEPT {
    return !(lhs == rhs);
}

/// Hashing support, O(1) complexity; Async-Handler-Safe.
template <std::size_t Depth>
std::size_t hash_value(const basic_stacktrace<Depth>& st) BOOST_NOEXCEPT {
    return st.hash_code();
}

/// Outputs stacktrace in a human readable format to output stream; unsafe to use in async handlers.
template <class CharT, class TraitsT, std::size_t Depth>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, const basic_stacktrace<Depth>& bt) {
    const std::streamsize w = os.width();
    const std::size_t frames = bt.size();
    for (std::size_t i = 0; i < frames; ++i) {
        os.width(2);
        os << i;
        os.width(w);
        os << "# ";
        os << bt[i];
        os << '\n';
    }

    return os;
}

typedef basic_stacktrace<BOOST_STACKTRACE_DEFAULT_MAX_DEPTH> stacktrace;

}} // namespace boost::stacktrace

#endif // BOOST_STACKTRACE_STACKTRACE_HPP
