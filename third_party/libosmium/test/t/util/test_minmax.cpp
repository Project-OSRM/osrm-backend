#include "catch.hpp"

#include <osmium/osm/timestamp.hpp>
#include <osmium/util/minmax.hpp>

#include <limits>

TEST_CASE("min_op numeric") {
    osmium::min_op<int> x;
    REQUIRE(x() == std::numeric_limits<int>::max());

    x.update(17);
    REQUIRE(x() == 17);

    x.update(10);
    REQUIRE(x() == 10);

    x.update(22);
    REQUIRE(x() == 10);
}

TEST_CASE("max_op numeric") {
    osmium::max_op<uint32_t> x;
    REQUIRE(x() == 0);

    x.update(17);
    REQUIRE(x() == 17);

    x.update(10);
    REQUIRE(x() == 17);

    x.update(22);
    REQUIRE(x() == 22);
}

TEST_CASE("min_op timestamp") {
    osmium::min_op<osmium::Timestamp> x;

    x.update(osmium::Timestamp("2010-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2010-01-01T00:00:00Z");

    x.update(osmium::Timestamp("2015-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2010-01-01T00:00:00Z");

    x.update(osmium::Timestamp("2000-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2000-01-01T00:00:00Z");
}

TEST_CASE("max_op timestamp") {
    osmium::max_op<osmium::Timestamp> x;

    x.update(osmium::Timestamp("2010-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2010-01-01T00:00:00Z");

    x.update(osmium::Timestamp("2015-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2015-01-01T00:00:00Z");

    x.update(osmium::Timestamp("2000-01-01T00:00:00Z"));
    REQUIRE(x().to_iso() == "2015-01-01T00:00:00Z");
}

