#include "catch.hpp"

#include "area_helper.hpp"
#include "wnl_helper.hpp"

#include <osmium/geom/geojson.hpp>

#include <iterator>
#include <string>

TEST_CASE("GeoJSON point geometry") {
    osmium::geom::GeoJSONFactory<> factory;
    const std::string json{factory.create_point(osmium::Location{3.2, 4.2})};
    REQUIRE(std::string{"{\"type\":\"Point\",\"coordinates\":[3.2,4.2]}"} == json);
}

TEST_CASE("GeoJSON empty point geometry") {
    osmium::geom::GeoJSONFactory<> factory;
    REQUIRE_THROWS_AS(factory.create_point(osmium::Location{}), const osmium::invalid_location&);
}

TEST_CASE("GeoJSON linestring geometry") {
    osmium::geom::GeoJSONFactory<> factory;
    osmium::memory::Buffer buffer{1000};

    SECTION("linestring, default") {
        const auto& wnl = create_test_wnl_okay(buffer);
        const std::string json{factory.create_linestring(wnl)};
        REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.2,4.2],[3.5,4.7],[3.6,4.9]]}"} == json);
    }

    SECTION("linestring, unique, backwards") {
        const auto& wnl = create_test_wnl_okay(buffer);
        const std::string json{factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.6,4.9],[3.5,4.7],[3.2,4.2]]}"} == json);
    }

    SECTION("linestring, all") {
        const auto& wnl = create_test_wnl_okay(buffer);
        const std::string json{factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.2,4.2],[3.5,4.7],[3.5,4.7],[3.6,4.9]]}"} == json);
    }

    SECTION("linestring, all, backwards") {
        const auto& wnl = create_test_wnl_okay(buffer);
        const std::string json{factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.6,4.9],[3.5,4.7],[3.5,4.7],[3.2,4.2]]}"} == json);
    }

    SECTION("empty_linestring") {
        const auto& wnl = create_test_wnl_empty(buffer);

        REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward), const osmium::geometry_error&);
    }

    SECTION("linestring with two same locations") {
        const auto& wnl = create_test_wnl_same_location(buffer);

        REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::geometry_error&);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), const osmium::geometry_error&);

        {
            const std::string json{factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
            REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.5,4.7],[3.5,4.7]]}"} == json);
        }

        {
            const std::string json{factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
            REQUIRE(std::string{"{\"type\":\"LineString\",\"coordinates\":[[3.5,4.7],[3.5,4.7]]}"} == json);
        }
    }

    SECTION("linestring with undefined location") {
        const auto& wnl = create_test_wnl_undefined_location(buffer);
        REQUIRE_THROWS_AS(factory.create_linestring(wnl), const osmium::invalid_location&);
    }

}

TEST_CASE("GeoJSON polygon geometry") {
    osmium::geom::GeoJSONFactory<> factory;
    osmium::memory::Buffer buffer{1000};
    const auto& wnl = create_test_wnl_closed(buffer);

    SECTION("unique forwards (default)") {
      const std::string wkt{factory.create_polygon(wnl)};
      REQUIRE(wkt == "{\"type\":\"Polygon\",\"coordinates\":[[[3,3],[4.1,4.1],[3.6,4.1],[3.1,3.5],[3,3]]]}");
    }

    SECTION("unique backwards") {
      const std::string wkt{factory.create_polygon(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
      REQUIRE(wkt == "{\"type\":\"Polygon\",\"coordinates\":[[[3,3],[3.1,3.5],[3.6,4.1],[4.1,4.1],[3,3]]]}");
    }

    SECTION("all forwards") {
      const std::string wkt{factory.create_polygon(wnl,  osmium::geom::use_nodes::all)};
      REQUIRE(wkt == "{\"type\":\"Polygon\",\"coordinates\":[[[3,3],[4.1,4.1],[4.1,4.1],[3.6,4.1],[3.1,3.5],[3,3]]]}");
    }

    SECTION("all backwards") {
      const std::string wkt{factory.create_polygon(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
      REQUIRE(wkt == "{\"type\":\"Polygon\",\"coordinates\":[[[3,3],[3.1,3.5],[3.6,4.1],[4.1,4.1],[4.1,4.1],[3,3]]]}");
    }
}

TEST_CASE("GeoJSON area geometry") {
    osmium::geom::GeoJSONFactory<> factory;
    osmium::memory::Buffer buffer{1000};

    SECTION("area_1outer_0inner") {
        const osmium::Area& area = create_test_area_1outer_0inner(buffer);

        REQUIRE_FALSE(area.is_multipolygon());
        REQUIRE(std::distance(area.cbegin(), area.cend()) == 2);
        REQUIRE(area.subitems<osmium::OuterRing>().size() == area.num_rings().first);

        std::string json{factory.create_multipolygon(area)};
        REQUIRE(std::string{"{\"type\":\"MultiPolygon\",\"coordinates\":[[[[3.2,4.2],[3.5,4.7],[3.6,4.9],[3.2,4.2]]]]}"} == json);
    }

    SECTION("area_1outer_1inner") {
        const osmium::Area& area = create_test_area_1outer_1inner(buffer);

        REQUIRE_FALSE(area.is_multipolygon());
        REQUIRE(std::distance(area.cbegin(), area.cend()) == 3);
        REQUIRE(area.subitems<osmium::OuterRing>().size() == area.num_rings().first);
        REQUIRE(area.subitems<osmium::InnerRing>().size() == area.num_rings().second);

        std::string json{factory.create_multipolygon(area)};
        REQUIRE(std::string{"{\"type\":\"MultiPolygon\",\"coordinates\":[[[[0.1,0.1],[9.1,0.1],[9.1,9.1],[0.1,9.1],[0.1,0.1]],[[1,1],[8,1],[8,8],[1,8],[1,1]]]]}"} == json);
    }

    SECTION("area_2outer_2inner") {
        const osmium::Area& area = create_test_area_2outer_2inner(buffer);

        REQUIRE(area.is_multipolygon());
        REQUIRE(std::distance(area.cbegin(), area.cend()) == 5);
        REQUIRE(area.subitems<osmium::OuterRing>().size() == area.num_rings().first);
        REQUIRE(area.subitems<osmium::InnerRing>().size() == area.num_rings().second);

        int outer_ring=0;
        int inner_ring=0;
        for (const auto& outer : area.outer_rings()) {
            if (outer_ring == 0) {
                REQUIRE(outer.front().ref() == 1);
            } else if (outer_ring == 1) {
                REQUIRE(outer.front().ref() == 100);
            } else {
                REQUIRE(false);
            }
            for (const auto& inner : area.inner_rings(outer)) {
                if (outer_ring == 0 && inner_ring == 0) {
                    REQUIRE(inner.front().ref() == 5);
                } else if (outer_ring == 0 && inner_ring == 1) {
                    REQUIRE(inner.front().ref() == 10);
                } else {
                    REQUIRE(false);
                }
                ++inner_ring;
            }
            inner_ring = 0;
            ++outer_ring;
        }

        std::string json{factory.create_multipolygon(area)};
        REQUIRE(std::string{"{\"type\":\"MultiPolygon\",\"coordinates\":[[[[0.1,0.1],[9.1,0.1],[9.1,9.1],[0.1,9.1],[0.1,0.1]],[[1,1],[4,1],[4,4],[1,4],[1,1]],[[5,5],[5,7],[7,7],[5,5]]],[[[10,10],[11,10],[11,11],[10,11],[10,10]]]]}"} == json);
    }

}

