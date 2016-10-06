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

    std::vector<InputEdge> edges = {
        // src, tgt, dist, edge_id, name_id, access_restricted, fwd, bkwd, roundabout, travel_mode
        {0,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         3,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {3,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {3,
         4,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {4,
         3,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID}};

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

    std::vector<InputEdge> edges = {
        // src, tgt, dist, edge_id, name_id, access_restricted, fwd, bkwd, roundabout, travel_mode
        {0,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {0,
         5,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         3,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {3,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {3,
         4,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {4,
         3,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {4,
         5,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {5,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {5,
         4,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
    };

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

    std::vector<InputEdge> edges = {
        // src, tgt, dist, edge_id, name_id, access_restricted, fwd, bkwd, roundabout, travel_mode
        {0,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         3,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {3,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
    };

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
        // src, tgt, dist, edge_id, name_id, access_restricted, fwd, bkwd, roundabout, travel_mode
        {0,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         2,
         1,
         SPECIAL_EDGEID,
         1,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         1,
         1,
         SPECIAL_EDGEID,
         1,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
    };

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
        // src, tgt, dist, edge_id, name_id, access_restricted, fwd, bkwd, roundabout, travel_mode
        {0,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         0,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         true,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {1,
         2,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
        {2,
         1,
         1,
         SPECIAL_EDGEID,
         0,
         false,
         false,
         false,
         true,  // startpoint
         false, // road_priority_forward
         false, // road_priority_backward
         TRAVEL_MODE_INACCESSIBLE,
         INVALID_LANE_DESCRIPTIONID},
    };

    Graph graph(5, edges);
    compressor.Compress(barrier_nodes, traffic_lights, map, graph, container);

    BOOST_CHECK(graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 2) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_SUITE_END()
