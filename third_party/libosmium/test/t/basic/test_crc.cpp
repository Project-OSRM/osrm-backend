#include "catch.hpp"

#include <boost/crc.hpp>

#include <osmium/osm/crc.hpp>

#include "helper.hpp"

TEST_CASE("CRC of basic datatypes") {

    osmium::CRC<boost::crc_32_type> crc32;

    SECTION("Bool") {
        crc32.update_bool(true);
        crc32.update_bool(false);

        REQUIRE(crc32().checksum() == 0x58c223be);
    }

    SECTION("Char") {
        crc32.update_int8('x');
        crc32.update_int8('y');

        REQUIRE(crc32().checksum() == 0x8fe62899);
    }

    SECTION("String") {
        const char* str = "foobar";
        crc32.update_string(str);

        REQUIRE(crc32().checksum() == 0x9ef61f95);
    }

    SECTION("Timestamp") {
        osmium::Timestamp t("2015-07-12T13:10:46Z");
        crc32.update(t);

        REQUIRE(crc32().checksum() == 0x58a29d7);
    }

    SECTION("Location") {
        osmium::Location loc { 3.46, 2.001 };
        crc32.update(loc);

        REQUIRE(crc32().checksum() == 0xddee042c);
    }

}

