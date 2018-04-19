#include "catch.hpp"

#include <osmium/area/assembler.hpp>
#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/area.hpp>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Build area from way") {
    osmium::memory::Buffer buffer{10240};

    const auto wpos = osmium::builder::add_way(buffer,
        _id(1),
        _nodes({
            {1, {1.0, 1.0}},
            {2, {1.0, 2.0}},
            {3, {2.0, 2.0}},
            {4, {2.0, 1.0}},
            {1, {1.0, 1.0}}
        })
    );

    osmium::area::AssemblerConfig config;
    osmium::area::Assembler assembler{config};

    osmium::memory::Buffer area_buffer{10240};
    REQUIRE(assembler(buffer.get<osmium::Way>(wpos), area_buffer));

    const auto& area = area_buffer.get<osmium::Area>(0);
    REQUIRE(area.from_way());
    REQUIRE(area.id() == 2);

    const auto it = area.outer_rings().begin();
    REQUIRE(it != area.outer_rings().end());
    REQUIRE(it->size() == 5);

    const auto& s = assembler.stats();
    REQUIRE(s.area_simple_case == 1);
    REQUIRE(s.from_ways == 1);
    REQUIRE(s.nodes == 4);
    REQUIRE(s.duplicate_nodes == 0);
    REQUIRE(s.invalid_locations == 0);
}

TEST_CASE("Build area from way with duplicate nodes") {
    osmium::memory::Buffer buffer{10240};

    const auto wpos = osmium::builder::add_way(buffer,
        _id(1),
        _nodes({
            {1, {1.0, 1.0}},
            {2, {1.0, 2.0}},
            {3, {2.0, 2.0}},
            {3, {2.0, 2.0}},
            {4, {2.0, 1.0}},
            {1, {1.0, 1.0}}
        })
    );

    osmium::area::AssemblerConfig config;
    osmium::area::Assembler assembler{config};

    osmium::memory::Buffer area_buffer{10240};
    REQUIRE(assembler(buffer.get<osmium::Way>(wpos), area_buffer));

    const auto& area = area_buffer.get<osmium::Area>(0);
    REQUIRE(area.from_way());
    REQUIRE(area.id() == 2);

    const auto it = area.outer_rings().begin();
    REQUIRE(it != area.outer_rings().end());
    REQUIRE(it->size() == 5);

    const auto& s = assembler.stats();
    REQUIRE(s.area_simple_case == 1);
    REQUIRE(s.from_ways == 1);
    REQUIRE(s.nodes == 4);
    REQUIRE(s.duplicate_nodes == 1);
    REQUIRE(s.invalid_locations == 0);
}

TEST_CASE("Build area from way with invalid location") {
    osmium::memory::Buffer buffer{10240};

    const auto wpos = osmium::builder::add_way(buffer,
        _id(1),
        _nodes({
            {1, {1.0, 1.0}},
            {2, {1.0, 2.0}},
            {3},
            {4, {2.0, 1.0}},
            {1, {1.0, 1.0}}
        })
    );

    osmium::area::AssemblerConfig config;
    osmium::area::Assembler assembler{config};

    osmium::memory::Buffer area_buffer{10240};
    REQUIRE_FALSE(assembler(buffer.get<osmium::Way>(wpos), area_buffer));

    const auto& s = assembler.stats();
    REQUIRE(s.duplicate_nodes == 0);
    REQUIRE(s.invalid_locations == 1);
}

TEST_CASE("Build area from way with ignored invalid location") {
    osmium::memory::Buffer buffer{10240};

    const auto wpos = osmium::builder::add_way(buffer,
        _id(1),
        _nodes({
            {1, {1.0, 1.0}},
            {2, {1.0, 2.0}},
            {3},
            {4, {2.0, 1.0}},
            {1, {1.0, 1.0}}
        })
    );

    osmium::area::AssemblerConfig config;
    config.ignore_invalid_locations = true;
    osmium::area::Assembler assembler{config};

    osmium::memory::Buffer area_buffer{10240};
    REQUIRE(assembler(buffer.get<osmium::Way>(wpos), area_buffer));

    const auto& area = area_buffer.get<osmium::Area>(0);
    REQUIRE(area.from_way());
    REQUIRE(area.id() == 2);

    const auto it = area.outer_rings().begin();
    REQUIRE(it != area.outer_rings().end());
    REQUIRE(it->size() == 4);

    const auto& s = assembler.stats();
    REQUIRE(s.area_simple_case == 1);
    REQUIRE(s.from_ways == 1);
    REQUIRE(s.nodes == 3);
    REQUIRE(s.invalid_locations == 1);
}

