#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/object_comparisons.hpp>

TEST_CASE("Object_Comparisons") {

    using namespace osmium::builder::attr;

    SECTION("order") {
        osmium::memory::Buffer buffer(10 * 1000);

        osmium::builder::add_node(buffer, _id(10), _version(1));
        osmium::builder::add_node(buffer, _id(15), _version(2));

        auto it = buffer.begin();
        osmium::Node& node1 = static_cast<osmium::Node&>(*it);
        osmium::Node& node2 = static_cast<osmium::Node&>(*(++it));

        REQUIRE(node1 < node2);
        REQUIRE_FALSE(node1 > node2);
        node1.set_id(20);
        node1.set_version(1);
        node2.set_id(20);
        node2.set_version(2);
        REQUIRE(node1 < node2);
        REQUIRE_FALSE(node1 > node2);
        node1.set_id(-10);
        node1.set_version(2);
        node2.set_id(-15);
        node2.set_version(1);
        REQUIRE(node1 < node2);
        REQUIRE_FALSE(node1 > node2);
    }

    SECTION("order_types") {
        osmium::memory::Buffer buffer(10 * 1000);

        osmium::builder::add_node(buffer, _id(3), _version(3));
        osmium::builder::add_node(buffer, _id(3), _version(4));
        osmium::builder::add_node(buffer, _id(3), _version(4));
        osmium::builder::add_way(buffer, _id(2), _version(2));
        osmium::builder::add_relation(buffer, _id(1), _version(1));

        auto it = buffer.begin();
        const osmium::Node& node1 = static_cast<const osmium::Node&>(*it);
        const osmium::Node& node2 = static_cast<const osmium::Node&>(*(++it));
        const osmium::Node& node3 = static_cast<const osmium::Node&>(*(++it));
        const osmium::Way& way = static_cast<const osmium::Way&>(*(++it));
        const osmium::Relation& relation = static_cast<const osmium::Relation&>(*(++it));

        REQUIRE(node1 < node2);
        REQUIRE(node2 < way);
        REQUIRE_FALSE(node2 > way);
        REQUIRE(way < relation);
        REQUIRE(node1 < relation);

        REQUIRE(osmium::object_order_type_id_version()(node1, node2));
        REQUIRE(osmium::object_order_type_id_reverse_version()(node2, node1));
        REQUIRE(osmium::object_order_type_id_version()(node1, way));
        REQUIRE(osmium::object_order_type_id_reverse_version()(node1, way));

        REQUIRE_FALSE(osmium::object_equal_type_id_version()(node1, node2));
        REQUIRE(osmium::object_equal_type_id_version()(node2, node3));

        REQUIRE(osmium::object_equal_type_id()(node1, node2));
        REQUIRE(osmium::object_equal_type_id()(node2, node3));

        REQUIRE_FALSE(osmium::object_equal_type_id_version()(node1, way));
        REQUIRE_FALSE(osmium::object_equal_type_id_version()(node1, relation));
        REQUIRE_FALSE(osmium::object_equal_type_id()(node1, relation));
    }

}
