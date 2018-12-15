#include "extractor/intersection/intersection_analysis.hpp"

#include "extractor/graph_compressor.hpp"

#include "../common/range_tools.hpp"
#include "../unit_tests/mocks/mock_scripting_environment.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(intersection_analysis_tests)

using namespace osrm;
using namespace osrm::guidance;
using namespace osrm::extractor;
using namespace osrm::extractor::intersection;
using InputEdge = util::NodeBasedDynamicGraph::InputEdge;
using Graph = util::NodeBasedDynamicGraph;

BOOST_AUTO_TEST_CASE(simple_intersection_connectivity)
{
    std::unordered_set<NodeID> barrier_nodes{6};
    std::unordered_set<NodeID> traffic_lights;
    std::vector<NodeBasedEdgeAnnotation> annotations{
        {EMPTY_NAMEID, 0, INAVLID_CLASS_DATA, TRAVEL_MODE_DRIVING, false},
        {EMPTY_NAMEID, 1, INAVLID_CLASS_DATA, TRAVEL_MODE_DRIVING, false}};
    std::vector<TurnRestriction> restrictions{TurnRestriction{NodeRestriction{0, 2, 1}, false}};
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    TurnLanesIndexedArray turn_lanes_data{{0, 0, 3},
                                          {TurnLaneType::uturn | TurnLaneType::left,
                                           TurnLaneType::straight,
                                           TurnLaneType::straight | TurnLaneType::right}};

    // Graph with an additional turn restriction 0→2→1 and bollard at 6
    //   0→5↔6↔7
    //   ↕
    // 1↔2←3
    //   ↓
    //   4
    const auto unit_edge =
        [](const NodeID from, const NodeID to, bool allowed, AnnotationID annotation) {
            return InputEdge{from,
                             to,
                             1,
                             1,
                             1,
                             GeometryID{0, false},
                             !allowed,
                             NodeBasedEdgeClassification(),
                             annotation};
        };

    std::vector<InputEdge> edges = {unit_edge(0, 2, true, 1),
                                    unit_edge(0, 5, true, 0),
                                    unit_edge(1, 2, true, 0),
                                    unit_edge(2, 0, true, 0),
                                    unit_edge(2, 1, true, 0),
                                    unit_edge(2, 3, false, 0),
                                    unit_edge(2, 4, true, 0),
                                    unit_edge(3, 2, true, 0),
                                    unit_edge(4, 2, false, 0),
                                    unit_edge(5, 0, false, 0),
                                    unit_edge(5, 6, true, 0),
                                    unit_edge(6, 5, true, 0),
                                    unit_edge(6, 7, true, 0),
                                    unit_edge(7, 6, true, 0)};
    IntersectionEdgeGeometries edge_geometries{
        {0, 180, 180, 10.},  // 0→2
        {1, 90, 90, 10.},    // 0→5
        {2, 90, 90, 10.},    // 1→2
        {3, 0, 0, 10.},      // 2→0
        {4, 270, 270, 10.},  // 2→1
        {5, 90, 90, 10.},    // 2→3
        {6, 180, 180, 10.},  // 2→4
        {7, 270, 270, 10.},  // 3→2
        {8, 0, 0, 10.},      // 4→2
        {9, 270, 270, 10.},  // 5→0
        {10, 90, 90, 10.},   // 5→6
        {11, 270, 270, 10.}, // 6→5
        {12, 90, 90, 10.},   // 6→7
        {13, 270, 270, 10.}  // 7→6
    };

    Graph graph(8, edges);

    GraphCompressor().Compress(barrier_nodes,
                               traffic_lights,
                               scripting_environment,
                               restrictions,
                               conditional_restrictions,
                               maneuver_overrides,
                               graph,
                               annotations,
                               container);

    REQUIRE_SIZE_RANGE(getIncomingEdges(graph, 2), 3);
    REQUIRE_SIZE_RANGE(getOutgoingEdges(graph, 2), 4);

    EdgeBasedNodeDataContainer node_data_container(
        std::vector<EdgeBasedNode>(graph.GetNumberOfEdges()), annotations);
    RestrictionMap restriction_map(restrictions, IndexNodeByFromAndVia());

    const auto connectivity_matrix = [&](NodeID node) {
        std::vector<bool> result;
        const auto incoming_edges = getIncomingEdges(graph, node);
        const auto outgoing_edges = getOutgoingEdges(graph, node);
        for (const auto incoming_edge : incoming_edges)
        {
            for (const auto outgoing_edge : outgoing_edges)
            {
                result.push_back(isTurnAllowed(graph,
                                               node_data_container,
                                               restriction_map,
                                               barrier_nodes,
                                               edge_geometries,
                                               turn_lanes_data,
                                               incoming_edge,
                                               outgoing_edge));
            }
        }
        return result;
    };

    CHECK_EQUAL_RANGE(connectivity_matrix(0), 1, 1); // from node 2 allowed U-turn and to node 5
    CHECK_EQUAL_RANGE(connectivity_matrix(1), 1);    // from node 2 allowed U-turn
    CHECK_EQUAL_RANGE(connectivity_matrix(2),
                      // clang-format off
                      1, 0, 0, 1, // from node 0 to node 4 and a U-turn at 2
                      1, 0, 0, 1, // from node 1 to nodes 0 and 4
                      1, 1, 0, 1  // from node 3 to nodes 0, 1 and 4
                      // clang-format on
                      );
    REQUIRE_SIZE_RANGE(connectivity_matrix(3), 0); // no incoming edges, empty matrix
    CHECK_EQUAL_RANGE(connectivity_matrix(4), 0);  // from node 2 not allowed U-turn
    CHECK_EQUAL_RANGE(connectivity_matrix(5),
                      // clang-format off
                      0, 1, // from node 0 to node 6
                      0, 1, // from node 6 a U-turn to node 6
                      // clang-format on
                      );

    CHECK_EQUAL_RANGE(connectivity_matrix(6),
                      // clang-format off
                      1, 0, // from node 5 a U-turn to node 5
                      0, 1, // from node 7 a U-turn to node 7
                      // clang-format on
                      );
}

BOOST_AUTO_TEST_CASE(roundabout_intersection_connectivity)
{
    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_lights;
    std::vector<NodeBasedEdgeAnnotation> annotations;
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    TurnLanesIndexedArray turn_lanes_data;

    // Graph with roundabout edges 5→0→2
    //  1  2  3
    //   ↘ ↑ ↙
    //     0
    //   ↙ ↑ ↘
    //  4  5  6
    const auto unit_edge = [](const NodeID from, const NodeID to, bool allowed, bool roundabout) {
        return InputEdge{from,
                         to,
                         1,
                         1,
                         1,
                         GeometryID{0, false},
                         !allowed,
                         NodeBasedEdgeClassification{
                             true, false, false, roundabout, false, false, false, {}, 0, 0},
                         0};
    };
    std::vector<InputEdge> edges = {unit_edge(0, 1, false, false),
                                    unit_edge(0, 2, true, true),
                                    unit_edge(0, 3, false, false),
                                    unit_edge(0, 4, true, false),
                                    unit_edge(0, 5, false, true),
                                    unit_edge(0, 6, true, false),
                                    unit_edge(1, 0, true, false),
                                    unit_edge(2, 0, false, true),
                                    unit_edge(3, 0, true, false),
                                    unit_edge(4, 0, false, false),
                                    unit_edge(5, 0, true, true),
                                    unit_edge(6, 0, false, false)};
    IntersectionEdgeGeometries edge_geometries{
        {0, 315, 315, 10}, // 0→1
        {1, 0, 0, 10},     // 0→2
        {2, 45, 45, 10},   // 0→3
        {3, 225, 225, 10}, // 0→4
        {4, 180, 180, 10}, // 0→5
        {5, 135, 135, 10}, // 0→6
        {6, 135, 135, 10}, // 1→0
        {7, 180, 180, 10}, // 2→0
        {8, 225, 225, 10}, // 3→0
        {9, 45, 45, 10},   // 4→0
        {10, 0, 0, 10},    // 5→0
        {11, 315, 315, 10} // 6→0
    };

    Graph graph(7, edges);

    GraphCompressor().Compress(barrier_nodes,
                               traffic_lights,
                               scripting_environment,
                               restrictions,
                               conditional_restrictions,
                               maneuver_overrides,
                               graph,
                               annotations,
                               container);

    REQUIRE_SIZE_RANGE(getIncomingEdges(graph, 0), 3);
    REQUIRE_SIZE_RANGE(getOutgoingEdges(graph, 0), 6);

    EdgeBasedNodeDataContainer node_data_container(
        std::vector<EdgeBasedNode>(graph.GetNumberOfEdges()), annotations);
    RestrictionMap restriction_map(restrictions, IndexNodeByFromAndVia());

    const auto connectivity_matrix = [&](NodeID node) {
        std::vector<bool> result;
        const auto incoming_edges = getIncomingEdges(graph, node);
        const auto outgoing_edges = getOutgoingEdges(graph, node);
        for (const auto incoming_edge : incoming_edges)
        {
            for (const auto outgoing_edge : outgoing_edges)
            {
                result.push_back(isTurnAllowed(graph,
                                               node_data_container,
                                               restriction_map,
                                               barrier_nodes,
                                               edge_geometries,
                                               turn_lanes_data,
                                               incoming_edge,
                                               outgoing_edge));
            }
        }
        return result;
    };

    CHECK_EQUAL_RANGE(connectivity_matrix(0),
                      // clang-format off
                      0, 1, 0, 0, 0, 1, // from node 1 to nodes 2 and 6
                      0, 1, 0, 1, 0, 0, // from node 3 to nodes 2 and 4
                      0, 1, 0, 1, 0, 1  // from node 5 to nodes 2, 4 and 6
                      // clang-format on
                      );
}

BOOST_AUTO_TEST_CASE(skip_degree_two_nodes)
{
    std::unordered_set<NodeID> barrier_nodes{1};
    std::unordered_set<NodeID> traffic_lights{2};
    std::vector<NodeBasedEdgeAnnotation> annotations(1);
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    TurnLanesIndexedArray turn_lanes_data;

    // Graph
    //
    //  0↔1→2↔3↔4→5         7
    //          ↑          ↕ ↕
    //          6         8 ↔ 9
    //
    const auto unit_edge = [](const NodeID from, const NodeID to, bool allowed) {
        return InputEdge{
            from, to, 1, 1, 1, GeometryID{0, false}, !allowed, NodeBasedEdgeClassification{}, 0};
    };
    std::vector<InputEdge> edges = {unit_edge(0, 1, true), // 0
                                    unit_edge(1, 0, true),
                                    unit_edge(1, 2, true),
                                    unit_edge(2, 1, false),
                                    unit_edge(2, 3, true),
                                    unit_edge(3, 2, true), // 5
                                    unit_edge(3, 4, true),
                                    unit_edge(4, 3, true),
                                    unit_edge(4, 5, true),
                                    unit_edge(4, 6, false),
                                    unit_edge(5, 4, false), // 10
                                    unit_edge(6, 4, true),
                                    // Circle
                                    unit_edge(7, 8, true), // 12
                                    unit_edge(7, 9, true),
                                    unit_edge(8, 7, true),
                                    unit_edge(8, 9, true),
                                    unit_edge(9, 7, true),
                                    unit_edge(9, 8, true)};

    Graph graph(10, edges);

    GraphCompressor().Compress(barrier_nodes,
                               traffic_lights,
                               scripting_environment,
                               restrictions,
                               conditional_restrictions,
                               maneuver_overrides,
                               graph,
                               annotations,
                               container);

    BOOST_CHECK_EQUAL(graph.GetTarget(skipDegreeTwoNodes(graph, {0, 0}).edge), 4);
    BOOST_CHECK_EQUAL(graph.GetTarget(skipDegreeTwoNodes(graph, {4, 7}).edge), 0);
    BOOST_CHECK_EQUAL(graph.GetTarget(skipDegreeTwoNodes(graph, {5, 10}).edge), 4);
    BOOST_CHECK_EQUAL(graph.GetTarget(skipDegreeTwoNodes(graph, {6, 11}).edge), 4);
    BOOST_CHECK_EQUAL(graph.GetTarget(skipDegreeTwoNodes(graph, {7, 12}).edge), 7);
}

BOOST_AUTO_TEST_SUITE_END()
