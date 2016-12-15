// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_BACKEND_IPP
#define BOOST_STACKTRACE_DETAIL_BACKEND_IPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/stacktrace/detail/backend.hpp>

#if defined(BOOST_STACKTRACE_USE_NOOP)
#   include <boost/stacktrace/detail/backend_noop.hpp>
#elif defined(BOOST_STACKTRACE_USE_WINDBG)
#   include <boost/stacktrace/detail/backend_windows.hpp>
#elif defined(BOOST_STACKTRACE_USE_BACKTRACE) || defined(BOOST_STACKTRACE_USE_UNWIND)
#   include <boost/stacktrace/detail/backend_linux.hpp>
#else
#   error No suitable backtrace backend found
#endif

#endif // BOOST_STACKTRACE_DETAIL_BACKEND_IPP
