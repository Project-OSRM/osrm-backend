// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_STACKTRACE_FWD_HPP
#define BOOST_STACKTRACE_STACKTRACE_FWD_HPP

#include <cstddef>

/// @file stacktrace_fwd.hpp This header contains only forward declarations of
/// boost::stacktrace::frame, boost::stacktrace::const_iterator, boost::stacktrace::basic_stacktrace
/// and does not include any other Boost headers.


#ifndef BOOST_STACKTRACE_DEFAULT_MAX_DEPTH
/// You may define this macro to some positive integer to limit the max stack frames count for the boost::stacktrace::stacktrace class.
/// This macro does not affect the boost::stacktrace::basic_stacktrace.
///
/// @b Default: 100
#define BOOST_STACKTRACE_DEFAULT_MAX_DEPTH 100
#endif

namespace boost { namespace stacktrace {

class frame;

class const_iterator;

template <std::size_t Depth>
class basic_stacktrace;

typedef basic_stacktrace<BOOST_STACKTRACE_DEFAULT_MAX_DEPTH> stacktrace;

}} // namespace boost::stacktrace


#endif // BOOST_STACKTRACE_STACKTRACE_FWD_HPP
