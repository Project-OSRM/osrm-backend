#include "catch.hpp"

#include <algorithm>
#include <functional>
#include <vector>

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/object_comparisons.hpp>

using namespace osmium::builder::attr;

TEST_CASE("Node comparisons") {

    osmium::memory::Buffer buffer(10 * 1000);
    std::vector<std::reference_wrapper<osmium::Node>> nodes;

    SECTION("nodes are ordered by id, version, and timestamp") {
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(            0), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(            1), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           10), _version(2), _timestamp("2016-01-01T00:01:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           10), _version(3), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           12), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           12), _version(2), _timestamp("2016-01-01T00:01:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           15), _version(1), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(10000000000ll), _version(2), _timestamp("2016-01-01T00:00:00Z"))));

        REQUIRE(std::is_sorted(nodes.cbegin(), nodes.cend()));
    }

    SECTION("equal nodes are not different") {
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(1), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(1), _version(2), _timestamp("2016-01-01T00:00:00Z"))));

        REQUIRE(nodes[0] == nodes[1]);
        REQUIRE_FALSE(nodes[0] < nodes[1]);
        REQUIRE_FALSE(nodes[0] > nodes[1]);
    }

    SECTION("IDs are ordered by absolute value") {
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(  0))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(  1))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id( -1))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id( 10))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(-10))));

        REQUIRE(std::is_sorted(nodes.cbegin(), nodes.cend()));
    }

    SECTION("reverse version ordering") {
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(            0), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(            1), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           10), _version(3), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           10), _version(2), _timestamp("2016-01-01T00:01:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           12), _version(2), _timestamp("2016-01-01T00:01:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           12), _version(2), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(           15), _version(1), _timestamp("2016-01-01T00:00:00Z"))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(10000000000ll), _version(2), _timestamp("2016-01-01T00:00:00Z"))));

        REQUIRE(std::is_sorted(nodes.cbegin(), nodes.cend(), osmium::object_order_type_id_reverse_version{}));
    }

}

TEST_CASE("Object comparisons") {

    osmium::memory::Buffer buffer(10 * 1000);
    std::vector<std::reference_wrapper<osmium::OSMObject>> objects;

    SECTION("types are ordered nodes, then ways, then relations") {
        objects.emplace_back(buffer.get<osmium::Node>(    osmium::builder::add_node(    buffer, _id(3))));
        objects.emplace_back(buffer.get<osmium::Way>(     osmium::builder::add_way(     buffer, _id(2))));
        objects.emplace_back(buffer.get<osmium::Relation>(osmium::builder::add_relation(buffer, _id(1))));

        REQUIRE(std::is_sorted(objects.cbegin(), objects.cend()));
    }

}

