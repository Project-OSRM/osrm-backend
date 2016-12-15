// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_BACKEND_NOOP_HPP
#define BOOST_STACKTRACE_DETAIL_BACKEND_NOOP_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

namespace boost { namespace stacktrace { namespace detail {


backend::backend(void** /*memory*/, std::size_t /*size*/) BOOST_NOEXCEPT
    : hash_code_(0)
    , frames_count_(0)
    , data_(0)
{}

std::string backend::get_name(const void* /*addr*/) {
    return std::string();
}

std::string backend::get_source_file(const void* /*addr*/) {
    return std::string();
}

std::size_t backend::get_source_line(const void* /*addr*/) {
    return 0;
}

bool backend::operator< (const backend& /*rhs*/) const BOOST_NOEXCEPT {
    return false;
}

bool backend::operator==(const backend& /*rhs*/) const BOOST_NOEXCEPT {
    return true;
}

}}} // namespace boost::stacktrace::detail

#endif // BOOST_STACKTRACE_DETAIL_BACKEND_LIBUNWIND_HPP
