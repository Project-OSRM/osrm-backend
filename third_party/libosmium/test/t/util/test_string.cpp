#include "catch.hpp"

#include <osmium/util/string.hpp>

#include <string>
#include <vector>

TEST_CASE("split_string string") {
    const std::string str{"foo,baramba,baz"};
    const std::vector<std::string> result = {"foo", "baramba", "baz"};

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(result == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ",;"));
    REQUIRE(result == osmium::split_string(str, ",;", true));
}

TEST_CASE("split_string string without sep") {
    const std::string str{"foo"};
    const std::vector<std::string> result = {"foo"};

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(result == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ",;"));
    REQUIRE(result == osmium::split_string(str, ",;", true));
}

TEST_CASE("split_string string with empty at end") {
    const std::string str{"foo,bar,"};
    const std::vector<std::string> result = {"foo", "bar", ""};
    const std::vector<std::string> resultc = {"foo", "bar"};

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(resultc == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ";,"));
    REQUIRE(resultc == osmium::split_string(str, ";,", true));
}

TEST_CASE("split_string string with empty in middle") {
    const std::string str{"foo,,bar"};
    const std::vector<std::string> result = {"foo", "", "bar"};
    const std::vector<std::string> resultc = {"foo", "bar"};

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(resultc == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ",;"));
    REQUIRE(resultc == osmium::split_string(str, ";,", true));
}

TEST_CASE("split_string string with empty at start") {
    const std::string str{",bar,baz"};
    const std::vector<std::string> result = {"", "bar", "baz"};
    const std::vector<std::string> resultc = {"bar", "baz"};

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(resultc == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ";,"));
    REQUIRE(resultc == osmium::split_string(str, ",;", true));
}

TEST_CASE("split_string sep") {
    const std::string str{","};
    const std::vector<std::string> result = {"", ""};
    const std::vector<std::string> resultc;

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(resultc == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ",;"));
    REQUIRE(resultc == osmium::split_string(str, ",;", true));
}

TEST_CASE("split_string empty string") {
    const std::string str{};
    const std::vector<std::string> result;

    REQUIRE(result == osmium::split_string(str, ','));
    REQUIRE(result == osmium::split_string(str, ',', true));
    REQUIRE(result == osmium::split_string(str, ",;"));
    REQUIRE(result == osmium::split_string(str, ",;", true));
}

TEST_CASE("split_string string with multiple sep characters") {
    const std::string str{"foo,bar;baz-,blub"};
    const std::vector<std::string> result = {"foo", "bar", "baz", "", "blub"};

    REQUIRE(result == osmium::split_string(str, ";,-"));
}

