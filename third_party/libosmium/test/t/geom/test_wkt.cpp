#include "catch.hpp"

#include "area_helper.hpp"
#include "wnl_helper.hpp"

#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/wkt.hpp>

#include <string>

TEST_CASE("WKT geometry for point") {
    const osmium::geom::WKTFactory<> factory;
    const std::string wkt{factory.create_point(osmium::Location{3.2, 4.2})};
    REQUIRE(wkt == "POINT(3.2 4.2)");
}

TEST_CASE("WKT geometry for empty point") {
    const osmium::geom::WKTFactory<> factory;
    REQUIRE_THROWS_AS(factory.create_point(osmium::Location()), const osmium::invalid_location&);
}

TEST_CASE("WKT geometry for point in ekwt") {
    const osmium::geom::WKTFactory<> factory{7, osmium::geom::wkt_type::ewkt};

    const std::string wkt{factory.create_point(osmium::Location{3.2, 4.2})};
    REQUIRE(wkt == "SRID=4326;POINT(3.2 4.2)");
}

TEST_CASE("WKT geometry for point in ekwt in web mercator") {
    const osmium::geom::WKTFactory<osmium::geom::MercatorProjection> factory{2, osmium::geom::wkt_type::ewkt};

    const std::string wkt{factory.create_point(osmium::Location{3.2, 4.2})};
    REQUIRE(wkt == "SRID=3857;POINT(356222.37 467961.14)");
}

TEST_CASE("WKT geometry factory") {
    osmium::geom::WKTFactory<> factory;

    osmium::memory::Buffer buffer{10000};

    SECTION("linestring") {
        const auto& wnl = create_test_wnl_okay(buffer);

        SECTION("unique forwards (default)") {
            const std::string wkt{factory.create_linestring(wnl)};
            REQUIRE(wkt == "LINESTRING(3.2 4.2,3.5 4.7,3.6 4.9)");
        }

        SECTION("unique backwards") {
            const std::string wkt{factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
            REQUIRE(wkt == "LINESTRING(3.6 4.9,3.5 4.7,3.2 4.2)");
        }

        SECTION("all forwards") {
            const std::string wkt{factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
            REQUIRE(wkt == "LINESTRING(3.2 4.2,3.5 4.7,3.5 4.7,3.6 4.9)");
        }

        SECTION("all backwards") {
            const std::string wkt{factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
            REQUIRE(wkt == "LINESTRING(3.6 4.9,3.5 4.7,3.5 4.7,3.2 4.2)");
        }
    }

    SECTION("polygon") {
        const auto& wnl = create_test_wnl_closed(buffer);

        SECTION("unique forwards (default)") {
            const std::string wkt{factory.create_polygon(wnl)};
            REQUIRE(wkt == "POLYGON((3 3,4.1 4.1,3.6 4.1,3.1 3.5,3 3))");
        }

        SECTION("unique backwards") {
            const std::string wkt{factory.create_polygon(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
            REQUIRE(wkt == "POLYGON((3 3,3.1 3.5,3.6 4.1,4.1 4.1,3 3))");
        }

        SECTION("all forwards") {
            const std::string wkt{factory.create_polygon(wnl,  osmium::geom::use_nodes::all)};
            REQUIRE(wkt == "POLYGON((3 3,4.1 4.1,4.1 4.1,3.6 4.1,3.1 3.5,3 3))");
        }

        SECTION("all backwards") {
            const std::string wkt{factory.create_polygon(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
            REQUIRE(wkt == "POLYGON((3 3,3.1 3.5,3.6 4.1,4.1 4.1,4.1 4.1,3 3))");
        }
    }

    SECTION("empty linestring") {
        const auto& wnl = create_test_wnl_empty(buffer);

        REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward), const osmium::geometry_error&);
    }

    SECTION("linestring with two same locations") {
        const auto& wnl = create_test_wnl_same_location(buffer);

        SECTION("unique forwards") {
            REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::geometry_error&);

            try {
                factory.create_linestring(wnl);
            } catch (const osmium::geometry_error& e) {
                REQUIRE(e.id() == 0);
                REQUIRE(std::string(e.what()) == "need at least two points for linestring");
            }
        }

        SECTION("unique backwards") {
            REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), const osmium::geometry_error&);
        }

        SECTION("all forwards") {
            const std::string wkt{factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
            REQUIRE(wkt == "LINESTRING(3.5 4.7,3.5 4.7)");
        }

        SECTION("all backwards") {
            const std::string wkt{factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
            REQUIRE(wkt == "LINESTRING(3.5 4.7,3.5 4.7)");
        }
    }

    SECTION("linestring with undefined location") {
        const auto& wnl = create_test_wnl_undefined_location(buffer);

        REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::invalid_location&);
    }

    SECTION("area with one outer and no inner rings") {
        const osmium::Area& area = create_test_area_1outer_0inner(buffer);

        const std::string wkt{factory.create_multipolygon(area)};
        REQUIRE(wkt == "MULTIPOLYGON(((3.2 4.2,3.5 4.7,3.6 4.9,3.2 4.2)))");
    }

    SECTION("area with one outer and one inner ring") {
        const osmium::Area& area = create_test_area_1outer_1inner(buffer);

        const std::string wkt{factory.create_multipolygon(area)};
        REQUIRE(wkt == "MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,8 1,8 8,1 8,1 1)))");
    }

    SECTION("area with two outer and two inner rings") {
        const osmium::Area& area = create_test_area_2outer_2inner(buffer);

        const std::string wkt{factory.create_multipolygon(area)};
        REQUIRE(wkt == "MULTIPOLYGON(((0.1 0.1,9.1 0.1,9.1 9.1,0.1 9.1,0.1 0.1),(1 1,4 1,4 4,1 4,1 1),(5 5,5 7,7 7,5 5)),((10 10,11 10,11 11,10 11,10 10)))");
    }

}

