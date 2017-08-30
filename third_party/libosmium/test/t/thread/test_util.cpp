#include <catch.hpp>

#include <stdexcept>

#include <osmium/thread/util.hpp>

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
    } catch(...) {
        p.set_exception(std::current_exception());
    }

    REQUIRE_THROWS_AS(osmium::thread::check_for_exception(f), const std::runtime_error&);
}

