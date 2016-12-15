// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <boost/stacktrace.hpp>
#include <boost/core/lightweight_test.hpp>

using boost::stacktrace::stacktrace;
BOOST_SYMBOL_IMPORT std::pair<stacktrace, stacktrace> foo2(int i);
BOOST_SYMBOL_IMPORT std::pair<stacktrace, stacktrace> foo1(int i);
BOOST_SYMBOL_IMPORT stacktrace return_from_nested_namespaces();

void test_deeply_nested_namespaces() {
    BOOST_TEST(return_from_nested_namespaces().size() == 0);
    BOOST_TEST(return_from_nested_namespaces().begin()->name() == "");
}

void test_nested() {
    std::pair<stacktrace, stacktrace> res = foo2(15);

    BOOST_TEST(!res.first);
    BOOST_TEST(res.first.size() == 0);
    BOOST_TEST(res.first.begin()->name() == "");
    BOOST_TEST(res.first.begin()->source_file() == "");
    BOOST_TEST(res.first.begin()->source_line() == 0);
    BOOST_TEST(!res.second);
    BOOST_TEST(res.second.size() == 0);
    BOOST_TEST(res.second.begin()->name() == "");
    BOOST_TEST(res.second.begin()->source_file() == "");
    BOOST_TEST(res.second.begin()->source_line() == 0);

    BOOST_TEST(res.second[0].name() == "");
    BOOST_TEST(res.second[0].source_file() == "");
    BOOST_TEST(res.second[0].source_line() == 0);
    BOOST_TEST(res.second[0].address() == 0);

    BOOST_TEST(res.second <= res.first);
    BOOST_TEST(res.second >= res.first);
    BOOST_TEST(res.second == res.first);
    BOOST_TEST(res.second == res.first);
    BOOST_TEST(!(res.second > res.first));
}

void test_empty_frame() {
    boost::stacktrace::frame empty_frame;
    BOOST_TEST(!empty_frame);
    BOOST_TEST(empty_frame.source_file() == "");
    BOOST_TEST(empty_frame.name() == "");
    BOOST_TEST(empty_frame.source_line() == 0);
}

int main() {
    test_deeply_nested_namespaces();
    test_nested();
    test_empty_frame();

    return boost::report_errors();
}
