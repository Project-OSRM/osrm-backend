#include "catch.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

#include <osmium/thread/pool.hpp>
#include <osmium/util/compatibility.hpp>

struct test_job_with_result {
    int operator()() const {
        return 42;
    }
};

struct test_job_throw {
    OSMIUM_NORETURN void operator()() const {
        throw std::runtime_error("exception in pool thread");
    }
};

TEST_CASE("number of threads in pool") {

    // hardcoded setting
    REQUIRE(osmium::thread::detail::get_pool_size( 1,  0,  2) ==  1);
    REQUIRE(osmium::thread::detail::get_pool_size( 4,  0,  2) ==  4);
    REQUIRE(osmium::thread::detail::get_pool_size( 4,  0,  4) ==  4);
    REQUIRE(osmium::thread::detail::get_pool_size(16,  0,  4) == 16);
    REQUIRE(osmium::thread::detail::get_pool_size(16,  0, 16) == 16);
    REQUIRE(osmium::thread::detail::get_pool_size( 8,  4,  2) ==  8);
    REQUIRE(osmium::thread::detail::get_pool_size( 8, 16,  2) ==  8);
    REQUIRE(osmium::thread::detail::get_pool_size(-2, 16,  2) ==  1);
    REQUIRE(osmium::thread::detail::get_pool_size(-2, 16,  8) ==  6);

    // user decides through OSMIUM_POOL_THREADS env variable
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  0,  2) ==  1);
    REQUIRE(osmium::thread::detail::get_pool_size( 0, -2,  4) ==  2);
    REQUIRE(osmium::thread::detail::get_pool_size( 0, -1,  8) ==  7);
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  0, 16) == 14);
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  1, 16) ==  1);
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  2, 16) ==  2);
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  4, 16) ==  4);
    REQUIRE(osmium::thread::detail::get_pool_size( 0,  8, 16) ==  8);

    // outliers
    REQUIRE(osmium::thread::detail::get_pool_size(-100, 0, 16) ==   1);
    REQUIRE(osmium::thread::detail::get_pool_size(1000, 0, 16) == 256);

}

TEST_CASE("thread") {

    auto& pool = osmium::thread::Pool::instance();

    SECTION("can get access to thread pool") {
        REQUIRE(pool.queue_empty());
    }

    SECTION("can send job to thread pool") {
        auto future = pool.submit(test_job_with_result {});

        REQUIRE(future.get() == 42);
    }

    SECTION("can throw from job in thread pool") {
        auto future = pool.submit(test_job_throw {});

        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }

}

