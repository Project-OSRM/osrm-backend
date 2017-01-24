
#include "catch.hpp"

#include <osmium/util/memory.hpp>

TEST_CASE("Check memory usage") {
#ifdef __linux__
    const int size_in_mbytes = 10;

    osmium::MemoryUsage m1;
    REQUIRE(m1.current() > 1);
    REQUIRE(m1.peak() > 1);

// Memory reporting on M68k architecture doesn't work properly.
# ifndef __m68k__
    {
        std::vector<int> v;
        v.reserve(size_in_mbytes * 1024 * 1024);

        osmium::MemoryUsage m2;
        REQUIRE(m2.current() >= m1.current() + size_in_mbytes);
        REQUIRE(m2.peak() >= m1.peak() + size_in_mbytes);
        REQUIRE(m2.peak() - m2.current() <= 1);
    }

    osmium::MemoryUsage m3;
    REQUIRE(m3.current() > 1);
    REQUIRE(m3.current() < m3.peak());
    REQUIRE(m3.peak() >= m1.peak() + size_in_mbytes);
# endif
#else
    osmium::MemoryUsage m;
    REQUIRE(m.current() == 0);
    REQUIRE(m.peak() == 0);
#endif
}

