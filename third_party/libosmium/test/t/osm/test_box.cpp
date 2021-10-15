#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/geom/relations.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/crc.hpp>

#include <sstream>

TEST_CASE("Default constructor creates invalid box") {
    const osmium::Box b{};

    REQUIRE_FALSE(b);
    REQUIRE_FALSE(b.bottom_left());
    REQUIRE_FALSE(b.top_right());
    REQUIRE_THROWS_AS(b.size(), const osmium::invalid_location&);
}

TEST_CASE("Extend box with undefined") {
    osmium::Box b{};

    REQUIRE_FALSE(b);
    b.extend(osmium::Location{});
    REQUIRE_FALSE(b);
    REQUIRE_FALSE(b.bottom_left());
    REQUIRE_FALSE(b.top_right());
}

TEST_CASE("Extend box with invalid") {
    osmium::Box b{};

    REQUIRE_FALSE(b);
    b.extend(osmium::Location{200.0, 100.0});
    REQUIRE_FALSE(b);
    REQUIRE_FALSE(b.bottom_left());
    REQUIRE_FALSE(b.top_right());
}

TEST_CASE("Extend box with valid") {
    osmium::Box b{};

    const osmium::Location loc1{1.2, 3.4};
    b.extend(loc1);
    REQUIRE(!!b);
    REQUIRE(!!b.bottom_left());
    REQUIRE(!!b.top_right());
    REQUIRE(b.contains(loc1));

    const osmium::Location loc2{3.4, 4.5};
    const osmium::Location loc3{5.6, 7.8};

    b.extend(loc2);
    b.extend(loc3);
    REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
    REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));

    // extend with undefined doesn't change anything
    b.extend(osmium::Location{});
    REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
    REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));

    REQUIRE(b.contains(loc1));
    REQUIRE(b.contains(loc2));
    REQUIRE(b.contains(loc3));

    osmium::CRC<crc_type> crc32;
    crc32.update(b);
    REQUIRE(crc32().checksum() == 0xd381a838);
}

TEST_CASE("Output of defined Box") {
    osmium::Box b{};

    b.extend(osmium::Location{1.2, 3.4});
    b.extend(osmium::Location{5.6, 7.8});

    std::stringstream out;
    out << b;

    REQUIRE(out.str() == "(1.2,3.4,5.6,7.8)");
    REQUIRE(b.size() == Approx(19.36).epsilon(0.000001));
}

TEST_CASE("Output of undefined Box") {
    const osmium::Box b{};

    std::stringstream out;
    out << b;

    REQUIRE(out.str() == "(undefined)");
}

TEST_CASE("Output of undefined Box (bottom left)") {
    osmium::Box b{};

    b.top_right() = osmium::Location(1.2, 3.4);
    std::stringstream out;
    out << b;
    REQUIRE(out.str() == "(undefined)");
}

TEST_CASE("Output of undefined Box (top right)") {
    osmium::Box b{};

    b.bottom_left() = osmium::Location(1.2, 3.4);
    std::stringstream out;
    out << b;
    REQUIRE(out.str() == "(undefined)");
}

TEST_CASE("Create box from locations") {
    const osmium::Box b{osmium::Location{1.23, 2.34}, osmium::Location{3.45, 4.56}};
    REQUIRE(!!b);
    REQUIRE(b.bottom_left() == (osmium::Location{1.23, 2.34}));
    REQUIRE(b.top_right() == (osmium::Location{3.45, 4.56}));
}

TEST_CASE("Create box from doubles") {
    const osmium::Box b{1.23, 2.34, 3.45, 4.56};
    REQUIRE(!!b);
    REQUIRE(b.bottom_left() == (osmium::Location{1.23, 2.34}));
    REQUIRE(b.top_right() == (osmium::Location{3.45, 4.56}));
}

TEST_CASE("Relationship between boxes: contains") {
    osmium::Box outer{};
    outer.extend(osmium::Location{1, 1});
    outer.extend(osmium::Location{10, 10});

    osmium::Box inner{};
    inner.extend(osmium::Location{2, 2});
    inner.extend(osmium::Location{4, 4});

    osmium::Box overlap{};
    overlap.extend(osmium::Location{3, 3});
    overlap.extend(osmium::Location{5, 5});

    REQUIRE(      osmium::geom::contains(inner, outer));
    REQUIRE_FALSE(osmium::geom::contains(outer, inner));

    REQUIRE_FALSE(osmium::geom::contains(overlap, inner));
    REQUIRE_FALSE(osmium::geom::contains(inner, overlap));
}

TEST_CASE("Relationship between boxes: overlaps") {
    osmium::Box outer{};
    outer.extend(osmium::Location{1, 1});
    outer.extend(osmium::Location{10, 10});

    osmium::Box inner{};
    inner.extend(osmium::Location{2, 2});
    inner.extend(osmium::Location{4, 4});

    osmium::Box overlap{};
    overlap.extend(osmium::Location{3, 3});
    overlap.extend(osmium::Location{5, 5});

    osmium::Box outside{};
    overlap.extend(osmium::Location{30, 30});
    overlap.extend(osmium::Location{50, 50});

    REQUIRE(osmium::geom::overlaps(inner, outer));
    REQUIRE(osmium::geom::overlaps(outer, inner));

    REQUIRE(osmium::geom::overlaps(overlap, inner));
    REQUIRE(osmium::geom::overlaps(inner, overlap));

    REQUIRE_FALSE(osmium::geom::overlaps(outside, inner));
    REQUIRE_FALSE(osmium::geom::overlaps(inner, outside));
}

