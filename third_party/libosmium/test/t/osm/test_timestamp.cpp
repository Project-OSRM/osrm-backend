#include "catch.hpp"

#include <osmium/osm/timestamp.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("Timestamp can be default initialized to invalid value") {
    const osmium::Timestamp t{};
    REQUIRE(0 == uint32_t(t));
    REQUIRE(t.to_iso().empty());
    REQUIRE(t.to_iso_all() == "1970-01-01T00:00:00Z");
    REQUIRE_FALSE(t.valid());
}

TEST_CASE("Timestamp invalid value is zero") {
    const osmium::Timestamp t{static_cast<time_t>(0)};
    REQUIRE(0 == uint32_t(t));
    REQUIRE(t.to_iso().empty());
    REQUIRE(t.to_iso_all() == "1970-01-01T00:00:00Z");
    REQUIRE_FALSE(t.valid());
}

TEST_CASE("Timestamp can be initialized from time_t") {
    const osmium::Timestamp t{static_cast<time_t>(1)};
    REQUIRE(1 == uint32_t(t));
    REQUIRE("1970-01-01T00:00:01Z" == t.to_iso());
    REQUIRE(t.valid());
}

TEST_CASE("Timestamp can be initialized from const char*") {
    const osmium::Timestamp t{"2000-01-01T00:00:00Z"};
    REQUIRE("2000-01-01T00:00:00Z" == t.to_iso());
    REQUIRE(t.valid());
}

TEST_CASE("Timestamp can be initialized from string") {
    const std::string s = "2000-01-01T00:00:00Z";
    const osmium::Timestamp t{s};
    REQUIRE("2000-01-01T00:00:00Z" == t.to_iso());
    REQUIRE(t.valid());
}

TEST_CASE("Timestamp throws if initialized from bad string") {
    REQUIRE_THROWS_AS(osmium::Timestamp("x"), const std::invalid_argument&);
}

TEST_CASE("Timestamp can be explicitly cast to time_t") {
    const osmium::Timestamp t{4242};
    const time_t x = t.seconds_since_epoch();
    REQUIRE(x == 4242);
}

TEST_CASE("Timestamp uint32_t can be initialized from Timestamp") {
    const osmium::Timestamp t{4242};
    const uint32_t x { t };

    REQUIRE(x == 4242);
}

TEST_CASE("Timestamps can be compared") {
    const osmium::Timestamp t1{10};
    const osmium::Timestamp t2{50};
    REQUIRE(t1 < t2);
    REQUIRE(t1 > osmium::start_of_time());
    REQUIRE(t2 > osmium::start_of_time());
    REQUIRE(t1 < osmium::end_of_time());
    REQUIRE(t2 < osmium::end_of_time());
}

TEST_CASE("Timestamp can be written to stream") {
    const osmium::Timestamp t{1};
    std::stringstream ss;
    ss << t;
    REQUIRE("1970-01-01T00:00:01Z" == ss.str());
}

void test_int2_to_string(int value, const char* ref) {
    std::string s;
    osmium::detail::add_2digit_int_to_string(value, s);
    REQUIRE(s == ref);
}

void test_int4_to_string(int value, const char* ref) {
    std::string s;
    osmium::detail::add_4digit_int_to_string(value, s);
    REQUIRE(s == ref);
}

TEST_CASE("Write two digit numbers") {
    test_int2_to_string( 0, "00");
    test_int2_to_string( 1, "01");
    test_int2_to_string( 2, "02");
    test_int2_to_string( 3, "03");
    test_int2_to_string( 4, "04");
    test_int2_to_string( 9, "09");
    test_int2_to_string(10, "10");
    test_int2_to_string(20, "20");
    test_int2_to_string(29, "29");
    test_int2_to_string(99, "99");
}

TEST_CASE("Write four digit numbers") {
    test_int4_to_string(1000, "1000");
    test_int4_to_string(1001, "1001");
    test_int4_to_string(2018, "2018");
    test_int4_to_string(9999, "9999");
}

TEST_CASE("Valid timestamps") {
    std::vector<std::string> test_cases = {
        "1970-01-01T00:00:01Z",
        "2000-01-01T00:00:00Z",
        "2006-12-31T23:59:59Z",
        "2030-12-31T23:59:59Z",
        "2016-02-28T23:59:59Z",
        "2016-03-31T23:59:59Z"
    };

    for (const auto& tc : test_cases) {
        const osmium::Timestamp t{tc};
        REQUIRE(tc == t.to_iso());
        REQUIRE(tc == t.to_iso_all());
    }
}

TEST_CASE("Invalid timestamps") {
    REQUIRE_THROWS_AS(osmium::Timestamp{""}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"x"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"xxxxxxxxxxxxxxxxxxxx"}, const std::invalid_argument&);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01x00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00:00x"}, const std::invalid_argument&);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000x01-01T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01x01T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00x00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00x00Z"}, const std::invalid_argument&);

    REQUIRE_THROWS_AS(osmium::Timestamp{"0000-00-01T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-00-01T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-00T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T24:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:60:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00:61Z"}, const std::invalid_argument&);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-32T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-02-30T00:00:00Z"}, const std::invalid_argument&);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-03-32T00:00:00Z"}, const std::invalid_argument&);
}

