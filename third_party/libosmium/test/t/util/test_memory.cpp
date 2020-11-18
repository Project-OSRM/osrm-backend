#include "catch.hpp"

#include <osmium/util/memory.hpp>

#include <vector>

TEST_CASE("Check memory usage") {
#ifdef __linux__
    osmium::MemoryUsage m1;
    REQUIRE(m1.current() > 1);
    REQUIRE(m1.peak() > 1);
#else
    osmium::MemoryUsage m;
    REQUIRE(m.current() == 0);
    REQUIRE(m.peak() == 0);
#endif
}

