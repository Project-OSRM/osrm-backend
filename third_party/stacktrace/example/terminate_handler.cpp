// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/array.hpp>
BOOST_NOINLINE void foo(int i);
BOOST_NOINLINE void bar(int i);
 
BOOST_NOINLINE void bar(int i) {
    boost::array<int, 5> a = {{-1, -231, -123, -23, -32}};
    if (i >= 0) {
        foo(a[i]);
    } else {
        std::terminate();
    }
}

BOOST_NOINLINE void foo(int i) {
    bar(--i);
}

inline void ignore_exit(int){ std::exit(0); }
#define _Exit ignore_exit

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//[getting_started_terminate_handlers

#include <exception>    // std::set_terminate, std::abort
#include <signal.h>     // ::signal
#include <boost/stacktrace.hpp>
#include <iostream>     // std::cerr

void my_terminate_handler() {
    std::cerr << "Terminate called:\n" << boost::stacktrace::stacktrace() << '\n';
    std::abort();
}

void my_signal_handler(int signum) {
    ::signal(signum, SIG_DFL);
    boost::stacktrace::stacktrace bt;
    if (bt) {
        std::cerr << "Signal " << signum << ", backtrace:\n" << boost::stacktrace::stacktrace() << '\n'; // ``[footnote Strictly speaking this code is not async-signal-safe, because it uses std::cerr. [link boost_stacktrace.build_macros_and_backends Section "Build, Macros and Backends"] describes async-signal-safe backends, so if you will use the noop backend code becomes absolutely valid as that backens always returns 0 frames and `operator<<` will be never called. ]``
    }
    _Exit(-1);
}
//]

void setup_handlers() {
//[getting_started_setup_handlers
    std::set_terminate(&my_terminate_handler);
    ::signal(SIGSEGV, &my_signal_handler);
    ::signal(SIGABRT, &my_signal_handler);
//]
}


int main() {
    setup_handlers();
    foo(5);
    
    return 2;
}


