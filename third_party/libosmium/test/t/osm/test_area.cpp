#include "catch.hpp"

#include "test_crc.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/crc.hpp>

#include <string>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build area") {
    osmium::memory::Buffer buffer(10000);

    osmium::builder::add_area(buffer,
        _id(17),
        _version(3),
        _visible(),
        _cid(333),
        _uid(21),
        _timestamp(time_t(123)),
        _user("foo"),
        _tag("landuse", "forest"),
        _tag("name", "Sherwood Forest"),
        _outer_ring({
            {1, {3.2, 4.2}},
            {2, {3.5, 4.7}},
            {3, {3.6, 4.9}},
            {1, {3.2, 4.2}}
        }),
        _inner_ring({
            {5, {1.0, 1.0}},
            {6, {8.0, 1.0}},
            {7, {8.0, 8.0}},
            {8, {1.0, 8.0}},
            {5, {1.0, 1.0}}
        })
    );

    const osmium::Area& area = buffer.get<osmium::Area>(0);

    REQUIRE(17 == area.id());
    REQUIRE(3 == area.version());
    REQUIRE(area.visible());
    REQUIRE(333 == area.changeset());
    REQUIRE(21 == area.uid());
    REQUIRE(std::string("foo") == area.user());
    REQUIRE(123 == uint32_t(area.timestamp()));
    REQUIRE(2 == area.tags().size());

    int inner = 0;
    int outer = 0;
    for (const auto& subitem : area) {
        switch (subitem.type()) {
            case osmium::item_type::outer_ring: {
                    const auto& ring = static_cast<const osmium::OuterRing&>(subitem);
                    REQUIRE(ring.size() == 4);
                    ++outer;
                }
                break;
            case osmium::item_type::inner_ring: {
                    const auto& ring = static_cast<const osmium::OuterRing&>(subitem);
                    REQUIRE(ring.size() == 5);
                    ++inner;
                }
                break;
            default:
                break;
        }
    }

    REQUIRE(outer == 1);
    REQUIRE(inner == 1);

    osmium::CRC<crc_type> crc32;
    crc32.update(area);
    REQUIRE(crc32().checksum() == 0x2b2b7fa0);

    const osmium::Box envelope = area.envelope();
    REQUIRE(envelope.bottom_left().lon() == Approx(3.2));
    REQUIRE(envelope.bottom_left().lat() == Approx(4.2));
    REQUIRE(envelope.top_right().lon() == Approx(3.6));
    REQUIRE(envelope.top_right().lat() == Approx(4.9));
}

