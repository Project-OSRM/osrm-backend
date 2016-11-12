#include "catch.hpp"

#include <string>

#include <osmium/geom/factory.hpp>

TEST_CASE("Geometry exception") {

    osmium::geometry_error e{"some error message", "node", 17};
    REQUIRE(e.id() == 17);
    REQUIRE(std::string{e.what()} == "some error message (node_id=17)");

}

