
#include <test.hpp>

#include <vtzero/types.hpp>

#include <string>

TEST_CASE("default constructed string_value_type") {
    vtzero::string_value_type v;
    REQUIRE(v.value.data() == nullptr);
}

TEST_CASE("string_value_type with value") {
    vtzero::string_value_type v{"foo"};
    REQUIRE(v.value.data()[0] == 'f');
    REQUIRE(v.value.size() == 3);
}

TEST_CASE("default constructed float_value_type") {
    vtzero::float_value_type v;
    REQUIRE(v.value == Approx(0.0));
}

TEST_CASE("float_value_type with value") {
    float x = 2.7f;
    vtzero::float_value_type v{x};
    REQUIRE(v.value == Approx(x));
}

TEST_CASE("default constructed double_value_type") {
    vtzero::double_value_type v;
    REQUIRE(v.value == Approx(0.0));
}

TEST_CASE("double_value_type with value") {
    double x = 2.7;
    vtzero::double_value_type v{x};
    REQUIRE(v.value == Approx(x));
}

TEST_CASE("default constructed int_value_type") {
    vtzero::int_value_type v;
    REQUIRE(v.value == 0);
}

TEST_CASE("int_value_type with value") {
    vtzero::int_value_type v{123};
    REQUIRE(v.value == 123);
}

TEST_CASE("default constructed uint_value_type") {
    vtzero::uint_value_type v;
    REQUIRE(v.value == 0);
}

TEST_CASE("uint_value_type with value") {
    vtzero::uint_value_type v{123};
    REQUIRE(v.value == 123);
}

TEST_CASE("default constructed sint_value_type") {
    vtzero::sint_value_type v;
    REQUIRE(v.value == 0);
}

TEST_CASE("sint_value_type with value") {
    vtzero::sint_value_type v{-14};
    REQUIRE(v.value == -14);
}

TEST_CASE("default constructed bool_value_type") {
    vtzero::bool_value_type v;
    REQUIRE_FALSE(v.value);
}

TEST_CASE("bool_value_type with value") {
    bool x = true;
    vtzero::bool_value_type v{x};
    REQUIRE(v.value);
}

TEST_CASE("property_value_type names") {
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::string_value)} == "string");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::float_value)} == "float");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::double_value)} == "double");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::int_value)} == "int");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::uint_value)} == "uint");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::sint_value)} == "sint");
    REQUIRE(std::string{vtzero::property_value_type_name(vtzero::property_value_type::bool_value)} == "bool");
}

TEST_CASE("default constructed index value") {
    vtzero::index_value v;
    REQUIRE_FALSE(v.valid());
    REQUIRE_ASSERT(v.value());
}

TEST_CASE("valid index value") {
    vtzero::index_value v{32};
    REQUIRE(v.valid());
    REQUIRE(v.value() == 32);
}

TEST_CASE("default constructed geometry") {
    vtzero::geometry geom;
    REQUIRE(geom.type() == vtzero::GeomType::UNKNOWN);
    REQUIRE(geom.data().empty());
}

TEST_CASE("GeomType names") {
    REQUIRE(std::string{vtzero::geom_type_name(vtzero::GeomType::UNKNOWN)} == "unknown");
    REQUIRE(std::string{vtzero::geom_type_name(vtzero::GeomType::POINT)} == "point");
    REQUIRE(std::string{vtzero::geom_type_name(vtzero::GeomType::LINESTRING)} == "linestring");
    REQUIRE(std::string{vtzero::geom_type_name(vtzero::GeomType::POLYGON)} == "polygon");
}

