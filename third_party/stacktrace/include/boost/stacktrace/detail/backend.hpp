// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_BACKEND_HPP
#define BOOST_STACKTRACE_DETAIL_BACKEND_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <string>


// Link or header only
#if !defined(BOOST_STACKTRACE_LINK) && defined(BOOST_STACKTRACE_DYN_LINK)
#   define BOOST_STACKTRACE_LINK
#endif

#if defined(BOOST_STACKTRACE_LINK) && !defined(BOOST_STACKTRACE_DYN_LINK) && defined(BOOST_ALL_DYN_LINK)
#   define BOOST_STACKTRACE_DYN_LINK
#endif

// Backend autodetection
#if !defined(BOOST_STACKTRACE_USE_NOOP) && !defined(BOOST_STACKTRACE_USE_WINDBG) && !defined(BOOST_STACKTRACE_USE_UNWIND) \
    && !defined(BOOST_STACKTRACE_USE_BACKTRACE) && !defined(BOOST_STACKTRACE_USE_HEADER)

#if defined(__has_include) && (!defined(__GNUC__) || __GNUC__ > 4 || BOOST_CLANG)
#   if __has_include("Dbgeng.h")
#       define BOOST_STACKTRACE_USE_WINDBG
#   else
#       define BOOST_STACKTRACE_USE_UNWIND
#   endif
#else
#   if defined(BOOST_WINDOWS)
#       define BOOST_STACKTRACE_USE_WINDBG
#   else
#       define BOOST_STACKTRACE_USE_UNWIND
#   endif
#endif

#endif

#ifdef BOOST_STACKTRACE_LINK
#   if defined(BOOST_STACKTRACE_DYN_LINK)
#       ifdef BOOST_STACKTRACE_INTERNAL_BUILD_LIBS
#           define BOOST_STACKTRACE_FUNCTION BOOST_SYMBOL_EXPORT
#       else
#           define BOOST_STACKTRACE_FUNCTION BOOST_SYMBOL_IMPORT
#       endif
#   else
#       define BOOST_STACKTRACE_FUNCTION
#   endif
#else
#   define BOOST_STACKTRACE_FUNCTION inline
#endif

namespace boost { namespace stacktrace { namespace detail {

// Class that implements the actual backtracing
class backend {
    std::size_t         hash_code_;
    std::size_t         frames_count_;
    void**              data_;

    void copy_frames_from(const backend& b) BOOST_NOEXCEPT {
        if (data_ == b.data_) {
            return;
        }

        for (std::size_t i = 0; i < frames_count_; ++i) {
            data_[i] = b.data_[i];
        }
    }

public:
    BOOST_STACKTRACE_FUNCTION backend(void** memory, std::size_t size) BOOST_NOEXCEPT;
    BOOST_STACKTRACE_FUNCTION static std::string get_name(const void* addr);
    const void* get_address(std::size_t frame_no) const BOOST_NOEXCEPT {
        return frame_no < frames_count_ ? data_[frame_no] : 0;
    }
    BOOST_STACKTRACE_FUNCTION static std::string get_source_file(const void* addr);
    BOOST_STACKTRACE_FUNCTION static std::size_t get_source_line(const void* addr);
    BOOST_STACKTRACE_FUNCTION bool operator< (const backend& rhs) const BOOST_NOEXCEPT;
    BOOST_STACKTRACE_FUNCTION bool operator==(const backend& rhs) const BOOST_NOEXCEPT;

    backend(const backend& b, void** memory) BOOST_NOEXCEPT
        : hash_code_(b.hash_code_)
        , frames_count_(b.frames_count_)
        , data_(memory)
    {
        copy_frames_from(b);
    }

    backend& operator=(const backend& b) BOOST_NOEXCEPT {
        hash_code_ = b.hash_code_;
        frames_count_ = b.frames_count_;
        copy_frames_from(b);

        return *this;
    }

    std::size_t size() const BOOST_NOEXCEPT {
        return frames_count_;
    }

    std::size_t hash_code() const BOOST_NOEXCEPT {
        return hash_code_;
    }
};

}}} // namespace boost::stacktrace::detail

/// @cond
#undef BOOST_STACKTRACE_FUNCTION

#ifndef BOOST_STACKTRACE_LINK
#   include <boost/stacktrace/detail/backend.ipp>
#endif
/// @endcond

#endif // BOOST_STACKTRACE_DETAIL_BACKEND_HPP
