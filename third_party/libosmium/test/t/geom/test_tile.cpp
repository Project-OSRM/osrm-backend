#include "catch.hpp"

#include <sstream>

#include <osmium/geom/tile.hpp>

#include "helper.hpp"

#include "test_tile_data.hpp"

TEST_CASE("Tile") {

    SECTION("x0.0 y0.0 zoom 0") {
        osmium::Location l(0.0, 0.0);

        osmium::geom::Tile t(0, l);

        REQUIRE(t.x == 0);
        REQUIRE(t.y == 0);
        REQUIRE(t.z == 0);
    }

    SECTION("x180.0 y90.0 zoom 0") {
        osmium::Location l(180.0, 90.0);

        osmium::geom::Tile t(0, l);

        REQUIRE(t.x == 0);
        REQUIRE(t.y == 0);
        REQUIRE(t.z == 0);
    }

    SECTION("x180.0 y90.0 zoom 4") {
        osmium::Location l(180.0, 90.0);

        osmium::geom::Tile t(4, l);

        REQUIRE(t.x == (1 << 4) - 1);
        REQUIRE(t.y == 0);
        REQUIRE(t.z == 4);
    }

    SECTION("x0.0 y0.0 zoom 4") {
        osmium::Location l(0.0, 0.0);

        osmium::geom::Tile t(4, l);

        auto n = 1 << (4-1);
        REQUIRE(t.x == n);
        REQUIRE(t.y == n);
        REQUIRE(t.z == 4);
    }

    SECTION("equality") {
        osmium::geom::Tile a(4, 3, 4);
        osmium::geom::Tile b(4, 3, 4);
        osmium::geom::Tile c(4, 4, 3);
        REQUIRE(a == b);
        REQUIRE(a != c);
        REQUIRE(b != c);
    }

    SECTION("order") {
        osmium::geom::Tile a(2, 3, 4);
        osmium::geom::Tile b(4, 3, 4);
        osmium::geom::Tile c(4, 4, 3);
        osmium::geom::Tile d(4, 4, 2);
        REQUIRE(a < b);
        REQUIRE(a < c);
        REQUIRE(b < c);
        REQUIRE(d < c);
    }

    SECTION("tilelist") {
        std::istringstream input_data(s);
        while (input_data) {
            double lon, lat;
            uint32_t x, y, zoom;
            input_data >> lon;
            input_data >> lat;
            input_data >> x;
            input_data >> y;
            input_data >> zoom;

            osmium::Location l(lon, lat);
            osmium::geom::Tile t(zoom, l);
            REQUIRE(t.x == x);
            REQUIRE(t.y == y);
        }
    }

}

