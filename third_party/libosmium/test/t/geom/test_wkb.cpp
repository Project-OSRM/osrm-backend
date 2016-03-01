#include "catch.hpp"

#include <osmium/geom/wkb.hpp>
#include "wnl_helper.hpp"

#if __BYTE_ORDER == __LITTLE_ENDIAN

TEST_CASE("WKB_Geometry_byte_order_dependent") {

SECTION("point") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    std::string wkb {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(std::string{"01010000009A99999999990940CDCCCCCCCCCC1040"} == wkb);
}

SECTION("point_ewkb") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::ewkb, osmium::geom::out_type::hex);

    std::string wkb {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(std::string{"0101000020E61000009A99999999990940CDCCCCCCCCCC1040"} == wkb);
}

SECTION("linestring") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_okay(buffer);

    {
        std::string wkb {factory.create_linestring(wnl)};
        REQUIRE(std::string{"0102000000030000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCCCC1240CDCCCCCCCCCC0C409A99999999991340"} == wkb);
    }

    {
        std::string wkb {factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        REQUIRE(std::string{"010200000003000000CDCCCCCCCCCC0C409A999999999913400000000000000C40CDCCCCCCCCCC12409A99999999990940CDCCCCCCCCCC1040"} == wkb);
    }

    {
        std::string wkb {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"0102000000040000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCCCC12400000000000000C40CDCCCCCCCCCC1240CDCCCCCCCCCC0C409A99999999991340"} == wkb);
    }

    {
        std::string wkb {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"010200000004000000CDCCCCCCCCCC0C409A999999999913400000000000000C40CDCCCCCCCCCC12400000000000000C40CDCCCCCCCCCC12409A99999999990940CDCCCCCCCCCC1040"} == wkb);
    }
}

SECTION("linestring_ewkb") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::ewkb, osmium::geom::out_type::hex);

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_okay(buffer);

    std::string ewkb {factory.create_linestring(wnl)};
    REQUIRE(std::string{"0102000020E6100000030000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCCCC1240CDCCCCCCCCCC0C409A99999999991340"} == ewkb);
}

SECTION("linestring_with_two_same_locations") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_same_location(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);

    {
        std::string wkb {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(std::string{"0102000000020000000000000000000C40CDCCCCCCCCCC12400000000000000C40CDCCCCCCCCCC1240"} == wkb);
    }

    {
        std::string wkb {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(std::string{"0102000000020000000000000000000C40CDCCCCCCCCCC12400000000000000C40CDCCCCCCCCCC1240"} == wkb);
    }
}

SECTION("linestring_with_undefined_location") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_undefined_location(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::invalid_location);
}

}

#endif

TEST_CASE("WKB_Geometry_byte_order_independent") {

SECTION("empty_point") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    REQUIRE_THROWS_AS(factory.create_point(osmium::Location()), osmium::invalid_location);
}

SECTION("empty_linestring") {
    osmium::geom::WKBFactory<> factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex);

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_empty(buffer);

    REQUIRE_THROWS_AS(factory.create_linestring(wnl), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all), osmium::geometry_error);
    REQUIRE_THROWS_AS(factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward), osmium::geometry_error);
}

}

