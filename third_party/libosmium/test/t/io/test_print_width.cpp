#include "catch.hpp"

#include <osmium/io/detail/debug_output_format.hpp>

TEST_CASE("Calculate width of a number") {
    REQUIRE(osmium::io::detail::print_width(0) == 1);
    REQUIRE(osmium::io::detail::print_width(1) == 1);
    REQUIRE(osmium::io::detail::print_width(2) == 1);
    REQUIRE(osmium::io::detail::print_width(9) == 1);
    REQUIRE(osmium::io::detail::print_width(10) == 1); // 0 .. 9
    REQUIRE(osmium::io::detail::print_width(11) == 2);
    REQUIRE(osmium::io::detail::print_width(42) == 2);
    REQUIRE(osmium::io::detail::print_width(99) == 2);
    REQUIRE(osmium::io::detail::print_width(100) == 2); // 0 .. 99
    REQUIRE(osmium::io::detail::print_width(101) == 3);
    REQUIRE(osmium::io::detail::print_width(999) == 3);
    REQUIRE(osmium::io::detail::print_width(1000) == 3); // 0 .. 100
    REQUIRE(osmium::io::detail::print_width(1001) == 4);
}
