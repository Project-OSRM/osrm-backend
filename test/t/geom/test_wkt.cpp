#include "catch.hpp"

#include <osmium/geom/wkt.hpp>

#include "area_helper.hpp"
#include "wnl_helper.hpp"

TEST_CASE("WKT_Geometry") {

SECTION("point") {
    osmium::geom::WKTFactory<> factory;

    std::string wkt {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(std::string{"POINT(3.2 4.2)"} == wkt);
}

SECTION("empty_point") {
    osmium::geom::WKTFactory<> factory;

    REQUIRE_THROWS_AS(factory.create_point(osmium::Location()), osmium::invalid_location);
}

SECTION("linestring") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_okay(buffer);

    {
        std::string wkt {factory.create_linestring(wnl)};
        REQUIRE(std::string{"LINESTRING(3.2 4.2,3.5 4.7,3.6 4.9)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.6 4.9,3.5 4.7,3.2 4.2)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"LINESTRING(3.2 4.2,3.5 4.7,3.5 4.7,3.6 4.9)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.6 4.9,3.5 4.7,3.5 4.7,3.2 4.2)"} == wkt);
    }
}

SECTION("empty_linestring") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_empty(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward), osmium::geometry_error);
}

SECTION("linestring_with_two_same_locations") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_same_location(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);

    try {
        factory.create_linestring(wnl);
    } catch (osmium::geometry_error& e) {
        REQUIRE(e.id() == 0);
        REQUIRE(std::string(e.what()) == "need at least two points for linestring");
    }

    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"LINESTRING(3.5 4.7,3.5 4.7)"} == wkt);
    }

    {
        std::string wkt {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"LINESTRING(3.5 4.7,3.5 4.7)"} == wkt);
    }
}

SECTION("linestring_with_undefined_location") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_undefined_location(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::invalid_location);
}

SECTION("area_1outer_0inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_1outer_0inner(buffer);

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((3.2 4.2,3.5 4.7,3.6 4.9,3.2 4.2)))"} == wkt);
    }
}

SECTION("area_1outer_1inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_1outer_1inner(buffer);

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,8 1,8 8,1 8,1 1)))"} == wkt);
    }
}

SECTION("area_2outer_2inner") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_2outer_2inner(buffer);

    {
        std::string wkt {factory.create_multipolygon(area)};
        REQUIRE(std::string{"MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,4 1,4 4,1 4,1 1),(5 5,5 7,7 7,5 5)),((10 10,11 10,11 11,10 11,10 10)))"} == wkt);
    }
}

}

