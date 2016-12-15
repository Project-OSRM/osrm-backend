// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined(BOOST_NO_CXX11_NOEXCEPT) || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

int main(){}

#else

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//[getting_started_class_traced
#include <boost/stacktrace.hpp>

struct traced {
    const boost::stacktrace::stacktrace trace;

    virtual const char* what() const noexcept = 0;
    virtual ~traced(){}
};
//]

//[getting_started_class_with_trace
template <class Exception>
struct with_trace : public Exception, public traced {
    template <class... Args>
    with_trace(Args&&... args)
        : Exception(std::forward<Args>(args)...)
    {}

    const char* what() const noexcept {
        return Exception::what();
    }
};
//]

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOST_NOINLINE void oops(int i);
BOOST_NOINLINE void foo(int i);
BOOST_NOINLINE void bar(int i);

#include <stdexcept>
BOOST_NOINLINE void oops(int i) {
    //[getting_started_throwing_with_trace
    if (i >= 4)
        throw with_trace<std::out_of_range>("'i' must be less than 4 in oops()");
    if (i <= 0)
        throw with_trace<std::logic_error>("'i' must not be greater than zero in oops()");
    //]
    foo(i);
    std::exit(1);
}

#include <boost/array.hpp>
BOOST_NOINLINE void bar(int i) {
    boost::array<int, 5> a = {{0, 0, 0, 0, 0}};
    if (i < 5) {
        if (i >= 0) {
            foo(a[i]);
        } else {
            oops(i);
        }
    }
    std::exit(2);
}

BOOST_NOINLINE void foo(int i) {
    bar(--i);
}

#include <iostream>
int main() {

    //[getting_started_catching_trace
    try {
        foo(5); // testing assert handler
    } catch (const traced& e) {
        std::cerr << e.what() << '\n';
        if (e.trace) {
            std::cerr << "Backtrace:\n" << e.trace << '\n';
        } /*<-*/ std::exit(0); /*->*/
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; /*<-*/ std::exit(3); /*->*/
    }
    //]
    
    return 5;
}

#endif

