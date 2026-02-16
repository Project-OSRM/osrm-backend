#include "catch.hpp"

#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/wkt.hpp>

#include <string>

TEST_CASE("Projection using MercatorProjection class to WKT") {
    const osmium::geom::WKTFactory<osmium::geom::MercatorProjection> factory{2};

    const std::string wkt{factory.create_point(osmium::Location{3.2, 4.2})};
    REQUIRE(wkt == "POINT(356222.37 467961.14)");
}

