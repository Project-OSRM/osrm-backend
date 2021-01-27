#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/osm/crc.hpp>
#include <osmium/osm/relation.hpp>

#include <string>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build relation") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::add_relation(buffer,
        _id(17),
        _version(3),
        _visible(),
        _cid(333),
        _uid(21),
        _timestamp(time_t(123)),
        _user("foo"),
        _tag("type", "multipolygon"),
        _tag("name", "Sherwood Forest"),
        _member(osmium::item_type::way, 1, "inner"),
        _member(osmium::item_type::way, 2, ""),
        _member(osmium::item_type::way, 3, "outer")
    );

    const osmium::Relation& relation = buffer.get<osmium::Relation>(0);

    REQUIRE(17 == relation.id());
    REQUIRE(3 == relation.version());
    REQUIRE(relation.visible());
    REQUIRE(333 == relation.changeset());
    REQUIRE(21 == relation.uid());
    REQUIRE(std::string("foo") == relation.user());
    REQUIRE(123 == uint32_t(relation.timestamp()));
    REQUIRE(2 == relation.tags().size());
    REQUIRE(3 == relation.members().size());

    int n=1;
    for (auto& member : relation.members()) {
        REQUIRE(osmium::item_type::way == member.type());
        REQUIRE(n == member.ref());
        switch (n) {
            case 1:
                REQUIRE(std::string("inner") == member.role());
                break;
            case 2:
                REQUIRE(std::string("") == member.role());
                break;
            case 3:
                REQUIRE(std::string("outer") == member.role());
                break;
            default:
                REQUIRE(false);
        }
        ++n;
    }

    osmium::CRC<crc_type> crc32;
    crc32.update(relation);
    REQUIRE(crc32().checksum() == 0x2c2352e);
}

TEST_CASE("Member role too long") {
    osmium::memory::Buffer buffer{10000};

    osmium::builder::RelationMemberListBuilder builder{buffer};

    const char role[2000] = "";
    builder.add_member(osmium::item_type::node, 1, role, 1024);
    REQUIRE_THROWS(builder.add_member(osmium::item_type::node, 1, role, 1025));
}

