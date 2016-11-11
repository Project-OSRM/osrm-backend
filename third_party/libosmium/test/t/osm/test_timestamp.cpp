#include "catch.hpp"

#include <sstream>

#include <osmium/osm/timestamp.hpp>

TEST_CASE("Timestamp") {

    SECTION("can be default initialized to invalid value") {
        osmium::Timestamp t;
        REQUIRE(0 == uint32_t(t));
        REQUIRE("" == t.to_iso());
        REQUIRE_FALSE(t.valid());
    }

    SECTION("invalid value is zero") {
        osmium::Timestamp t(static_cast<time_t>(0));
        REQUIRE(0 == uint32_t(t));
        REQUIRE("" == t.to_iso());
        REQUIRE_FALSE(t.valid());
    }

    SECTION("can be initialized from time_t") {
        osmium::Timestamp t(static_cast<time_t>(1));
        REQUIRE(1 == uint32_t(t));
        REQUIRE("1970-01-01T00:00:01Z" == t.to_iso());
        REQUIRE(t.valid());
    }

    SECTION("can be initialized from const char*") {
        osmium::Timestamp t("2000-01-01T00:00:00Z");
        REQUIRE("2000-01-01T00:00:00Z" == t.to_iso());
        REQUIRE(t.valid());
    }

    SECTION("can be initialized from string") {
        std::string s = "2000-01-01T00:00:00Z";
        osmium::Timestamp t(s);
        REQUIRE("2000-01-01T00:00:00Z" == t.to_iso());
        REQUIRE(t.valid());
    }

    SECTION("throws if initialized from bad string") {
        REQUIRE_THROWS_AS(osmium::Timestamp("x"), std::invalid_argument);
    }

    SECTION("can be explicitly cast to time_t") {
        osmium::Timestamp t(4242);
        time_t x = t.seconds_since_epoch();
        REQUIRE(x == 4242);
    }

    SECTION("uint32_t can be initialized from Timestamp") {
        osmium::Timestamp t(4242);
        uint32_t x { t };

        REQUIRE(x == 4242);
    }

    SECTION("can be compared") {
        osmium::Timestamp t1(10);
        osmium::Timestamp t2(50);
        REQUIRE(t1 < t2);
        REQUIRE(t1 > osmium::start_of_time());
        REQUIRE(t2 > osmium::start_of_time());
        REQUIRE(t1 < osmium::end_of_time());
        REQUIRE(t2 < osmium::end_of_time());
    }

    SECTION("can be written to stream") {
        std::stringstream ss;
        osmium::Timestamp t(1);
        ss << t;
        REQUIRE("1970-01-01T00:00:01Z" == ss.str());
    }

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
        osmium::Timestamp t{tc};
        REQUIRE(tc == t.to_iso());
    }

}

TEST_CASE("Invalid timestamps") {
    REQUIRE_THROWS_AS(osmium::Timestamp{""}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"x"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"xxxxxxxxxxxxxxxxxxxx"}, std::invalid_argument);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01x00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00:00x"}, std::invalid_argument);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000x01-01T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01x01T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00x00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00x00Z"}, std::invalid_argument);

    REQUIRE_THROWS_AS(osmium::Timestamp{"0000-00-01T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-00-01T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-00T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T24:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:60:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-01T00:00:61Z"}, std::invalid_argument);

    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-01-32T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-02-30T00:00:00Z"}, std::invalid_argument);
    REQUIRE_THROWS_AS(osmium::Timestamp{"2000-03-32T00:00:00Z"}, std::invalid_argument);
}

