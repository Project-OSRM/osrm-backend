#include "extractor/graph_compressor.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/restriction_map.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(graph_compressor)

using namespace osrm;
using namespace osrm::extractor;
using InputEdge = util::NodeBasedDynamicGraph::InputEdge;
using Graph = util::NodeBasedDynamicGraph;

namespace
{

// creates a default edge of unit weight
inline InputEdge MakeUnitEdge(const NodeID from, const NodeID to)
{
    // src, tgt, dist, edge_id, name_id, fwd, bkwd, roundabout, circular, startpoint, local access,
    // split edge, travel_mode
    return {from,
            to,
            1,
            SPECIAL_EDGEID,
            0,
            0,
            false,
            false,
            false,
            false,
            true,
            TRAVEL_MODE_INACCESSIBLE,
            INVALID_LANE_DESCRIPTIONID};
}

} // namespace

BOOST_AUTO_TEST_CASE(long_road_test)
{
    //
    // 0---1---2---3---4
    //
    GraphCompressor compressor;

    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    RestrictionMap map;
    CompressedEdgeContainer container;

    std::vector<InputEdge> edges = {MakeUnitEdge(0, 1),
                                    MakeUnitEdge(1, 0),
                                    MakeUnitEdge(1, 2),
                                    MakeUnitEdge(2, 1),
                                    MakeUnitEdge(2, 3),
                                    MakeUnitEdge(3, 2),
                                    MakeUnitEdge(3, 4),
                                    MakeUnitEdge(4, 3)};

    BOOST_ASSERT(edges[0].data.IsCompatibleTo(edges[2].data));
    BOOST_ASSERT(edges[2].data.IsCompatibleTo(edges[4].data));
    BOOST_ASSERT(edges[4].data.IsCompatibleTo(edges[6].data));

    Graph graph(5, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK_EQUAL(graph.FindEdge(0, 1), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(1, 2), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(2, 3), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(3, 4), SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(0, 4) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(loop_test)
{
    //
    // 0---1---2
    // |       |
    // 5---4---3
    //
    GraphCompressor compressor;

    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    RestrictionMap map;
    CompressedEdgeContainer container;

    std::vector<InputEdge> edges = {MakeUnitEdge(0, 1),
                                    MakeUnitEdge(0, 5),
                                    MakeUnitEdge(1, 0),
                                    MakeUnitEdge(1, 2),
                                    MakeUnitEdge(2, 1),
                                    MakeUnitEdge(2, 3),
                                    MakeUnitEdge(3, 2),
                                    MakeUnitEdge(3, 4),
                                    MakeUnitEdge(4, 3),
                                    MakeUnitEdge(4, 5),
                                    MakeUnitEdge(5, 0),
                                    MakeUnitEdge(5, 4)};

    BOOST_ASSERT(edges.size() == 12);
    BOOST_ASSERT(edges[0].data.IsCompatibleTo(edges[1].data));
    BOOST_ASSERT(edges[1].data.IsCompatibleTo(edges[2].data));
    BOOST_ASSERT(edges[2].data.IsCompatibleTo(edges[3].data));
    BOOST_ASSERT(edges[3].data.IsCompatibleTo(edges[4].data));
    BOOST_ASSERT(edges[4].data.IsCompatibleTo(edges[5].data));
    BOOST_ASSERT(edges[5].data.IsCompatibleTo(edges[6].data));
    BOOST_ASSERT(edges[6].data.IsCompatibleTo(edges[7].data));
    BOOST_ASSERT(edges[7].data.IsCompatibleTo(edges[8].data));
    BOOST_ASSERT(edges[8].data.IsCompatibleTo(edges[9].data));
    BOOST_ASSERT(edges[9].data.IsCompatibleTo(edges[10].data));
    BOOST_ASSERT(edges[10].data.IsCompatibleTo(edges[11].data));

    Graph graph(6, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK_EQUAL(graph.FindEdge(5, 0), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(0, 1), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(1, 2), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(2, 3), SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(5, 3) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(3, 4) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(4, 5) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(t_intersection)
{
    //
    // 0---1---2
    //     |
    //     3
    //
    GraphCompressor compressor;

    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    RestrictionMap map;
    CompressedEdgeContainer container;

    std::vector<InputEdge> edges = {MakeUnitEdge(0, 1),
                                    MakeUnitEdge(1, 0),
                                    MakeUnitEdge(1, 2),
                                    MakeUnitEdge(1, 3),
                                    MakeUnitEdge(2, 1),
                                    MakeUnitEdge(3, 1)};

    BOOST_ASSERT(edges[0].data.IsCompatibleTo(edges[1].data));
    BOOST_ASSERT(edges[1].data.IsCompatibleTo(edges[2].data));
    BOOST_ASSERT(edges[2].data.IsCompatibleTo(edges[3].data));
    BOOST_ASSERT(edges[3].data.IsCompatibleTo(edges[4].data));
    BOOST_ASSERT(edges[4].data.IsCompatibleTo(edges[5].data));

    Graph graph(4, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK(graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 2) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 3) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(street_name_changes)
{
    //
    // 0---1---2
    //
    GraphCompressor compressor;

    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    RestrictionMap map;
    CompressedEdgeContainer container;

    std::vector<InputEdge> edges = {
        MakeUnitEdge(0, 1), MakeUnitEdge(1, 0), MakeUnitEdge(1, 2), MakeUnitEdge(2, 1)};
    edges[2].data.name_id = edges[3].data.name_id = 1;

    BOOST_ASSERT(edges[0].data.IsCompatibleTo(edges[1].data));
    BOOST_ASSERT(edges[2].data.IsCompatibleTo(edges[3].data));

    Graph graph(5, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK(graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 2) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(direction_changes)
{
    //
    // 0-->1---2
    //
    GraphCompressor compressor;

    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    RestrictionMap map;
    CompressedEdgeContainer container;

    std::vector<InputEdge> edges = {
        MakeUnitEdge(0, 1), MakeUnitEdge(1, 0), MakeUnitEdge(1, 2), MakeUnitEdge(2, 1)};
    // make first edge point forward
    edges[1].data.reversed = true;

    Graph graph(5, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK(graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 2) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_SUITE_END()
