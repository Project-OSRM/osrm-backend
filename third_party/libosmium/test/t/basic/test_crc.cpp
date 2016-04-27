#include "catch.hpp"

#include <boost/crc.hpp>

#include <osmium/osm/crc.hpp>

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

    SECTION("Int16") {
        crc32.update_int16(0x0123U);
        crc32.update_int16(0x1234U);

        REQUIRE(crc32().checksum() == 0xda923744);
    }

    SECTION("Int32") {
        crc32.update_int32(0x01234567UL);
        crc32.update_int32(0x12345678UL);

        REQUIRE(crc32().checksum() == 0x9b4e2af3);
    }

    SECTION("Int64") {
        crc32.update_int64(0x0123456789abcdefULL);
        crc32.update_int64(0x123456789abcdef0ULL);

        REQUIRE(crc32().checksum() == 0x6d8b7267);
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

