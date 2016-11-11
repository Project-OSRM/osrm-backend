#include "catch.hpp"

#include <osmium/osm/entity_bits.hpp>

static_assert((osmium::osm_entity_bits::node
              |osmium::osm_entity_bits::way
              |osmium::osm_entity_bits::relation)
              == osmium::osm_entity_bits::nwr, "entity_bits nwr failed");

static_assert((osmium::osm_entity_bits::node
              |osmium::osm_entity_bits::way
              |osmium::osm_entity_bits::relation
              |osmium::osm_entity_bits::area)
              == osmium::osm_entity_bits::nwra, "entity_bits nwra failed");

static_assert((osmium::osm_entity_bits::nwra
              |osmium::osm_entity_bits::changeset)
              == osmium::osm_entity_bits::all, "entity_bits all failed");

static_assert((osmium::osm_entity_bits::all
              &osmium::osm_entity_bits::node)
              == osmium::osm_entity_bits::node, "entity_bits node failed");

static_assert((~osmium::osm_entity_bits::all) == osmium::osm_entity_bits::nothing, "entity_bits nothing is the inverse of all");
static_assert((~osmium::osm_entity_bits::nothing) == osmium::osm_entity_bits::all, "entity_bits all is the inverse of nothing");
static_assert((~osmium::osm_entity_bits::changeset) == osmium::osm_entity_bits::nwra, "entity_bits nwra is the inverse of changeset");

TEST_CASE("Bitwise 'and' and 'or' on entity bits") {
    osmium::osm_entity_bits::type entities = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way;
    REQUIRE(entities == (osmium::osm_entity_bits::node | osmium::osm_entity_bits::way));

    entities |= osmium::osm_entity_bits::relation;
    REQUIRE((entities & osmium::osm_entity_bits::object));

    entities |= osmium::osm_entity_bits::area;
    REQUIRE(entities == osmium::osm_entity_bits::object);

    REQUIRE_FALSE((entities & osmium::osm_entity_bits::changeset));

    entities &= osmium::osm_entity_bits::node;
    REQUIRE((entities & osmium::osm_entity_bits::node));
    REQUIRE_FALSE((entities & osmium::osm_entity_bits::way));
    REQUIRE(entities == osmium::osm_entity_bits::node);
}

TEST_CASE("Bitwise 'not' on entity bits") {
    REQUIRE(~osmium::osm_entity_bits::all == osmium::osm_entity_bits::nothing);
    REQUIRE(~osmium::osm_entity_bits::nothing == osmium::osm_entity_bits::all);
    REQUIRE(~osmium::osm_entity_bits::node == (osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation | osmium::osm_entity_bits::area | osmium::osm_entity_bits::changeset));
    REQUIRE(~osmium::osm_entity_bits::nwr == (osmium::osm_entity_bits::area | osmium::osm_entity_bits::changeset));
    REQUIRE(~osmium::osm_entity_bits::nwra == osmium::osm_entity_bits::changeset);
}

TEST_CASE("Converting item types to entity bits") {
    REQUIRE(osmium::osm_entity_bits::nothing   == osmium::osm_entity_bits::from_item_type(osmium::item_type::undefined));
    REQUIRE(osmium::osm_entity_bits::node      == osmium::osm_entity_bits::from_item_type(osmium::item_type::node));
    REQUIRE(osmium::osm_entity_bits::way       == osmium::osm_entity_bits::from_item_type(osmium::item_type::way));
    REQUIRE(osmium::osm_entity_bits::relation  == osmium::osm_entity_bits::from_item_type(osmium::item_type::relation));
    REQUIRE(osmium::osm_entity_bits::changeset == osmium::osm_entity_bits::from_item_type(osmium::item_type::changeset));
    REQUIRE(osmium::osm_entity_bits::area      == osmium::osm_entity_bits::from_item_type(osmium::item_type::area));
}

