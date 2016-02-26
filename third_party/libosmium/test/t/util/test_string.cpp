#include "catch.hpp"

#include <osmium/util/string.hpp>

TEST_CASE("split_string") {

    SECTION("split_string string") {
        std::string str { "foo,baramba,baz" };
        std::vector<std::string> result = {"foo", "baramba", "baz"};

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(result == osmium::split_string(str, ',', true));
    }

    SECTION("split_string string without sep") {
        std::string str { "foo" };
        std::vector<std::string> result = {"foo"};

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(result == osmium::split_string(str, ',', true));
    }

    SECTION("split_string string with empty at end") {
        std::string str { "foo,bar," };
        std::vector<std::string> result = {"foo", "bar", ""};
        std::vector<std::string> resultc = {"foo", "bar"};

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(resultc == osmium::split_string(str, ',', true));
    }

    SECTION("split_string string with empty in middle") {
        std::string str { "foo,,bar" };
        std::vector<std::string> result = {"foo", "", "bar"};
        std::vector<std::string> resultc = {"foo", "bar"};

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(resultc == osmium::split_string(str, ',', true));
    }

    SECTION("split_string string with empty at start") {
        std::string str { ",bar,baz" };
        std::vector<std::string> result = {"", "bar", "baz"};
        std::vector<std::string> resultc = {"bar", "baz"};

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(resultc == osmium::split_string(str, ',', true));
    }

    SECTION("split_string sep") {
        std::string str { "," };
        std::vector<std::string> result = {"", ""};
        std::vector<std::string> resultc;

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(resultc == osmium::split_string(str, ',', true));
    }

    SECTION("split_string empty string") {
        std::string str { "" };
        std::vector<std::string> result;

        REQUIRE(result == osmium::split_string(str, ','));
        REQUIRE(result == osmium::split_string(str, ',', true));
    }

}

