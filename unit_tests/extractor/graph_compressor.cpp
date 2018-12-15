#include "extractor/graph_compressor.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/restriction.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include "../unit_tests/mocks/mock_scripting_environment.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <unordered_set>
#include <vector>

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
    return {from,                          // source
            to,                            // target
            1,                             // weight
            1,                             // duration
            1,                             // distance
            GeometryID{0, false},          // geometry_id
            false,                         // reversed
            NodeBasedEdgeClassification(), // default flags
            0};                            // AnnotationID
}

bool compatible(Graph const &graph,
                const std::vector<NodeBasedEdgeAnnotation> &node_data_container,
                EdgeID const first,
                EdgeID second)
{
    auto const &first_flags = graph.GetEdgeData(first).flags;
    auto const &second_flags = graph.GetEdgeData(second).flags;
    if (!(first_flags == second_flags))
        return false;

    if (graph.GetEdgeData(first).reversed != graph.GetEdgeData(second).reversed)
        return false;

    auto const &first_annotation = node_data_container[graph.GetEdgeData(first).annotation_data];
    auto const &second_annotation = node_data_container[graph.GetEdgeData(second).annotation_data];

    return first_annotation.CanCombineWith(second_annotation);
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
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    std::vector<NodeBasedEdgeAnnotation> annotations(1);
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    std::vector<InputEdge> edges = {MakeUnitEdge(0, 1),
                                    MakeUnitEdge(1, 0),
                                    MakeUnitEdge(1, 2),
                                    MakeUnitEdge(2, 1),
                                    MakeUnitEdge(2, 3),
                                    MakeUnitEdge(3, 2),
                                    MakeUnitEdge(3, 4),
                                    MakeUnitEdge(4, 3)};

    Graph graph(5, edges);
    BOOST_CHECK(compatible(graph, annotations, 0, 2));
    BOOST_CHECK(compatible(graph, annotations, 2, 4));
    BOOST_CHECK(compatible(graph, annotations, 4, 6));

    compressor.Compress(barrier_nodes,
                        traffic_lights,
                        scripting_environment,
                        restrictions,
                        conditional_restrictions,
                        maneuver_overrides,
                        graph,
                        annotations,
                        container);
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
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    std::vector<NodeBasedEdgeAnnotation> annotations(1);
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

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

    Graph graph(6, edges);
    BOOST_CHECK(edges.size() == 12);
    BOOST_CHECK(compatible(graph, annotations, 0, 1));
    BOOST_CHECK(compatible(graph, annotations, 1, 2));
    BOOST_CHECK(compatible(graph, annotations, 2, 3));
    BOOST_CHECK(compatible(graph, annotations, 3, 4));
    BOOST_CHECK(compatible(graph, annotations, 4, 5));
    BOOST_CHECK(compatible(graph, annotations, 5, 6));
    BOOST_CHECK(compatible(graph, annotations, 6, 7));
    BOOST_CHECK(compatible(graph, annotations, 7, 8));
    BOOST_CHECK(compatible(graph, annotations, 8, 9));
    BOOST_CHECK(compatible(graph, annotations, 9, 10));
    BOOST_CHECK(compatible(graph, annotations, 10, 11));
    BOOST_CHECK(compatible(graph, annotations, 11, 0));

    compressor.Compress(barrier_nodes,
                        traffic_lights,
                        scripting_environment,
                        restrictions,
                        conditional_restrictions,
                        maneuver_overrides,
                        graph,
                        annotations,
                        container);

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
    std::vector<NodeBasedEdgeAnnotation> annotations(1);
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    std::vector<InputEdge> edges = {MakeUnitEdge(0, 1),
                                    MakeUnitEdge(1, 0),
                                    MakeUnitEdge(1, 2),
                                    MakeUnitEdge(1, 3),
                                    MakeUnitEdge(2, 1),
                                    MakeUnitEdge(3, 1)};

    Graph graph(4, edges);
    BOOST_CHECK(compatible(graph, annotations, 0, 1));
    BOOST_CHECK(compatible(graph, annotations, 1, 2));
    BOOST_CHECK(compatible(graph, annotations, 2, 3));
    BOOST_CHECK(compatible(graph, annotations, 3, 4));
    BOOST_CHECK(compatible(graph, annotations, 4, 5));

    compressor.Compress(barrier_nodes,
                        traffic_lights,
                        scripting_environment,
                        restrictions,
                        conditional_restrictions,
                        maneuver_overrides,
                        graph,
                        annotations,
                        container);

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
    std::vector<NodeBasedEdgeAnnotation> annotations(2);
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    std::vector<InputEdge> edges = {
        MakeUnitEdge(0, 1), MakeUnitEdge(1, 0), MakeUnitEdge(1, 2), MakeUnitEdge(2, 1)};

    annotations[1].name_id = 1;
    edges[2].data.annotation_data = edges[3].data.annotation_data = 1;

    Graph graph(5, edges);
    BOOST_CHECK(compatible(graph, annotations, 0, 1));
    BOOST_CHECK(compatible(graph, annotations, 2, 3));

    compressor.Compress(barrier_nodes,
                        traffic_lights,
                        scripting_environment,
                        restrictions,
                        conditional_restrictions,
                        maneuver_overrides,
                        graph,
                        annotations,
                        container);

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
    std::vector<NodeBasedEdgeAnnotation> annotations(1);
    std::vector<TurnRestriction> restrictions;
    std::vector<ConditionalTurnRestriction> conditional_restrictions;
    CompressedEdgeContainer container;
    test::MockScriptingEnvironment scripting_environment;
    std::vector<UnresolvedManeuverOverride> maneuver_overrides;

    std::vector<InputEdge> edges = {
        MakeUnitEdge(0, 1), MakeUnitEdge(1, 0), MakeUnitEdge(1, 2), MakeUnitEdge(2, 1)};
    // make first edge point forward
    edges[1].data.reversed = true;

    Graph graph(5, edges);
    compressor.Compress(barrier_nodes,
                        traffic_lights,
                        scripting_environment,
                        restrictions,
                        conditional_restrictions,
                        maneuver_overrides,
                        graph,
                        annotations,
                        container);

    BOOST_CHECK(graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(graph.FindEdge(1, 2) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_SUITE_END()
