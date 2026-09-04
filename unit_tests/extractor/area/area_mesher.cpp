#include "extractor/area/area_mesher.hpp"

#include "extractor/area/area_data_collector.hpp"
#include "extractor/area/area_manager.hpp"
#include "extractor/area/typedefs.hpp"
#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_relation.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/way.hpp>

#include <array>
#include <set>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_mesher_test)

using namespace osrm;
using namespace osrm::extractor;
using namespace osrm::extractor::area;

namespace
{

// same grid as the visibility graph tests: one step is ~25 m in x and ~50 m in y
constexpr double LON0 = 1.0;
constexpr double LAT0 = 1.0;
constexpr double DLON = 0.000449225603374;
constexpr double DLAT = 0.000452183355507;

osmium::NodeRef node(osmium::object_id_type id, double col, double row)
{ return osmium::NodeRef{id, osmium::Location{LON0 + col * DLON, LAT0 - row * DLAT}}; }

template <typename Ring> void add_ring(Ring &ring, const std::vector<std::array<double, 3>> &pts)
{
    for (const auto &p : pts)
        ring.push_back(node(static_cast<osmium::object_id_type>(p[0]), p[1], p[2]));
}

std::string ids(const NodeRefSet &nodes)
{
    std::string out;
    for (const auto &n : nodes)
    {
        if (!out.empty())
            out += " ";
        out += std::to_string(n.ref());
    }
    return out;
}

/** A square with a square hole, as libosmium orients them: outer ccw, inner cw. */
OsmiumPolygon square_with_hole()
{
    OsmiumPolygon poly;
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 0}, {4, 0, 0}});
    OsmiumPolygon::ring_type inner;
    add_ring(inner, {{5, 3, 5}, {6, 3, 3}, {7, 5, 3}, {8, 5, 5}});
    poly.inners().push_back(inner);
    return poly;
}

/** Builds an osmium::Area with the same geometry, closing every ring. */
void build_area(osmium::memory::Buffer &buffer)
{
    using namespace osmium::builder::attr;
    auto loc = [](double col, double row)
    { return osmium::Location{LON0 + col * DLON, LAT0 - row * DLAT}; };

    osmium::builder::add_area(
        buffer,
        _id(42),
        _outer_ring(
            {{1, loc(0, 8)}, {2, loc(8, 8)}, {3, loc(8, 0)}, {4, loc(0, 0)}, {1, loc(0, 8)}}),
        _inner_ring(
            {{5, loc(3, 5)}, {6, loc(3, 3)}, {7, loc(5, 3)}, {8, loc(5, 5)}, {5, loc(3, 5)}}));
}

/**
 * Attach one distinct way to each of the given nodes, the way AreaMesher::init() expects
 * to find them.  Note that ExtractionContainers seeds way_node_id_offsets with a leading
 * sentinel, so an offset is appended *after* each way rather than before it.
 */
void attach_ways(AreaManager &manager,
                 ExtractionContainers &containers,
                 const std::vector<osmium::object_id_type> &nodes)
{
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        containers.used_node_id_list.push_back(to_alias<OSMNodeID>(nodes[i]));
        containers.ways_list.push_back(to_alias<OSMWayID>(100 + i));
        containers.way_node_id_offsets.push_back(containers.used_node_id_list.size());
        manager.node_ids.emplace(nodes[i]);
    }
}

} // namespace

// Only true corners are obstacles.  On an inner ring that is every corner; a point that
// merely sits along a straight edge is not one, however the projection rounds it.
BOOST_AUTO_TEST_CASE(area_mesher_obstacle_vertices_are_the_hole_corners)
{
    AreaMesher mesher;
    const auto obstacles = mesher.get_obstacle_vertices(square_with_hole());

    BOOST_CHECK_EQUAL(ids(obstacles), "5 6 7 8");
}

BOOST_AUTO_TEST_CASE(area_mesher_collinear_outer_point_is_not_an_obstacle)
{
    OsmiumPolygon poly;
    // 9 sits halfway along the right hand edge, so it is collinear rather than a corner
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {9, 8, 4}, {3, 8, 0}, {4, 0, 0}});

    AreaMesher mesher;
    BOOST_CHECK(mesher.get_obstacle_vertices(poly).empty());
}

// A reflex vertex of the outer ring sticks into the area and does block the view.
BOOST_AUTO_TEST_CASE(area_mesher_reflex_outer_vertex_is_an_obstacle)
{
    OsmiumPolygon poly;
    // an L shape: 5 is the inward corner
    add_ring(poly.outer(), {{1, 0, 8}, {2, 8, 8}, {3, 8, 4}, {5, 4, 4}, {4, 4, 0}, {6, 0, 0}});

    AreaMesher mesher;
    const auto obstacles = mesher.get_obstacle_vertices(poly);

    BOOST_CHECK_EQUAL(ids(obstacles), "5");
}

// area_builder() turns the osmium rings into open boost rings, dropping the repeated
// closing node.
BOOST_AUTO_TEST_CASE(area_mesher_area_builder_opens_the_rings)
{
    osmium::memory::Buffer buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(buffer);

    AreaMesher mesher;
    const auto &area = buffer.get<osmium::Area>(0);
    const auto mp = mesher.area_builder(area);

    BOOST_REQUIRE_EQUAL(mp.size(), 1u);
    BOOST_CHECK_EQUAL(mp[0].outer().size(), 4u); // 5 nodes in the closed ring
    BOOST_REQUIRE_EQUAL(mp[0].inners().size(), 1u);
    BOOST_CHECK_EQUAL(mp[0].inners()[0].size(), 4u);
    // the ring must not repeat its first node
    BOOST_CHECK(mp[0].outer().front().ref() != mp[0].outer().back().ref());
}

// Without any incident ways there is nothing to connect, so the area is not meshed.
BOOST_AUTO_TEST_CASE(area_mesher_no_entry_points_without_incident_ways)
{
    AreaMesher mesher;
    BOOST_CHECK(mesher.get_entry_points(square_with_hole()).empty());
}

// With three distinct ways touching three different perimeter nodes, those nodes become
// the entry points.
BOOST_AUTO_TEST_CASE(area_mesher_entry_points_are_the_nodes_carrying_other_ways)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;

    // three ways, each using exactly one node of the outer ring
    attach_ways(manager, containers, {1, 2, 3});

    AreaMesher mesher;
    mesher.init(manager, containers);
    const auto entry_points = mesher.get_entry_points(square_with_hole());

    BOOST_CHECK_EQUAL(ids(entry_points), "1 2 3");
}

// End to end: an area with three entry points is meshed into virtual ways, and every
// generated way joins two nodes of the area.
BOOST_AUTO_TEST_CASE(area_mesher_meshes_an_area_into_ways)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;

    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);

    BOOST_CHECK_GT(mesher.added_ways, 0);

    const std::set<osmium::object_id_type> area_nodes{1, 2, 3, 4, 5, 6, 7, 8};
    int ways = 0;
    for (const auto &way : out_buffer.select<osmium::Way>())
    {
        ++ways;
        BOOST_REQUIRE_EQUAL(way.nodes().size(), 2u);
        BOOST_CHECK(area_nodes.count(way.nodes()[0].ref()) == 1);
        BOOST_CHECK(area_nodes.count(way.nodes()[1].ref()) == 1);
        BOOST_CHECK(way.nodes()[0].ref() != way.nodes()[1].ref());
        // the generated ways are marked so profiles can recognize them
        BOOST_CHECK_EQUAL(way.get_value_by_key("osrm:virtual", ""), "yes");
    }
    BOOST_CHECK_EQUAL(ways, mesher.added_ways);
}

// An area whose vertex count exceeds the safety valve is left alone.
BOOST_AUTO_TEST_CASE(area_mesher_refuses_to_mesh_too_many_vertices)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;

    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.max_vertices = 1;
    mesher.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);

    BOOST_CHECK_EQUAL(mesher.added_ways, 0);
}

// mesh_buffer() walks every area in the input buffer.
BOOST_AUTO_TEST_CASE(area_mesher_mesh_buffer_walks_every_area)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;

    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{8192, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{8192, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher single;
    single.init(manager, containers);
    single.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);
    const int one = single.added_ways;

    osmium::memory::Buffer both_out{8192, osmium::memory::Buffer::auto_grow::yes};
    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.mesh_buffer(in_buffer, both_out, relations);

    BOOST_CHECK_EQUAL(mesher.added_ways, 2 * one);
}

// BufferReader hands out areas in batches and then reports end of input; reading past
// that is a programming error.
BOOST_AUTO_TEST_CASE(area_mesher_buffer_reader_batches_then_reports_eof)
{
    osmium::memory::Buffer in_buffer{16384, osmium::memory::Buffer::auto_grow::yes};
    for (int i = 0; i < 6; ++i)
        build_area(in_buffer);

    BufferReader reader{in_buffer};

    auto count_areas = [](const osmium::memory::Buffer &b)
    {
        int n = 0;
        for (const auto &area : b.select<osmium::Area>())
        {
            (void)area;
            ++n;
        }
        return n;
    };

    const auto first = reader.read(); // at most 4 areas per buffer
    BOOST_CHECK_EQUAL(count_areas(first), 4);
    const auto second = reader.read();
    BOOST_CHECK_EQUAL(count_areas(second), 2);

    const auto eof = reader.read();
    BOOST_CHECK(!eof); // an invalid buffer signals end of input
    BOOST_CHECK_THROW(reader.read(), osrm::util::exception);
}

// The engine needs the polygon and its entry points to snap a coordinate lying inside
// the area later, and meshing is the only point where both are known.
BOOST_AUTO_TEST_CASE(area_mesher_records_the_area_for_the_engine)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;
    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.area_walking_speed = 1.25;
    mesher.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);

    BOOST_REQUIRE_EQUAL(mesher.collector().size(), 1u);
    const auto &record = mesher.collector().polygons()[0];

    // the outer ring, open, without the repeated closing node
    std::string ring;
    for (const auto &n : record.boundary_vertices)
        ring += (ring.empty() ? "" : " ") + std::to_string(n.ref());
    BOOST_CHECK_EQUAL(ring, "1 2 3 4");

    BOOST_CHECK_CLOSE(record.walking_speed, 1.25, 0.001);
}

// An area nobody can enter is of no use to snapping either, so it is not recorded.
BOOST_AUTO_TEST_CASE(area_mesher_does_not_record_an_area_without_entry_points)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;
    // no ways attached, so get_entry_points() finds nothing

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);

    BOOST_CHECK_EQUAL(mesher.collector().size(), 0u);
}

// An area too large to mesh is exactly where snapping matters most, so it is recorded
// even though no ways are generated for it.
BOOST_AUTO_TEST_CASE(area_mesher_records_an_area_it_declined_to_mesh)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;
    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);
    osmium::memory::Buffer out_buffer{4096, osmium::memory::Buffer::auto_grow::yes};

    AreaMesher mesher;
    mesher.init(manager, containers);
    mesher.max_vertices = 1;
    mesher.mesh_area(in_buffer.get<osmium::Area>(0), out_buffer, relations);

    BOOST_CHECK_EQUAL(mesher.added_ways, 0);
    BOOST_CHECK_EQUAL(mesher.collector().size(), 1u);
}

// With the whole visibility graph emitted, every node of the area has onward edges, so a
// coordinate snapped inside it can set off towards any node it can see.  The entry-point
// mesh leaves obstacle corners with none.
BOOST_AUTO_TEST_CASE(area_mesher_can_emit_the_whole_visibility_graph)
{
    ExtractionRelationContainer relations;
    AreaManager manager{relations};
    ExtractionContainers containers;
    attach_ways(manager, containers, {1, 2, 3});

    osmium::memory::Buffer in_buffer{4096, osmium::memory::Buffer::auto_grow::yes};
    build_area(in_buffer);

    const auto mesh = [&](bool whole)
    {
        osmium::memory::Buffer out{8192, osmium::memory::Buffer::auto_grow::yes};
        AreaMesher mesher;
        mesher.init(manager, containers);
        mesher.emit_visibility_graph = whole;
        mesher.mesh_area(in_buffer.get<osmium::Area>(0), out, relations);
        std::set<std::pair<osmium::object_id_type, osmium::object_id_type>> edges;
        for (const auto &way : out.select<osmium::Way>())
        {
            const auto a = way.nodes()[0].ref(), b = way.nodes()[1].ref();
            edges.emplace(std::min(a, b), std::max(a, b));
        }
        return edges;
    };

    const auto entry_point_mesh = mesh(false);
    const auto whole_graph = mesh(true);

    // The whole graph is a superset.  Not necessarily a strict one: the pruning keeps the
    // shortest-path tree rooted at each entry point, and on an area this small every sight
    // line lies on one of those trees, so the two coincide.  The saving shows up on larger
    // areas, where the forest grows as entry points x vertices and the whole graph as
    // vertices squared.
    BOOST_CHECK_GE(whole_graph.size(), entry_point_mesh.size());
    for (const auto &edge : entry_point_mesh)
        BOOST_CHECK_MESSAGE(whole_graph.count(edge) == 1,
                            "edge " << edge.first << "-" << edge.second
                                    << " is in the entry-point mesh but not the whole graph");

    // every corner of the obstacle has at least one edge, in either mesh: the ring edges
    // are emitted whichever way the visibility graph is pruned, and without them a
    // straight line to such a corner is a dead end
    const auto degree = [](const auto &edges, osmium::object_id_type node)
    {
        std::size_t count = 0;
        for (const auto &edge : edges)
            count += (edge.first == node || edge.second == node);
        return count;
    };
    for (osmium::object_id_type corner : {5, 6, 7, 8})
    {
        BOOST_CHECK_MESSAGE(degree(whole_graph, corner) > 0,
                            "obstacle corner " << corner << " has no onward edge");
        BOOST_CHECK_MESSAGE(degree(entry_point_mesh, corner) > 0,
                            "obstacle corner " << corner << " has no onward edge when pruned");
    }
}

BOOST_AUTO_TEST_SUITE_END()
