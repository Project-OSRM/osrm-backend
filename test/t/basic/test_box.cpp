#include "catch.hpp"

#include <sstream>

#include <boost/crc.hpp>

#include <osmium/osm/box.hpp>
#include <osmium/osm/crc.hpp>
#include <osmium/geom/relations.hpp>

TEST_CASE("Starting with default constructed box") {

    osmium::Box b;

    SECTION("default constructor creates invalid box") {
        REQUIRE(!b);
        REQUIRE(!b.bottom_left());
        REQUIRE(!b.top_right());
        REQUIRE_THROWS_AS(b.size(), osmium::invalid_location);
    }

    SECTION("extend with undefined") {
        REQUIRE(!b);
        b.extend(osmium::Location{});
        REQUIRE(!b);
        REQUIRE(!b.bottom_left());
        REQUIRE(!b.top_right());
    }

    SECTION("extend with invalid") {
        REQUIRE(!b);
        b.extend(osmium::Location{200.0, 100.0});
        REQUIRE(!b);
        REQUIRE(!b.bottom_left());
        REQUIRE(!b.top_right());
    }

    SECTION("extend with valid") {
        osmium::Location loc1 { 1.2, 3.4 };
        b.extend(loc1);
        REQUIRE(!!b);
        REQUIRE(!!b.bottom_left());
        REQUIRE(!!b.top_right());
        REQUIRE(b.contains(loc1));

        osmium::Location loc2 { 3.4, 4.5 };
        osmium::Location loc3 { 5.6, 7.8 };

        b.extend(loc2);
        b.extend(loc3);
        REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
        REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));

        // extend with undefined doesn't change anything
        b.extend(osmium::Location());
        REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
        REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));

        REQUIRE(b.contains(loc1));
        REQUIRE(b.contains(loc2));
        REQUIRE(b.contains(loc3));

        osmium::CRC<boost::crc_32_type> crc32;
        crc32.update(b);
        REQUIRE(crc32().checksum() == 0xd381a838);
    }

    SECTION("output defined") {
        b.extend(osmium::Location(1.2, 3.4));
        b.extend(osmium::Location(5.6, 7.8));
        std::stringstream out;
        out << b;
        REQUIRE(out.str() == "(1.2,3.4,5.6,7.8)");
        REQUIRE(b.size() == Approx(19.36).epsilon(0.000001));
    }

    SECTION("output undefined") {
        std::stringstream out;
        out << b;
        REQUIRE(out.str() == "(undefined)");
    }

    SECTION("output undefined bottom left") {
        b.top_right() = osmium::Location(1.2, 3.4);
        std::stringstream out;
        out << b;
        REQUIRE(out.str() == "(undefined)");
    }

    SECTION("output undefined top right") {
        b.bottom_left() = osmium::Location(1.2, 3.4);
        std::stringstream out;
        out << b;
        REQUIRE(out.str() == "(undefined)");
    }

}

TEST_CASE("Create box from locations") {
    osmium::Box b{osmium::Location{1.23, 2.34}, osmium::Location{3.45, 4.56}};
    REQUIRE(!!b);
    REQUIRE(b.bottom_left() == (osmium::Location{1.23, 2.34}));
    REQUIRE(b.top_right() == (osmium::Location{3.45, 4.56}));
}

TEST_CASE("Create box from doubles") {
    osmium::Box b{1.23, 2.34, 3.45, 4.56};
    REQUIRE(!!b);
    REQUIRE(b.bottom_left() == (osmium::Location{1.23, 2.34}));
    REQUIRE(b.top_right() == (osmium::Location{3.45, 4.56}));
}

TEST_CASE("Relationship between boxes") {

    osmium::Box outer;
    outer.extend(osmium::Location(1, 1));
    outer.extend(osmium::Location(10, 10));

    osmium::Box inner;
    inner.extend(osmium::Location(2, 2));
    inner.extend(osmium::Location(4, 4));

    osmium::Box overlap;
    overlap.extend(osmium::Location(3, 3));
    overlap.extend(osmium::Location(5, 5));

    REQUIRE( osmium::geom::contains(inner, outer));
    REQUIRE(!osmium::geom::contains(outer, inner));

    REQUIRE(!osmium::geom::contains(overlap, inner));
    REQUIRE(!osmium::geom::contains(inner, overlap));

}

