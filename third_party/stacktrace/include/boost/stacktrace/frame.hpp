// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_FRAME_HPP
#define BOOST_STACKTRACE_FRAME_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <iosfwd>
#include <string>

#include <boost/core/explicit_operator_bool.hpp>
#include <boost/stacktrace/detail/backend.hpp>

namespace boost { namespace stacktrace {

/// Non-owning class that references the frame information stored inside the boost::stacktrace::stacktrace class.
class frame {
    /// @cond
    const void* addr_;
    /// @endcond

public:
    /// @brief Constructs frame that references NULL address.
    /// Calls to source_file() and source_line() wil lreturn empty string.
    /// Calls to source_line() will return 0.
    ///
    /// @b Complexity: O(1).
    ///
    /// @b Async-Handler-Safety: Safe.
    /// @throws Nothing.
    frame() BOOST_NOEXCEPT
        : addr_(0)
    {}

#ifdef BOOST_STACKTRACE_DOXYGEN_INVOKED
    /// @brief Copy constructs frame.
    ///
    /// @b Complexity: O(1).
    ///
    /// @b Async-Handler-Safety: Safe.
    /// @throws Nothing.
    frame(const frame&) = default;

    /// @brief Copy assigns frame.
    ///
    /// @b Complexity: O(1).
    ///
    /// @b Async-Handler-Safety: Safe.
    /// @throws Nothing.
    frame& operator=(const frame&) = default;
#endif

    /// @brief Constructs frame that can extract information from addr at runtime.
    ///
    /// @b Complexity: O(1).
    ///
    /// @b Async-Handler-Safety: Safe.
    /// @throws Nothing.
    explicit frame(const void* addr) BOOST_NOEXCEPT
        : addr_(addr)
    {}

    /// @returns Name of the frame (function name in a human readable form).
    ///
    /// @b Async-Handler-Safety: Unsafe.
    /// @throws std::bad_alloc if not enough memory to construct resulting string.
    std::string name() const {
        return boost::stacktrace::detail::backend::get_name(address());
    }

    /// @returns Address of the frame function.
    ///
    /// @b Async-Handler-Safety: Safe.
    /// @throws Nothing.
    const void* address() const BOOST_NOEXCEPT {
        return addr_;
    }

    /// @returns Path to the source file, were the function of the frame is defined. Returns empty string
    /// if this->source_line() == 0.
    /// @throws std::bad_alloc if not enough memory to construct resulting string.
    ///
    /// @b Async-Handler-Safety: Unsafe.
    std::string source_file() const {
        return boost::stacktrace::detail::backend::get_source_file(address());
    }

    /// @returns Code line in the source file, were the function of the frame is defined.
    /// @throws std::bad_alloc if not enough memory to construct string for internal needs.
    ///
    /// @b Async-Handler-Safety: Unsafe.
    std::size_t source_line() const {
        return boost::stacktrace::detail::backend::get_source_line(address());
    }

    /// @brief Checks that frame is not references NULL address.
    /// @returns `true` if `this->address() != 0`
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()

    /// @brief Checks that frame references NULL address.
    /// @returns `true` if `this->address() == 0`
    ///
    /// @b Complexity: O(1)
    ///
    /// @b Async-Handler-Safety: Safe.
    bool empty() const BOOST_NOEXCEPT { return !address(); }
    
    /// @cond
    bool operator!() const BOOST_NOEXCEPT { return !address(); }
    /// @endcond
};

/// Comparison operators that provide platform dependant ordering and have O(1) complexity; are Async-Handler-Safe.
inline bool operator< (const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return lhs.address() < rhs.address(); }
inline bool operator> (const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return rhs < lhs; }
inline bool operator<=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs > rhs); }
inline bool operator>=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs < rhs); }
inline bool operator==(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return lhs.address() == rhs.address(); }
inline bool operator!=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs == rhs); }

/// Hashing support, O(1) complexity; Async-Handler-Safe.
inline std::size_t hash_value(const frame& f) BOOST_NOEXCEPT {
    return reinterpret_cast<std::size_t>(f.address());
}

/// Outputs stacktrace::frame in a human readable format to output stream; unsafe to use in async handlers.
template <class CharT, class TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, const frame& f) {
    std::string name = f.name();
    if (!name.empty()) {
        os << name;
    } else {
        os << f.address();
    }

    const std::size_t source_line = f.source_line();
    if (source_line) {
        os << " at " << f.source_file() << ':' << source_line;
    }

    return os;
}

}} // namespace boost::stacktrace

#endif // BOOST_STACKTRACE_FRAME_HPP
