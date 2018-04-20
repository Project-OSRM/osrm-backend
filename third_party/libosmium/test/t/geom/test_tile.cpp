#include "catch.hpp"

#include "test_tile_data.hpp"

#include <osmium/geom/tile.hpp>

#include <sstream>

TEST_CASE("Helper functions") {
    REQUIRE(osmium::geom::num_tiles_in_zoom(0) == 1);
    REQUIRE(osmium::geom::num_tiles_in_zoom(1) == 2);
    REQUIRE(osmium::geom::num_tiles_in_zoom(12) == 4096);

    REQUIRE(osmium::geom::tile_extent_in_zoom(1) == osmium::geom::detail::max_coordinate_epsg3857);
}

TEST_CASE("Tile from x0.0 y0.0 at zoom 0") {
    osmium::Location l{0.0, 0.0};

    osmium::geom::Tile t{0, l};

    REQUIRE(t.x == 0);
    REQUIRE(t.y == 0);
    REQUIRE(t.z == 0);
    REQUIRE(t.valid());
}

TEST_CASE("Tile from x180.0 y90.0 at zoom 0") {
    osmium::Location l{180.0, 90.0};

    osmium::geom::Tile t{0, l};

    REQUIRE(t.x == 0);
    REQUIRE(t.y == 0);
    REQUIRE(t.z == 0);
    REQUIRE(t.valid());
}

TEST_CASE("Tile from x180.0 y90.0 at zoom 4") {
    osmium::Location l{180.0, 90.0};

    osmium::geom::Tile t{4, l};

    REQUIRE(t.x == (1u << 4u) - 1);
    REQUIRE(t.y == 0);
    REQUIRE(t.z == 4);
    REQUIRE(t.valid());
}

TEST_CASE("Tile from x0.0 y0.0 at zoom 4") {
    osmium::Location l{0.0, 0.0};

    osmium::geom::Tile t{4, l};

    const auto n = 1u << (4u - 1u);
    REQUIRE(t.x == n);
    REQUIRE(t.y == n);
    REQUIRE(t.z == 4);
    REQUIRE(t.valid());
}

TEST_CASE("Tile from max values at zoom 4") {
    osmium::geom::Tile t{4u, 15u, 15u};
    REQUIRE(t.valid());
}

TEST_CASE("Tile from max values at zoom 30") {
    osmium::geom::Tile t{30u, (1u << 30u) - 1, (1u << 30u) - 1};
    REQUIRE(t.valid());
}

TEST_CASE("Tile from coordinates") {
    osmium::geom::Coordinates c{9.99312, 53.55078};
    osmium::geom::Tile t{12, osmium::geom::lonlat_to_mercator(c)};
    REQUIRE(t.valid());
    REQUIRE(t.x == 2161);
    REQUIRE(t.y == 1323);
}

TEST_CASE("Tile equality") {
    osmium::geom::Tile a{4, 3, 4};
    osmium::geom::Tile b{4, 3, 4};
    osmium::geom::Tile c{4, 4, 3};
    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(b != c);
}

TEST_CASE("Tile order") {
    osmium::geom::Tile a{4, 3, 4};
    osmium::geom::Tile b{6, 3, 4};
    osmium::geom::Tile c{6, 4, 3};
    osmium::geom::Tile d{6, 4, 2};
    REQUIRE(a < b);
    REQUIRE(a < c);
    REQUIRE(b < c);
    REQUIRE(d < c);
}

TEST_CASE("Check a random list of tiles") {
    std::istringstream input_data(s);
    while (input_data) {
        double lon, lat;
        uint32_t x, y, zoom;
        input_data >> lon;
        input_data >> lat;
        input_data >> x;
        input_data >> y;
        input_data >> zoom;

        osmium::Location l{lon, lat};
        osmium::geom::Tile t{zoom, l};
        REQUIRE(t.x == x);
        REQUIRE(t.y == y);
    }
}

TEST_CASE("Invalid tiles") {
    osmium::geom::Tile tile{0, 0, 0};

    REQUIRE(tile.valid());

    SECTION("Zoom level out of bounds") {
        tile.z = 100;
    }
    SECTION("x out of bounds") {
        tile.x = 1;
    }
    SECTION("y out of bounds") {
        tile.y = 1;
    }
    SECTION("x out of bounds") {
        tile.z = 4;
        tile.x = 100;
    }
    SECTION("y out of bounds") {
        tile.z = 4;
        tile.y = 100;
    }

    REQUIRE_FALSE(tile.valid());
}

