#include "catch.hpp"

#include <osmium/memory/item.hpp>

TEST_CASE("padded length") {
    REQUIRE(osmium::memory::padded_length(0) ==  0);
    REQUIRE(osmium::memory::padded_length(1) ==  8);
    REQUIRE(osmium::memory::padded_length(2) ==  8);
    REQUIRE(osmium::memory::padded_length(7) ==  8);
    REQUIRE(osmium::memory::padded_length(8) ==  8);
    REQUIRE(osmium::memory::padded_length(9) == 16);

    REQUIRE(osmium::memory::padded_length(2147483647) == 2147483648);
    REQUIRE(osmium::memory::padded_length(2147483648) == 2147483648);
    REQUIRE(osmium::memory::padded_length(2147483650) == 2147483656);

    // The following checks only make sense on a 64 bit system (with
    // sizeof(size_t) == 8), because the numbers are too large for 32 bit.
    // The casts to size_t do nothing on a 64 bit system, on a 32 bit system
    // they bring the numbers into the right range and everything still works.
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967295)) == static_cast<std::size_t>(4294967296));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967296)) == static_cast<std::size_t>(4294967296));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(4294967297)) == static_cast<std::size_t>(4294967304));

    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(7999999999)) == static_cast<std::size_t>(8000000000));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(8000000000)) == static_cast<std::size_t>(8000000000));
    REQUIRE(osmium::memory::padded_length(static_cast<std::size_t>(8000000001)) == static_cast<std::size_t>(8000000008));
}

