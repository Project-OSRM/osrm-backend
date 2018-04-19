#include "catch.hpp"

#include <osmium/memory/item.hpp>

TEST_CASE("padded length") {
    REQUIRE(osmium::memory::padded_length(0) ==  0);
    REQUIRE(osmium::memory::padded_length(1) ==  8);
    REQUIRE(osmium::memory::padded_length(2) ==  8);
    REQUIRE(osmium::memory::padded_length(7) ==  8);
    REQUIRE(osmium::memory::padded_length(8) ==  8);
    REQUIRE(osmium::memory::padded_length(9) == 16);

    REQUIRE(osmium::memory::padded_length(2147483647ul) == 2147483648ul);
    REQUIRE(osmium::memory::padded_length(2147483648ul) == 2147483648ul);
    REQUIRE(osmium::memory::padded_length(2147483650ul) == 2147483656ul);

    // The following checks only make sense on a 64 bit system (with
    // sizeof(size_t) == 8), because the numbers are too large for 32 bit.
    // The casts to size_t do nothing on a 64 bit system, on a 32 bit system
    // they bring the numbers into the right range and everything still works.
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967295ull)) == static_cast<std::size_t>(4294967296ull));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967296ull)) == static_cast<std::size_t>(4294967296ull));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967297ull)) == static_cast<std::size_t>(4294967304ull));

    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(7999999999ull)) == static_cast<std::size_t>(8000000000ull));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(8000000000ull)) == static_cast<std::size_t>(8000000000ull));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(8000000001ull)) == static_cast<std::size_t>(8000000008ull));
}

