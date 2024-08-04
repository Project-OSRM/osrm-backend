#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/osm/crc.hpp>
#include <osmium/osm/way.hpp>

#include <string>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build way") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_way(buffer,
        _id(17),
        _version(3),
        _visible(true),
        _cid(333),
        _uid(21),
        _timestamp(static_cast<std::time_t>(123)),
        _user("foo"),
        _tag("highway", "residential"),
        _tag("name", "High Street"),
        _nodes({1, 3, 2})
    );

    auto& way = buffer.get<osmium::Way>(0);

    REQUIRE(osmium::item_type::way == way.type());
    REQUIRE(way.type_is_in(osmium::osm_entity_bits::way));
    REQUIRE(way.type_is_in(osmium::osm_entity_bits::node | osmium::osm_entity_bits::way));
    REQUIRE(17 == way.id());
    REQUIRE(3 == way.version());
    REQUIRE(way.visible());
    REQUIRE(333 == way.changeset());
    REQUIRE(21 == way.uid());
    REQUIRE(std::string("foo") == way.user());
    REQUIRE(123 == uint32_t(way.timestamp()));
    REQUIRE(2 == way.tags().size());
    REQUIRE(3 == way.nodes().size());
    REQUIRE(1 == way.nodes()[0].ref());
    REQUIRE(3 == way.nodes()[1].ref());
    REQUIRE(2 == way.nodes()[2].ref());
    REQUIRE_FALSE(way.is_closed());

    osmium::CRC<crc_type> crc32;
    crc32.update(way);
    REQUIRE(crc32().checksum() == 0x65f6ba91);

    way.remove_tags();
    REQUIRE(way.tags().empty());
    REQUIRE(3 == way.nodes().size());
}

TEST_CASE("build closed way") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_way(buffer,
        _tag("highway", "residential"),
        _tag("name", "High Street"),
        _nodes({1, 3, 1})
    );

    const osmium::Way& way = buffer.get<osmium::Way>(0);

    REQUIRE(way.is_closed());
}

TEST_CASE("build way with helpers") {
    osmium::memory::Buffer buffer{10000};

    {
        osmium::builder::WayBuilder builder(buffer);
        builder.set_user("username");
        builder.add_tags({
            {"amenity", "restaurant"},
            {"name", "Zum goldenen Schwanen"}
        });
        builder.add_node_refs({
            {22, {3.5, 4.7}},
            {67, {4.1, 2.2}}
        });
    }
    buffer.commit();

    const osmium::Way& way = buffer.get<osmium::Way>(0);

    REQUIRE(std::string("username") == way.user());

    REQUIRE(2 == way.tags().size());
    REQUIRE(std::string("amenity") == way.tags().begin()->key());
    REQUIRE(std::string("Zum goldenen Schwanen") == way.tags()["name"]);

    REQUIRE(2 == way.nodes().size());
    REQUIRE(22 == way.nodes()[0].ref());
    REQUIRE(4.1 == Approx(way.nodes()[1].location().lon()));

    const osmium::Box envelope = way.envelope();
    REQUIRE(envelope.bottom_left().lon() == Approx(3.5));
    REQUIRE(envelope.bottom_left().lat() == Approx(2.2));
    REQUIRE(envelope.top_right().lon() == Approx(4.1));
    REQUIRE(envelope.top_right().lat() == Approx(4.7));
}

