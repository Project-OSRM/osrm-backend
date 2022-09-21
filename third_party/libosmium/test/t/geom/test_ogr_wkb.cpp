#include "catch.hpp"

#include <osmium/geom/ogr.hpp>
#include <osmium/geom/wkb.hpp>
#include <osmium/util/endian.hpp>

#include <memory>
#include <sstream>
#include <string>

#if __BYTE_ORDER == __LITTLE_ENDIAN

#include "area_helper.hpp"
#include "wnl_helper.hpp"

std::string to_wkb(const OGRGeometry* geometry) {
    std::string buffer;
    buffer.resize(geometry->WkbSize());

    geometry->exportToWkb(wkbNDR, reinterpret_cast<unsigned char*>(&*buffer.begin()));

    return buffer;
}

TEST_CASE("compare WKB point against GDAL/OGR") {
    osmium::geom::WKBFactory<> wkb_factory{osmium::geom::wkb_type::wkb};
    osmium::geom::OGRFactory<> ogr_factory;

    const osmium::Location loc{3.2, 4.2};
    const std::string wkb{wkb_factory.create_point(loc)};
    const std::unique_ptr<OGRPoint> geometry = ogr_factory.create_point(loc);
    REQUIRE(to_wkb(geometry.get()) == wkb);
}

TEST_CASE("compare WKB linestring against GDAL/OGR") {
    osmium::geom::WKBFactory<> wkb_factory{osmium::geom::wkb_type::wkb};
    osmium::geom::OGRFactory<> ogr_factory;
    osmium::memory::Buffer buffer{10000};

    const auto& wnl = create_test_wnl_okay(buffer);

    SECTION("linestring") {
        const std::string wkb{wkb_factory.create_linestring(wnl)};
        const std::unique_ptr<OGRLineString> geometry = ogr_factory.create_linestring(wnl);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

    SECTION("linestring, unique nodes, backwards") {
        const std::string wkb{wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        const std::unique_ptr<OGRLineString> geometry = ogr_factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

    SECTION("linestring, all nodes, forwards") {
        const std::string wkb{wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        const std::unique_ptr<OGRLineString> geometry = ogr_factory.create_linestring(wnl, osmium::geom::use_nodes::all);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

    SECTION("linestring, all nodes, backwards") {
        const std::string wkb{wkb_factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        const std::unique_ptr<OGRLineString> geometry = ogr_factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

}

TEST_CASE("compare WKB area against GDAL/OGR") {
    osmium::geom::WKBFactory<> wkb_factory{osmium::geom::wkb_type::wkb};
    osmium::geom::OGRFactory<> ogr_factory;
    osmium::memory::Buffer buffer{10000};

    SECTION("area_1outer_0inner") {
        const osmium::Area& area = create_test_area_1outer_0inner(buffer);

        const std::string wkb{wkb_factory.create_multipolygon(area)};
        const std::unique_ptr<OGRMultiPolygon> geometry = ogr_factory.create_multipolygon(area);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

    SECTION("area_1outer_1inner") {
        const osmium::Area& area = create_test_area_1outer_1inner(buffer);

        const std::string wkb{wkb_factory.create_multipolygon(area)};
        const std::unique_ptr<OGRMultiPolygon> geometry = ogr_factory.create_multipolygon(area);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

    SECTION("area_2outer_2inner") {
        const osmium::Area& area = create_test_area_2outer_2inner(buffer);

        const std::string wkb{wkb_factory.create_multipolygon(area)};
        const std::unique_ptr<OGRMultiPolygon> geometry = ogr_factory.create_multipolygon(area);
        REQUIRE(to_wkb(geometry.get()) == wkb);
    }

}

#endif

