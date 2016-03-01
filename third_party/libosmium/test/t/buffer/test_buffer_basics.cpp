#include "catch.hpp"

#include <osmium/memory/buffer.hpp>

TEST_CASE("Buffer basics") {

    osmium::memory::Buffer invalid_buffer1;
    osmium::memory::Buffer invalid_buffer2;
    osmium::memory::Buffer empty_buffer1(1024);
    osmium::memory::Buffer empty_buffer2(2048);

    REQUIRE(!invalid_buffer1);
    REQUIRE(!invalid_buffer2);
    REQUIRE(empty_buffer1);
    REQUIRE(empty_buffer2);

    REQUIRE(invalid_buffer1 == invalid_buffer2);
    REQUIRE(invalid_buffer1 != empty_buffer1);
    REQUIRE(empty_buffer1   != empty_buffer2);

    REQUIRE(invalid_buffer1.capacity()  == 0);
    REQUIRE(invalid_buffer1.written()   == 0);
    REQUIRE(invalid_buffer1.committed() == 0);

    REQUIRE(empty_buffer1.capacity()  == 1024);
    REQUIRE(empty_buffer1.written()   ==    0);
    REQUIRE(empty_buffer1.committed() ==    0);

    REQUIRE(empty_buffer2.capacity()  == 2048);
    REQUIRE(empty_buffer2.written()   ==    0);
    REQUIRE(empty_buffer2.committed() ==    0);

}

