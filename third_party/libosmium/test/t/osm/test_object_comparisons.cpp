#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/object_comparisons.hpp>

#include <algorithm>
#include <functional>
#include <vector>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Object ID comparisons") {
    const osmium::object_id_type a =   0;
    const osmium::object_id_type b =  -1;
    const osmium::object_id_type c = -10;
    const osmium::object_id_type d = -11;
    const osmium::object_id_type e =   1;
    const osmium::object_id_type f =  11;
    const osmium::object_id_type g =  12;

    REQUIRE_FALSE(osmium::id_order{}(a, a));
    REQUIRE(osmium::id_order{}(a, b));
    REQUIRE(osmium::id_order{}(a, c));
    REQUIRE(osmium::id_order{}(a, d));
    REQUIRE(osmium::id_order{}(a, e));
    REQUIRE(osmium::id_order{}(a, f));
    REQUIRE(osmium::id_order{}(a, g));

    REQUIRE_FALSE(osmium::id_order{}(b, a));
    REQUIRE_FALSE(osmium::id_order{}(b, b));
    REQUIRE(osmium::id_order{}(b, c));
    REQUIRE(osmium::id_order{}(b, d));
    REQUIRE(osmium::id_order{}(b, e));
    REQUIRE(osmium::id_order{}(b, f));
    REQUIRE(osmium::id_order{}(b, g));

    REQUIRE_FALSE(osmium::id_order{}(c, a));
    REQUIRE_FALSE(osmium::id_order{}(c, b));
    REQUIRE_FALSE(osmium::id_order{}(c, c));
    REQUIRE(osmium::id_order{}(c, d));
    REQUIRE(osmium::id_order{}(c, e));
    REQUIRE(osmium::id_order{}(c, f));
    REQUIRE(osmium::id_order{}(c, g));

    REQUIRE_FALSE(osmium::id_order{}(d, a));
    REQUIRE_FALSE(osmium::id_order{}(d, b));
    REQUIRE_FALSE(osmium::id_order{}(d, c));
    REQUIRE_FALSE(osmium::id_order{}(d, d));
    REQUIRE(osmium::id_order{}(d, e));
    REQUIRE(osmium::id_order{}(d, f));
    REQUIRE(osmium::id_order{}(d, g));

    REQUIRE_FALSE(osmium::id_order{}(e, a));
    REQUIRE_FALSE(osmium::id_order{}(e, b));
    REQUIRE_FALSE(osmium::id_order{}(e, c));
    REQUIRE_FALSE(osmium::id_order{}(e, d));
    REQUIRE_FALSE(osmium::id_order{}(e, e));
    REQUIRE(osmium::id_order{}(e, f));
    REQUIRE(osmium::id_order{}(e, g));

    REQUIRE_FALSE(osmium::id_order{}(f, a));
    REQUIRE_FALSE(osmium::id_order{}(f, b));
    REQUIRE_FALSE(osmium::id_order{}(f, c));
    REQUIRE_FALSE(osmium::id_order{}(f, d));
    REQUIRE_FALSE(osmium::id_order{}(f, e));
    REQUIRE_FALSE(osmium::id_order{}(f, f));
    REQUIRE(osmium::id_order{}(f, g));

    REQUIRE_FALSE(osmium::id_order{}(g, a));
    REQUIRE_FALSE(osmium::id_order{}(g, b));
    REQUIRE_FALSE(osmium::id_order{}(g, c));
    REQUIRE_FALSE(osmium::id_order{}(g, d));
    REQUIRE_FALSE(osmium::id_order{}(g, e));
    REQUIRE_FALSE(osmium::id_order{}(g, f));
    REQUIRE_FALSE(osmium::id_order{}(g, g));
}

TEST_CASE("Node comparisons") {

    osmium::memory::Buffer buffer{10 * 1000};
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

    SECTION("IDs are ordered by sign and then absolute value") {
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(  0))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id( -1))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(-10))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id(  1))));
        nodes.emplace_back(buffer.get<osmium::Node>(osmium::builder::add_node(buffer, _id( 10))));

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

TEST_CASE("Object comparisons: types are ordered nodes, then ways, then relations") {
    osmium::memory::Buffer buffer{10 * 1000};
    std::vector<std::reference_wrapper<osmium::OSMObject>> objects;

    objects.emplace_back(buffer.get<osmium::Node>(    osmium::builder::add_node(    buffer, _id(3))));
    objects.emplace_back(buffer.get<osmium::Way>(     osmium::builder::add_way(     buffer, _id(2))));
    objects.emplace_back(buffer.get<osmium::Relation>(osmium::builder::add_relation(buffer, _id(1))));

    REQUIRE(std::is_sorted(objects.cbegin(), objects.cend()));
}

TEST_CASE("Object comparisons with partially missing timestamp") {
    osmium::memory::Buffer buffer{10 * 1000};
    osmium::OSMObject& obj1 = buffer.get<osmium::Node>(    osmium::builder::add_node(    buffer,
            _id(3), _version(2), _timestamp(("2016-01-01T00:00:00Z"))));
    osmium::OSMObject& obj2 = buffer.get<osmium::Node>(    osmium::builder::add_node(    buffer,
            _id(3), _version(2)));

    SECTION("OSMObject::operator<") {
        REQUIRE_FALSE(obj1 < obj2);
        REQUIRE_FALSE(obj1 > obj2);
        REQUIRE(obj1 == obj2);
    }

    SECTION("object_order_type_id_reverse_version") {
        const osmium::object_order_type_id_reverse_version comp{};
        REQUIRE_FALSE(comp(obj1, obj2));
        REQUIRE_FALSE(comp(obj2, obj1));
    }
}
