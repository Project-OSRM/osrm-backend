#include "catch.hpp"

#include <osmium/geom/projection.hpp>

TEST_CASE("CRS") {
    const osmium::geom::CRS wgs84{4326};
    const osmium::geom::CRS mercator{3857};

    const osmium::geom::Coordinates c{osmium::geom::deg_to_rad(1.2), osmium::geom::deg_to_rad(3.4)};
    const auto ct = osmium::geom::transform(wgs84, mercator, c);
    const auto c2 = osmium::geom::transform(mercator, wgs84, ct);

    REQUIRE(c.x == Approx(c2.x));
    REQUIRE(c.y == Approx(c2.y));
}

