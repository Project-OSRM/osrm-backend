
#include <test.hpp>

#include <sstream>
#include <string>

template <typename T>
std::string get_output(T v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

TEST_CASE("output GeomType") {
    REQUIRE(get_output(vtzero::GeomType::UNKNOWN)    == "unknown");
    REQUIRE(get_output(vtzero::GeomType::POINT)      == "point");
    REQUIRE(get_output(vtzero::GeomType::LINESTRING) == "linestring");
    REQUIRE(get_output(vtzero::GeomType::POLYGON)    == "polygon");
}

TEST_CASE("output property_value_type") {
    REQUIRE(get_output(vtzero::property_value_type::sint_value) == "sint");
}

TEST_CASE("output index_value") {
    REQUIRE(get_output(vtzero::index_value{}) == "invalid");
    REQUIRE(get_output(vtzero::index_value{5}) == "5");
}

TEST_CASE("output index_value_pair") {
    const auto in = vtzero::index_value{};
    const auto v2 = vtzero::index_value{2};
    const auto v5 = vtzero::index_value{5};
    REQUIRE(get_output(vtzero::index_value_pair{in, v2}) == "invalid");
    REQUIRE(get_output(vtzero::index_value_pair{v2, v5}) == "[2,5]");
}

TEST_CASE("output point") {
    REQUIRE(get_output(vtzero::point{}) == "(0,0)");
    REQUIRE(get_output(vtzero::point{4, 7}) == "(4,7)");
}

