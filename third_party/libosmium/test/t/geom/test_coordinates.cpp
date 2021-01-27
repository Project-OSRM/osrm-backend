#include "catch.hpp"

#include <osmium/geom/coordinates.hpp>

#include <string>

TEST_CASE("Default constructed coordinates are invalid") {
    const osmium::geom::Coordinates c;
    REQUIRE_FALSE(c.valid());
}

TEST_CASE("Coordinates constructed from doubles are valid") {
    const osmium::geom::Coordinates c{1.2, 3.4};
    REQUIRE(c.valid());
    REQUIRE(c.x == Approx(1.2));
    REQUIRE(c.y == Approx(3.4));
}

TEST_CASE("Coordinates constructed from a location are valid") {
    const osmium::Location loc{1.2, 3.4};
    const osmium::geom::Coordinates c{loc};
    REQUIRE(c.valid());
    REQUIRE(c.x == Approx(1.2));
    REQUIRE(c.y == Approx(3.4));
}

TEST_CASE("Comparing coordinates") {
    const osmium::geom::Coordinates ci1;
    const osmium::geom::Coordinates ci2;
    const osmium::geom::Coordinates cv1{1.2, 3.4};
    const osmium::geom::Coordinates cv2{1.2, 3.4};
    const osmium::geom::Coordinates cv3{2.1, 4.3};
    REQUIRE(ci1 == ci2);
    REQUIRE_FALSE(ci1 == cv1);
    REQUIRE(cv1 == cv2);
    REQUIRE_FALSE(cv1 == cv3);
}

TEST_CASE("Write coordinates to string") {
    const osmium::geom::Coordinates c{0.1234567, 1.89898989};
    std::string out;

    SECTION("precision 7") {
        c.append_to_string(out, ',', 7);
        REQUIRE(out == "0.1234567,1.8989899");
    }

    SECTION("precision 3") {
        c.append_to_string(out, ',', 3);
        REQUIRE(out == "0.123,1.899");
    }

    SECTION("with prefix and suffix") {
        c.append_to_string(out, '(', ',', ')', 3);
        REQUIRE(out == "(0.123,1.899)");
    }
}

TEST_CASE("Write invalid coordinates to string") {
    const osmium::geom::Coordinates c;
    std::string out;

    SECTION("with infix only") {
        c.append_to_string(out, ',', 7);
        REQUIRE(out == "invalid");
    }

    SECTION("with prefix and suffix") {
        c.append_to_string(out, '(', ',', ')', 3);
        REQUIRE(out == "(invalid)");
    }
}

