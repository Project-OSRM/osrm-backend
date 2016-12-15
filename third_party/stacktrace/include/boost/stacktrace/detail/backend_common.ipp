// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_BACKEND_COMMON_IPP
#define BOOST_STACKTRACE_DETAIL_BACKEND_COMMON_IPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <algorithm>

namespace boost { namespace stacktrace { namespace detail {

bool backend::operator< (const backend& rhs) const BOOST_NOEXCEPT {
    if (frames_count_ != rhs.frames_count_) {
        return frames_count_ < rhs.frames_count_;
    } else if (hash_code_ != rhs.hash_code_) {
        return hash_code_ < rhs.hash_code_;
    } else if (data_ == rhs.data_) {
        return false;
    }

    return std::lexicographical_compare(
        data_, data_ + frames_count_,
        rhs.data_, rhs.data_ + rhs.frames_count_
    );
}

bool backend::operator==(const backend& rhs) const BOOST_NOEXCEPT {
    if (hash_code_ != rhs.hash_code_ || frames_count_ != rhs.frames_count_) {
        return false;
    } else if (data_ == rhs.data_) {
        return true;
    }

    return std::equal(
        data_, data_ + frames_count_,
        rhs.data_
    );
}

}}}

#endif // BOOST_STACKTRACE_DETAIL_BACKEND_COMMON_IPP
