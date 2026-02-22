#include "catch.hpp"

#include <osmium/geom/factory.hpp>
#include <osmium/geom/mercator_projection.hpp>

TEST_CASE("Indentity Projection") {
    const osmium::geom::IdentityProjection projection;
    REQUIRE(4326 == projection.epsg());
    REQUIRE("+proj=longlat +datum=WGS84 +no_defs" == projection.proj_string());
}

TEST_CASE("MercatorProjection: Zero coordinates") {
    const osmium::geom::MercatorProjection projection;
    const osmium::Location loc{0.0, 0.0};
    const osmium::geom::Coordinates c{0.0, 0.0};
    REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.00001));
    REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.00001));
}

TEST_CASE("MercatorProjection: Max longitude") {
    const osmium::geom::MercatorProjection projection;
    const osmium::Location loc{180.0, 0.0};
    const osmium::geom::Coordinates c{20037508.34, 0.0};
    REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.00001));
    REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.00001));
}

TEST_CASE("MercatorProjection: Min longitude") {
    const osmium::geom::MercatorProjection projection;
    const osmium::Location loc{-180.0, 0.0};
    const osmium::geom::Coordinates c{-20037508.34, 0.0};
    REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.00001));
    REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.00001));
}

TEST_CASE("MercatorProjection: Max latitude") {
    const osmium::geom::MercatorProjection projection;
    const osmium::Location loc{0.0, 85.0511288};
    const osmium::geom::Coordinates c{0.0, 20037508.34};
    REQUIRE(projection(loc).x == Approx(c.x).epsilon(0.00001));
    REQUIRE(projection(loc).y == Approx(c.y).epsilon(0.00001));
}

