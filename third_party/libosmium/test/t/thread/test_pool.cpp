#include "catch.hpp"

#include <osmium/thread/pool.hpp>

#include <stdexcept>

struct test_job_with_result {
    int operator()() const {
        return 42;
    }
};

struct test_job_throw {
    [[noreturn]] void operator()() const {
        throw std::runtime_error{"exception in pool thread"};
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
    REQUIRE(osmium::thread::detail::get_pool_size(-100, 0, 16) ==  1);
    REQUIRE(osmium::thread::detail::get_pool_size(1000, 0, 16) == 32);

}

TEST_CASE("if zero number of threads requested, threads configured") {
    osmium::thread::Pool pool{0};
    REQUIRE(pool.num_threads() > 0);
}

TEST_CASE("if any negative number of threads requested, threads configured") {
    osmium::thread::Pool pool{-1};
    REQUIRE(pool.num_threads() > 0);
}

TEST_CASE("if outlier negative number of threads requested, threads configured") {
    osmium::thread::Pool pool{-100};
    REQUIRE(pool.num_threads() > 0);
}

TEST_CASE("if outlier positive number of threads requested, threads configured") {
    osmium::thread::Pool pool{1000};
    REQUIRE(pool.num_threads() > 0);
}

TEST_CASE("can get access to default thread pool") {
    auto& pool = osmium::thread::Pool::default_instance();
    REQUIRE(pool.queue_empty());
}

TEST_CASE("can send job to default thread pool") {
    auto& pool = osmium::thread::Pool::default_instance();
    auto future = pool.submit(test_job_with_result{});
    REQUIRE(future.get() == 42);
}

TEST_CASE("can throw from job in default thread pool") {
    auto& pool = osmium::thread::Pool::default_instance();
    auto future = pool.submit(test_job_throw{});
    REQUIRE_THROWS_AS(future.get(), const std::runtime_error&);
}

TEST_CASE("can get access to user provided thread pool") {
    osmium::thread::Pool pool{7};
    REQUIRE(pool.queue_empty());
}

TEST_CASE("can access user-provided number of threads from pool") {
    osmium::thread::Pool pool{7};
    REQUIRE(pool.num_threads() == 7);
}

TEST_CASE("can send job to user provided thread pool") {
    osmium::thread::Pool pool{7};
    auto future = pool.submit(test_job_with_result{});
    REQUIRE(future.get() == 42);
}

TEST_CASE("can throw from job in user provided thread pool") {
    osmium::thread::Pool pool{7};
    auto future = pool.submit(test_job_throw{});
    REQUIRE_THROWS_AS(future.get(), const std::runtime_error&);
}

