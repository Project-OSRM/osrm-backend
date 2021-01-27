#include "catch.hpp"

#include <osmium/memory/buffer.hpp>

#include <array>
#include <stdexcept>

TEST_CASE("Buffer basics") {
    osmium::memory::Buffer invalid_buffer1;
    osmium::memory::Buffer invalid_buffer2;
    osmium::memory::Buffer empty_buffer1{1024};
    osmium::memory::Buffer empty_buffer2{2048};

    REQUIRE_FALSE(invalid_buffer1);
    REQUIRE_FALSE(invalid_buffer2);
    REQUIRE(empty_buffer1);
    REQUIRE(empty_buffer2);

    REQUIRE(invalid_buffer1 == invalid_buffer2);
    REQUIRE(invalid_buffer1 != empty_buffer1);
    REQUIRE(empty_buffer1   != empty_buffer2);

    REQUIRE(invalid_buffer1.capacity()  == 0);
    REQUIRE(invalid_buffer1.written()   == 0);
    REQUIRE(invalid_buffer1.committed() == 0);
    REQUIRE(invalid_buffer1.clear() == 0);

    REQUIRE(empty_buffer1.capacity()  == 1024);
    REQUIRE(empty_buffer1.written()   ==    0);
    REQUIRE(empty_buffer1.committed() ==    0);
    REQUIRE(empty_buffer1.is_aligned());
    REQUIRE(empty_buffer1.clear() == 0);

    REQUIRE(empty_buffer2.capacity()  == 2048);
    REQUIRE(empty_buffer2.written()   ==    0);
    REQUIRE(empty_buffer2.committed() ==    0);
    REQUIRE(empty_buffer2.is_aligned());
    REQUIRE(empty_buffer2.clear() == 0);
}

TEST_CASE("Buffer with zero size") {
    osmium::memory::Buffer buffer{0};
    REQUIRE(buffer.capacity() == 64);
}

TEST_CASE("Buffer with less than minimum size") {
    osmium::memory::Buffer buffer{63};
    REQUIRE(buffer.capacity() == 64);
}

TEST_CASE("Buffer with minimum size") {
    osmium::memory::Buffer buffer{64};
    REQUIRE(buffer.capacity() == 64);
}

TEST_CASE("Buffer with non-aligned size") {
    osmium::memory::Buffer buffer{65};
    REQUIRE(buffer.capacity() > 65);
}

TEST_CASE("Grow a buffer") {
    osmium::memory::Buffer buffer{128};
    REQUIRE(buffer.capacity() == 128);
    buffer.grow(256);
    REQUIRE(buffer.capacity() == 256);
    buffer.grow(257);
    REQUIRE(buffer.capacity() > 256);
    REQUIRE(buffer.committed() == 0);
    REQUIRE(buffer.written() == 0);
    REQUIRE(buffer.is_aligned());
}

TEST_CASE("Reserve space in a non-growing buffer") {
    osmium::memory::Buffer buffer{128, osmium::memory::Buffer::auto_grow::no};

    REQUIRE(buffer.reserve_space(20) != nullptr);
    REQUIRE(buffer.written() == 20);
    REQUIRE_THROWS_AS(buffer.reserve_space(1000), const osmium::buffer_is_full&);
}

TEST_CASE("Reserve space in a growing buffer") {
    osmium::memory::Buffer buffer{128, osmium::memory::Buffer::auto_grow::yes};

    REQUIRE(buffer.reserve_space(20) != nullptr);
    REQUIRE(buffer.written() == 20);
    REQUIRE(buffer.reserve_space(1000) != nullptr);
    REQUIRE(buffer.written() == 1020);
}

TEST_CASE("Create buffer from existing data with good alignment works") {
    std::array<unsigned char, 128> data = {{0}};

    osmium::memory::Buffer buffer{data.data(), data.size()};
    REQUIRE(buffer.capacity() == 128);
    REQUIRE(buffer.committed() == 128);
}

TEST_CASE("Create buffer from existing data with good alignment and committed value works") {
    std::array<unsigned char, 128> data = {{0}};

    osmium::memory::Buffer buffer{data.data(), data.size(), 32};
    REQUIRE(buffer.capacity() == 128);
    REQUIRE(buffer.committed() == 32);
    REQUIRE(buffer.written() == 32);
}

TEST_CASE("Create buffer from existing data with bad alignment fails") {
    std::array<unsigned char, 128> data = {{0}};

    const auto l1 = [&](){
        osmium::memory::Buffer buffer{data.data(), 127};
    };

    const auto l2 = [&](){
        osmium::memory::Buffer buffer{data.data(), 127, 120};
    };

    const auto l3 = [&](){
        osmium::memory::Buffer buffer{data.data(), 128, 127};
    };

    const auto l4 = [&](){
        osmium::memory::Buffer buffer{data.data(), 32, 128};
    };

    REQUIRE_THROWS_AS(l1(), const std::invalid_argument&);
    REQUIRE_THROWS_AS(l2(), const std::invalid_argument&);
    REQUIRE_THROWS_AS(l3(), const std::invalid_argument&);
    REQUIRE_THROWS_AS(l4(), const std::invalid_argument&);
}

