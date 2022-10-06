#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/osm/crc.hpp>

TEST_CASE("CRC of bool") {
    osmium::CRC<crc_type> crc32;

    crc32.update_bool(true);
    crc32.update_bool(false);

    REQUIRE(crc32().checksum() == 0x58c223be);
}

TEST_CASE("CRC of char") {
    osmium::CRC<crc_type> crc32;

    crc32.update_int8('x');
    crc32.update_int8('y');

    REQUIRE(crc32().checksum() == 0x8fe62899);
}

TEST_CASE("CRC of int16") {
    osmium::CRC<crc_type> crc32;

    crc32.update_int16(0x0123U);
    crc32.update_int16(0x1234U);

    REQUIRE(crc32().checksum() == 0xda923744);
}

TEST_CASE("CRC of int32") {
    osmium::CRC<crc_type> crc32;

    crc32.update_int32(0x01234567UL);
    crc32.update_int32(0x12345678UL);

    REQUIRE(crc32().checksum() == 0x9b4e2af3);
}

TEST_CASE("CRC of int64") {
    osmium::CRC<crc_type> crc32;

    crc32.update_int64(0x0123456789abcdefULL);
    crc32.update_int64(0x123456789abcdef0ULL);

    REQUIRE(crc32().checksum() == 0x6d8b7267);
}

TEST_CASE("CRC of string") {
    osmium::CRC<crc_type> crc32;

    const char* str = "foobar";
    crc32.update_string(str);

    REQUIRE(crc32().checksum() == 0x9ef61f95);
}

TEST_CASE("CRC of Timestamp") {
    osmium::CRC<crc_type> crc32;

    const osmium::Timestamp t{"2015-07-12T13:10:46Z"};
    crc32.update(t);

    REQUIRE(crc32().checksum() == 0x58a29d7);
}

TEST_CASE("CRC of Location") {
    osmium::CRC<crc_type> crc32;

    const osmium::Location loc{3.46, 2.001};
    crc32.update(loc);

    REQUIRE(crc32().checksum() == 0xddee042c);
}

