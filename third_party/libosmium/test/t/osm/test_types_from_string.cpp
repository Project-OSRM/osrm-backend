#include "catch.hpp"

#include <osmium/osm/types.hpp>
#include <osmium/osm/types_from_string.hpp>

#include <stdexcept>

TEST_CASE("set ID from string") {
    REQUIRE(osmium::string_to_object_id("0") == 0);
    REQUIRE(osmium::string_to_object_id("17") == 17);
    REQUIRE(osmium::string_to_object_id("-17") == -17);
    REQUIRE(osmium::string_to_object_id("01") == 1);

    REQUIRE_THROWS_AS(osmium::string_to_object_id(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("0x1"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("12a"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("12345678901234567890"), const std::range_error&);
}

TEST_CASE("set type and ID from string") {
    const auto n17 = osmium::string_to_object_id("n17", osmium::osm_entity_bits::nwr);
    REQUIRE(n17.first == osmium::item_type::node);
    REQUIRE(n17.second == 17);

    const auto w42 = osmium::string_to_object_id("w42", osmium::osm_entity_bits::nwr);
    REQUIRE(w42.first == osmium::item_type::way);
    REQUIRE(w42.second == 42);

    const auto r_2 = osmium::string_to_object_id("r-2", osmium::osm_entity_bits::nwr);
    REQUIRE(r_2.first == osmium::item_type::relation);
    REQUIRE(r_2.second == -2);

    const auto d3 = osmium::string_to_object_id("3", osmium::osm_entity_bits::nwr);
    REQUIRE(d3.first == osmium::item_type::undefined);
    REQUIRE(d3.second == 3);

    const auto u3 = osmium::string_to_object_id("3", osmium::osm_entity_bits::nwr, osmium::item_type::undefined);
    REQUIRE(u3.first == osmium::item_type::undefined);
    REQUIRE(u3.second == 3);

    const auto n3 = osmium::string_to_object_id("3", osmium::osm_entity_bits::nwr, osmium::item_type::node);
    REQUIRE(n3.first == osmium::item_type::node);
    REQUIRE(n3.second == 3);

    REQUIRE_THROWS_AS(osmium::string_to_object_id("", osmium::osm_entity_bits::nwr), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("n", osmium::osm_entity_bits::nwr), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("x3", osmium::osm_entity_bits::nwr), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("nx3", osmium::osm_entity_bits::nwr), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("n3", osmium::osm_entity_bits::way), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_id("n3a", osmium::osm_entity_bits::nwr), const std::range_error&);
}

TEST_CASE("set object version from string") {
    REQUIRE(osmium::string_to_object_version("0") == 0);
    REQUIRE(osmium::string_to_object_version("1") == 1);
    REQUIRE(osmium::string_to_object_version("-1") == 0);
    REQUIRE(osmium::string_to_object_version("4294967294") == 4294967294);

    REQUIRE_THROWS_AS(osmium::string_to_object_version("-2"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version("4294967295"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_object_version("999999999999"), const std::range_error&);
}

TEST_CASE("set changeset id from string") {
    REQUIRE(osmium::string_to_changeset_id("0") == 0);
    REQUIRE(osmium::string_to_changeset_id("1") == 1);
    REQUIRE(osmium::string_to_changeset_id("-1") == 0);
    REQUIRE(osmium::string_to_changeset_id("4294967294") == 4294967294);

    REQUIRE_THROWS_AS(osmium::string_to_changeset_id("-2"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id("4294967295"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_changeset_id("999999999999"), const std::range_error&);
}

TEST_CASE("set user id from string") {
    REQUIRE(osmium::string_to_user_id("0") == 0);
    REQUIRE(osmium::string_to_user_id("1") == 1);
    REQUIRE(osmium::string_to_user_id("-1") == -1);
    REQUIRE(osmium::string_to_user_id("2147483647") == 2147483647);

    REQUIRE_THROWS_AS(osmium::string_to_user_id("-2"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id("2147483648"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id("4294967295"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_user_id("999999999999"), const std::range_error&);
}

TEST_CASE("set uid from string") {
    REQUIRE(osmium::string_to_uid("0") == 0);
    REQUIRE(osmium::string_to_uid("1") == 1);
    REQUIRE(osmium::string_to_uid("-1") == 0);
    REQUIRE(osmium::string_to_uid("4294967294") == 4294967294);

    REQUIRE_THROWS_AS(osmium::string_to_uid("-2"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid("4294967295"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_uid("999999999999"), const std::range_error&);
}

TEST_CASE("set num changes from string") {
    REQUIRE(osmium::string_to_num_changes("0") == 0);
    REQUIRE(osmium::string_to_num_changes("1") == 1);
    REQUIRE(osmium::string_to_num_changes("-1") == 0);
    REQUIRE(osmium::string_to_num_changes("4294967294") == 4294967294);

    REQUIRE_THROWS_AS(osmium::string_to_num_changes("-2"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes(""), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes(" "), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes(" 22"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes("x"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes("4294967295"), const std::range_error&);
    REQUIRE_THROWS_AS(osmium::string_to_num_changes("999999999999"), const std::range_error&);
}

