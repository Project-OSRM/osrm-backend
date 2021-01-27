#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/osm/crc.hpp>
#include <osmium/osm/node.hpp>

#include <string>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build node") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer,
        _id(17),
        _version(3),
        _visible(true),
        _cid(333),
        _uid(21),
        _timestamp(time_t(123)),
        _user("foo"),
        _tag("amenity", "pub"),
        _tag("name", "OSM BAR"),
        _location(3.5, 4.7)
    );

    auto& node = buffer.get<osmium::Node>(0);

    REQUIRE(osmium::item_type::node == node.type());
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::node));
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::nwr));
    REQUIRE(17L == node.id());
    REQUIRE(17UL == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(node.visible());
    REQUIRE_FALSE(node.deleted());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
    REQUIRE(std::string{"foo"} == node.user());
    REQUIRE(123 == uint32_t(node.timestamp()));
    REQUIRE(osmium::Location(3.5, 4.7) == node.location());
    REQUIRE(2 == node.tags().size());

    osmium::CRC<crc_type> crc32;
    crc32.update(node);
    REQUIRE(crc32().checksum() == 0x7dc553f9);

    node.set_visible(false);
    REQUIRE_FALSE(node.visible());
    REQUIRE(node.deleted());

    node.remove_tags();
    REQUIRE(node.tags().empty());
}

TEST_CASE("default values for node attributes") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer, _id(0));

    const osmium::Node& node = buffer.get<osmium::Node>(0);
    REQUIRE(0L == node.id());
    REQUIRE(0UL == node.positive_id());
    REQUIRE(0 == node.version());
    REQUIRE(node.visible());
    REQUIRE(0 == node.changeset());
    REQUIRE(0 == node.uid());
    REQUIRE(std::string{} == node.user());
    REQUIRE(0 == uint32_t(node.timestamp()));
    REQUIRE(osmium::Location() == node.location());
    REQUIRE(node.tags().empty());
}

TEST_CASE("set node attributes from strings") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer, _id(0));

    auto& node = buffer.get<osmium::Node>(0);
    node.set_id("-17")
        .set_version("3")
        .set_visible("true")
        .set_changeset("333")
        .set_timestamp("2014-03-17T16:23:08Z")
        .set_uid("21");

    REQUIRE(-17L == node.id());
    REQUIRE(17UL == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(std::string{"2014-03-17T16:23:08Z"} == node.timestamp().to_iso());
    REQUIRE(21 == node.uid());
}

TEST_CASE("set node attributes from strings using set_attribute()") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer, _id(0));

    auto& node = buffer.get<osmium::Node>(0);
    node.set_attribute("id", "-17")
        .set_attribute("version", "3")
        .set_attribute("visible", "true")
        .set_attribute("changeset", "333")
        .set_attribute("timestamp", "2014-03-17T16:23:08Z")
        .set_attribute("uid", "21");

    REQUIRE(-17L == node.id());
    REQUIRE(17UL == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(std::string{"2014-03-17T16:23:08Z"} == node.timestamp().to_iso());
    REQUIRE(21 == node.uid());
}

TEST_CASE("Setting attributes from bad data on strings should fail") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer, _id(0));

    auto& node = buffer.get<osmium::Node>(0);
    REQUIRE_THROWS(node.set_id("bar"));
    REQUIRE_THROWS(node.set_id("123x"));
    REQUIRE_THROWS(node.set_version("123x"));
    REQUIRE_THROWS(node.set_visible("foo"));
    REQUIRE_THROWS(node.set_changeset("123x"));
    REQUIRE_THROWS(node.set_changeset("NULL"));
    REQUIRE_THROWS(node.set_timestamp("2014-03-17T16:23:08Zx"));
    REQUIRE_THROWS(node.set_timestamp("2014-03-17T16:23:99Z"));
    REQUIRE_THROWS(node.set_uid("123x"));
    REQUIRE_THROWS(node.set_uid("anonymous"));
}

TEST_CASE("set large id") {
    osmium::memory::Buffer buffer{10000};

    const int64_t id = 3000000000L;
    osmium::builder::add_node(buffer, _id(id));

    auto& node = buffer.get<osmium::Node>(0);
    REQUIRE(id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());

    node.set_id(-id);
    REQUIRE(-id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());
}

TEST_CASE("set tags on node") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_node(buffer,
        _user("foo"),
        _tag("amenity", "pub"),
        _tag("name", "OSM BAR")
    );

    const osmium::Node& node = buffer.get<osmium::Node>(0);
    REQUIRE(nullptr == node.tags().get_value_by_key("fail"));
    REQUIRE(std::string{"pub"} == node.tags().get_value_by_key("amenity"));
    REQUIRE(std::string{"pub"} == node.get_value_by_key("amenity"));

    REQUIRE(std::string{"default"} == node.tags().get_value_by_key("fail", "default"));
    REQUIRE(std::string{"pub"} == node.tags().get_value_by_key("amenity", "default"));
    REQUIRE(std::string{"pub"} == node.get_value_by_key("amenity", "default"));
}

TEST_CASE("Setting diff flags on node") {
    osmium::memory::Buffer buffer{1000};

    osmium::builder::add_node(buffer, _id(17));

    auto& node = buffer.get<osmium::Node>(0);

    REQUIRE(node.diff() == osmium::diff_indicator_type::none);
    REQUIRE(node.diff_as_char() == '*');

    node.set_diff(osmium::diff_indicator_type::left);
    REQUIRE(node.diff() == osmium::diff_indicator_type::left);
    REQUIRE(node.diff_as_char() == '-');

    node.set_diff(osmium::diff_indicator_type::right);
    REQUIRE(node.diff() == osmium::diff_indicator_type::right);
    REQUIRE(node.diff_as_char() == '+');

    node.set_diff(osmium::diff_indicator_type::both);
    REQUIRE(node.diff() == osmium::diff_indicator_type::both);
    REQUIRE(node.diff_as_char() == ' ');
}

