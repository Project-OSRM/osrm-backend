
#include <test.hpp>

#include <vtzero/exception.hpp>

#include <string>

TEST_CASE("construct format_exception with const char*") {
    vtzero::format_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct format_exception with const std::string") {
    vtzero::format_exception e{std::string{"broken"}};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct geometry_exception with const char*") {
    vtzero::geometry_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct geometry_exception with std::string") {
    vtzero::geometry_exception e{std::string{"broken"}};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct type_exception") {
    vtzero::type_exception e;
    REQUIRE(std::string{e.what()} == "wrong property value type");
}

TEST_CASE("construct version_exception") {
    vtzero::version_exception e{42};
    REQUIRE(std::string{e.what()} == "unknown vector tile version: 42");
}

TEST_CASE("construct out_of_range_exception") {
    vtzero::out_of_range_exception e{99};
    REQUIRE(std::string{e.what()} == "index out of range: 99");
}

