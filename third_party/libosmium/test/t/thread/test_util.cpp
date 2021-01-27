#include "catch.hpp"

#include <osmium/thread/util.hpp>

#include <future>
#include <stdexcept>
#include <type_traits>

TEST_CASE("check_for_exception") {
    std::promise<int> p;
    auto f = p.get_future();

    SECTION("not ready") {
        osmium::thread::check_for_exception(f);
    }
    SECTION("ready") {
        p.set_value(3);
        osmium::thread::check_for_exception(f);
    }
    SECTION("no shared state") {
        p.set_value(3);
        REQUIRE(f.get() == 3);
        osmium::thread::check_for_exception(f);
    }
}

TEST_CASE("check_for_exception with exception") {
    std::promise<int> p;
    auto f = p.get_future();

    try {
        throw std::runtime_error{"TEST"};
    } catch (...) {
        p.set_exception(std::current_exception());
    }

    REQUIRE_THROWS_AS(osmium::thread::check_for_exception(f), const std::runtime_error&);
}

static_assert(std::is_nothrow_move_constructible<osmium::thread::thread_handler>::value, "thread_handler must have noexcept move constructor");

TEST_CASE("empty thread_handler") {
    osmium::thread::thread_handler th;
}

int foo;

void test_func(int value) {
    foo = value;
}

TEST_CASE("valid thread_handler") {
    foo = 22;
    test_func(17);
    REQUIRE(foo == 17);
    {
        osmium::thread::thread_handler th{test_func, 5};
    }
    REQUIRE(foo == 5);
}

