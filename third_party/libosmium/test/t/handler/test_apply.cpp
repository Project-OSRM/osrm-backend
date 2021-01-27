#include "catch.hpp"

#include "utils.hpp"

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/io/any_compression.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/visitor.hpp>

TEST_CASE("apply with lambdas on reader") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::io::Reader reader{file};

    int count_n = 0;
    int count_w = 0;
    int count_r = 0;
    int count_o = 0;
    int count_a = 0;

    osmium::apply(reader,
        [&](const osmium::Node& node) {
            count_n += node.version();
        },
        [&](const osmium::Way& way) {
            if (way.id() == 20) {
                ++count_w;
            }
        },
        [&](const osmium::Relation& relation) {
            if (relation.id() > 30) {
                ++count_r;
            }
        },
        [&](const osmium::OSMObject& object) {
            if (object.id() % 10 == 0) {
                ++count_o;
            }
        },
        [&](const osmium::Way& way) {
            if (way.id() == 21) {
                ++count_w;
            }
        },
        [&](const osmium::Area& /*area*/) {
            ++count_a;
        }
    );

    REQUIRE(count_n == 5);
    REQUIRE(count_w == 2);
    REQUIRE(count_r == 2);
    REQUIRE(count_o == 3);
    REQUIRE(count_a == 0);
}

TEST_CASE("apply with lambda on buffer") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::io::Reader reader{file};

    const auto buffer = reader.read();
    reader.close();

    std::size_t members = 0;

    osmium::apply(buffer, [&](const osmium::Relation& relation) {
        members += relation.members().size();
    });

    REQUIRE(members == 5);
}

TEST_CASE("apply on non-const buffer can change data") {
    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::io::Reader reader{file};

    auto buffer = reader.read();
    reader.close();

    int nodes = 0;

    osmium::apply(buffer,
        [&](osmium::Node& node) {
            node.set_version(123);
        },
        [&](const osmium::Node& node) {
            ++nodes;
            REQUIRE(node.version() == 123);
        }
    );

    REQUIRE(nodes == 5);
}

TEST_CASE("apply with handler and lambda") {
    using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
    using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    osmium::io::File file{with_data_dir("t/relations/data.osm")};
    osmium::io::Reader reader{file};

    auto buffer = reader.read();
    reader.close();

    index_type index;
    location_handler_type location_handler{index};

    int64_t x = 0;
    int64_t y = 0;

    osmium::apply(buffer,
        [&](const osmium::Way& way) {
            REQUIRE_FALSE(way.nodes().front().location().valid());
        },
        location_handler,
        [&](const osmium::Way& way) {
            REQUIRE(way.nodes().front().location().valid());
            for (const auto& wn : way.nodes()) {
                x += wn.location().x();
                y += wn.location().y();
            }
        }
    );

    REQUIRE(x == 44000000);
    REQUIRE(y == 40000000);
}

