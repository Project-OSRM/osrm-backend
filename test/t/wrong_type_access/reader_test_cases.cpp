
#include <test.hpp>

// protobuf wire type 0
TEST_CASE("check assert on non-varint access to varint") {
    const std::string buffer = load_data("int32/data-zero");

    protozero::pbf_reader item{buffer};
    REQUIRE(item.next());

    REQUIRE(item.get_int32() == 0);
    REQUIRE_THROWS_AS(item.get_fixed64(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_string(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_fixed32(), const assert_error&);
}

// protobuf wire type 1
TEST_CASE("check assert on non-fixed access to fixed64") {
    const std::string buffer = load_data("fixed64/data-zero");

    protozero::pbf_reader item{buffer};
    REQUIRE(item.next());

    REQUIRE_THROWS_AS(item.get_int32(), const assert_error&);
    REQUIRE(item.get_fixed64() == 0);
    REQUIRE_THROWS_AS(item.get_string(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_fixed32(), const assert_error&);
}

// protobuf wire type 2
TEST_CASE("check assert on non-string access to string") {
    const std::string buffer = load_data("string/data-string");

    protozero::pbf_reader item{buffer};
    REQUIRE(item.next());

    REQUIRE_THROWS_AS(item.get_int32(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_fixed64(), const assert_error&);
    REQUIRE(item.get_string() == "foobar");
    REQUIRE_THROWS_AS(item.get_fixed32(), const assert_error&);
}

// protobuf wire type 5
TEST_CASE("check assert on non-fixed access to fixed32") {
    const std::string buffer = load_data("fixed32/data-zero");

    protozero::pbf_reader item{buffer};
    REQUIRE(item.next());

    REQUIRE_THROWS_AS(item.get_int32(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_fixed64(), const assert_error&);
    REQUIRE_THROWS_AS(item.get_string(), const assert_error&);
    REQUIRE(item.get_fixed32() == 0);
}

