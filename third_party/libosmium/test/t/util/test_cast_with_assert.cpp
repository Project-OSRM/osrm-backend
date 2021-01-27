#include "catch.hpp"

#include <stdexcept>

// Define assert() to throw this error. This enables the tests to check that
// the assert() fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};

#ifdef assert
#undef assert
#endif

#define assert(x) if (!(x)) { throw assert_error{#x}; }

#include <osmium/util/cast.hpp>

TEST_CASE("static_cast_with_assert: same types is always okay") {
    const int f = 10;
    const auto t = osmium::static_cast_with_assert<int>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: casting to larger type is always okay") {
    const int16_t f = 10;
    const auto t = osmium::static_cast_with_assert<int32_t>(f);
    REQUIRE(t == f);
}


TEST_CASE("static_cast_with_assert: cast int32_t -> int16_t should not trigger assert for small int") {
    const int32_t f = 100;
    const auto t = osmium::static_cast_with_assert<int16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast int32_t -> int16_t should trigger assert for large int") {
    const int32_t f = 100000;
    REQUIRE_THROWS_AS(osmium::static_cast_with_assert<int16_t>(f), const assert_error&);
}


TEST_CASE("static_cast_with_assert: cast int16_t -> uint16_t should not trigger assert for zero") {
    const int16_t f = 0;
    const auto t = osmium::static_cast_with_assert<uint16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast int16_t -> uint16_t should not trigger assert for positive int") {
    const int16_t f = 1;
    const auto t = osmium::static_cast_with_assert<uint16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast int16_t -> uint16_t should trigger assert for negative int") {
    const int16_t f = -1;
    REQUIRE_THROWS_AS(osmium::static_cast_with_assert<uint16_t>(f), const assert_error&);
}


TEST_CASE("static_cast_with_assert: cast uint32_t -> uint16_t should not trigger assert for zero") {
    const uint32_t f = 0;
    const auto t = osmium::static_cast_with_assert<uint16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast uint32_t -> uint16_t should not trigger assert for small int") {
    const uint32_t f = 100;
    const auto t = osmium::static_cast_with_assert<uint16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast uint32_t -> uint16_t should trigger assert for large int") {
    const uint32_t f = 100000;
    REQUIRE_THROWS_AS(osmium::static_cast_with_assert<uint16_t>(f), const assert_error&);
}


TEST_CASE("static_cast_with_assert: cast uint16_t -> int16_t should not trigger assert for small int") {
    const uint16_t f = 1;
    const auto t = osmium::static_cast_with_assert<int16_t>(f);
    REQUIRE(t == f);
}

TEST_CASE("static_cast_with_assert: cast uint16_t -> int16_t should trigger assert for large int") {
    const uint16_t f = 65000;
    REQUIRE_THROWS_AS(osmium::static_cast_with_assert<int16_t>(f), const assert_error&);
}

