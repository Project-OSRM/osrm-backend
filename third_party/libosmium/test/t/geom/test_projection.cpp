#include "catch.hpp"

#include <random>

#include <osmium/geom/factory.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/projection.hpp>

TEST_CASE("Indentity Projection") {
    osmium::geom::IdentityProjection projection;
    REQUIRE(4326 == projection.epsg());
    REQUIRE("+proj=longlat +datum=WGS84 +no_defs" == projection.proj_string());
}

TEST_CASE("Projection 4326") {
    osmium::geom::Projection projection{4326};
    REQUIRE(4326 == projection.epsg());
    REQUIRE("+init=epsg:4326" == projection.proj_string());

    const osmium::Location loc{1.0, 2.0};
    const osmium::geom::Coordinates c{1.0, 2.0};
    REQUIRE(c == projection(loc));
}

TEST_CASE("Projection 4326 from init string") {
    osmium::geom::Projection projection{"+init=epsg:4326"};
    REQUIRE(-1 == projection.epsg());
    REQUIRE("+init=epsg:4326" == projection.proj_string());

    const osmium::Location loc{1.0, 2.0};
    const osmium::geom::Coordinates c{1.0, 2.0};
    REQUIRE(c == projection(loc));
}

TEST_CASE("Creating projection from unknown init string") {
    REQUIRE_THROWS_AS(osmium::geom::Projection projection{"abc"}, osmium::projection_error);
}

TEST_CASE("Creating projection from unknown EPSG code") {
    REQUIRE_THROWS_AS(osmium::geom::Projection projection{9999999}, osmium::projection_error);
}

TEST_CASE("Projection 3857") {
    osmium::geom::Projection projection{3857};
    REQUIRE(3857 == projection.epsg());
    REQUIRE("+init=epsg:3857" == projection.proj_string());

    SECTION("Zero coordinates") {
        const osmium::Location loc{0.0, 0.0};
        const osmium::geom::Coordinates c{0.0, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Max longitude") {
        const osmium::Location loc{180.0, 0.0};
        const osmium::geom::Coordinates c{20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Min longitude") {
        const osmium::Location loc{-180.0, 0.0};
        const osmium::geom::Coordinates c{-20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Max latitude") {
        const osmium::Location loc{0.0, 85.0511288};
        const osmium::geom::Coordinates c{0.0, 20037508.34};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
}

TEST_CASE("MercatorProjection") {
    osmium::geom::MercatorProjection projection;

    SECTION("Zero coordinates") {
        const osmium::Location loc{0.0, 0.0};
        const osmium::geom::Coordinates c{0.0, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Max longitude") {
        const osmium::Location loc{180.0, 0.0};
        const osmium::geom::Coordinates c{20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Min longitude") {
        const osmium::Location loc{-180.0, 0.0};
        const osmium::geom::Coordinates c{-20037508.34, 0.0};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }

    SECTION("Max latitude") {
        const osmium::Location loc{0.0, 85.0511288};
        const osmium::geom::Coordinates c{0.0, 20037508.34};
        REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.1));
        REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.1));
    }
}

TEST_CASE("Compare mercator implementations") {
    osmium::geom::MercatorProjection projection_merc;
    osmium::geom::Projection projection_3857{3857};

    SECTION("random coordinates") {
        std::random_device rd;
        std::mt19937 gen{rd()};
        std::uniform_real_distribution<> dis_x{-180.0, 180.0};
        std::uniform_real_distribution<> dis_y{-90.0, 90.0};

        for (int n = 0; n < 10000; ++n) {
            const osmium::Location loc{dis_x(gen), dis_y(gen)};
            REQUIRE(projection_merc(loc).x == Approx(projection_3857(loc).x).epsilon(0.1));
            REQUIRE(projection_merc(loc).y == Approx(projection_3857(loc).y).epsilon(0.1));
        }
    }

}
