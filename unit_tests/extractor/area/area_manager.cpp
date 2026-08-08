#include "extractor/area/area_manager.hpp"

#include "extractor/extraction_relation.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>

BOOST_AUTO_TEST_SUITE(area_manager_test)

using namespace osrm;
using namespace osrm::extractor;
using namespace osrm::extractor::area;

namespace
{

constexpr double LON0 = 1.0;
constexpr double LAT0 = 1.0;
constexpr double D = 0.00045;

osmium::Location loc(double col, double row) { return {LON0 + col * D, LAT0 - row * D}; }

/** A closed way, i.e. one whose first and last node are the same. */
const osmium::Way &closed_way(osmium::memory::Buffer &buffer, osmium::object_id_type id)
{
    using namespace osmium::builder::attr;
    const auto pos = osmium::builder::add_way(
        buffer,
        _id(id),
        _nodes({{1, loc(0, 0)}, {2, loc(4, 0)}, {3, loc(4, 4)}, {4, loc(0, 4)}, {1, loc(0, 0)}}));
    return buffer.get<osmium::Way>(pos);
}

const osmium::Way &open_way(osmium::memory::Buffer &buffer, osmium::object_id_type id)
{
    using namespace osmium::builder::attr;
    const auto pos = osmium::builder::add_way(
        buffer, _id(id), _nodes({{1, loc(0, 0)}, {2, loc(4, 0)}, {3, loc(4, 4)}}));
    return buffer.get<osmium::Way>(pos);
}

const osmium::Relation &
relation(osmium::memory::Buffer &buffer, osmium::object_id_type id, const char *type)
{
    using namespace osmium::builder::attr;
    const auto pos = osmium::builder::add_relation(buffer,
                                                   _id(id),
                                                   _tag("type", type),
                                                   _member(osmium::item_type::way, 21, "outer"),
                                                   _member(osmium::item_type::node, 7, ""),
                                                   _member(osmium::item_type::relation, 9, ""));
    return buffer.get<osmium::Relation>(pos);
}

} // namespace

BOOST_AUTO_TEST_CASE(area_manager_is_disabled_until_initialized)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};

    BOOST_CHECK(!manager.is_enabled());

    manager.init("visgraph+dijkstra");
    BOOST_CHECK(manager.is_enabled());

    // initializing twice keeps the algorithm chosen first
    manager.init("something else");
    BOOST_CHECK(manager.is_enabled());
}

// Only closed ways can bound an area; anything else is silently ignored.
BOOST_AUTO_TEST_CASE(area_manager_registers_only_closed_ways)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    osmium::memory::Buffer buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    manager.way(open_way(buffer, 100));
    BOOST_CHECK_EQUAL(manager.number_of_ways, 0u);

    manager.way(closed_way(buffer, 101));
    BOOST_CHECK_EQUAL(manager.number_of_ways, 1u);

    manager.prepare_for_lookup();
    BOOST_CHECK_EQUAL(manager.registered_closed_ways.size(), 1u);
    BOOST_CHECK_EQUAL(manager.registered_closed_ways[0], 101);
}

// Only type=multipolygon relations describe an area.
BOOST_AUTO_TEST_CASE(area_manager_registers_only_multipolygon_relations)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    osmium::memory::Buffer buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    manager.relation(relation(buffer, 200, "route"));
    BOOST_CHECK_EQUAL(manager.number_of_relations, 0u);

    manager.relation(relation(buffer, 201, "multipolygon"));
    BOOST_CHECK_EQUAL(manager.number_of_relations, 1u);
}

// A registered relation can be found again from either of its way or node members, which
// is what the lua process_* functions use to recognize an area.
BOOST_AUTO_TEST_CASE(area_manager_finds_the_relation_of_a_member)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    osmium::memory::Buffer buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    manager.relation(relation(buffer, 201, "multipolygon"));

    osmium::memory::Buffer members{4096, osmium::memory::Buffer::auto_grow::yes};
    const auto &member_way = closed_way(members, 21);
    const auto found_way = manager.get_relations_for_way(member_way);
    BOOST_REQUIRE_EQUAL(found_way.size(), 1u);
    BOOST_CHECK_EQUAL(found_way[0], 201);

    using namespace osmium::builder::attr;
    const auto pos = osmium::builder::add_node(members, _id(7), _location(loc(1, 1)));
    const auto found_node = manager.get_relations_for_node(members.get<osmium::Node>(pos));
    BOOST_REQUIRE_EQUAL(found_node.size(), 1u);
    BOOST_CHECK_EQUAL(found_node[0], 201);

    // a way that is not a member of anything comes back empty
    const auto &stranger = closed_way(members, 999);
    BOOST_CHECK(manager.get_relations_for_way(stranger).empty());
}

// Relation members we cannot use are marked uninteresting by zeroing their reference.
BOOST_AUTO_TEST_CASE(area_manager_tracks_way_and_node_members_only)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    osmium::memory::Buffer buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    manager.relation(relation(buffer, 201, "multipolygon"));

    // the way and the node member are tracked, the nested relation member is not
    BOOST_CHECK_EQUAL(manager.m_way_relation.size(), 1u);
    BOOST_CHECK_EQUAL(manager.m_node_relation.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
