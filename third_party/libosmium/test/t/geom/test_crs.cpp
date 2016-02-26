#include "catch.hpp"

#include <random>

#include <osmium/geom/projection.hpp>

TEST_CASE("CRS") {
    osmium::geom::CRS wgs84{4326};
    osmium::geom::CRS mercator{3857};

    osmium::geom::Coordinates c{osmium::geom::deg_to_rad(1.2), osmium::geom::deg_to_rad(3.4)};
    auto ct = osmium::geom::transform(wgs84, mercator, c);
    auto c2 = osmium::geom::transform(mercator, wgs84, ct);

    REQUIRE(c.x == Approx(c2.x));
    REQUIRE(c.y == Approx(c2.y));
}

