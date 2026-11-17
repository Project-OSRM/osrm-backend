#include "partitioner/edge_based_graph_reader.hpp"

#include "extractor/edge_based_edge.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(directed_graph_preparation)

BOOST_AUTO_TEST_CASE(equal_weight_opposite_turns_preserve_metadata_on_roundtrip)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 0, {15}, {16}, {17}, true, false}, {1, 0, 1, {15}, {26}, {27}, true, false}};
    const auto prepared = osrm::partitioner::prepareEdgesForUsageInGraph<
        osrm::partitioner::DynamicEdgeBasedGraphEdge>(
        osrm::partitioner::splitBidirectionalEdges(edges));
    BOOST_REQUIRE_EQUAL(prepared.size(), 4);
    const osrm::partitioner::DynamicEdgeBasedGraph graph{2, prepared, 0};
    const auto restored = osrm::partitioner::graphToEdges(graph);
    BOOST_REQUIRE_EQUAL(restored.size(), edges.size());
    for (const auto &expected : edges)
    {
        const auto &actual = restored[expected.data.turn_id];
        BOOST_CHECK_EQUAL(actual.source, expected.source);
        BOOST_CHECK_EQUAL(actual.target, expected.target);
        BOOST_CHECK_EQUAL(actual.data.turn_id, expected.data.turn_id);
        BOOST_CHECK(actual.data.weight == expected.data.weight);
        BOOST_CHECK(actual.data.duration == expected.data.duration);
        BOOST_CHECK(actual.data.distance == expected.data.distance);
        BOOST_CHECK(actual.data.forward);
        BOOST_CHECK(!actual.data.backward);
    }
}

BOOST_AUTO_TEST_CASE(equal_cost_distinct_turn_ids_remain_separate)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 0, {15}, {16}, {17}, true, false}, {1, 0, 1, {15}, {16}, {17}, true, false}};
    const auto prepared = osrm::partitioner::prepareEdgesForUsageInGraph<
        osrm::partitioner::DynamicEdgeBasedGraphEdge>(
        osrm::partitioner::splitBidirectionalEdges(edges));
    BOOST_REQUIRE_EQUAL(prepared.size(), 4);
    const osrm::partitioner::DynamicEdgeBasedGraph graph{2, prepared, 0};
    for (const auto edge : graph.GetAdjacentEdgeRange(0))
    {
        const auto &data = graph.GetEdgeData(edge);
        BOOST_CHECK_EQUAL(data.turn_id, data.forward ? 0 : 1);
    }
}

BOOST_AUTO_TEST_CASE(parallel_turns_choose_one_complete_metric)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 0, {15}, {50}, {2000}, true, false},
        {0, 1, 1, {15}, {20}, {1000}, true, false},
        {0, 1, 2, {20}, {1}, {1}, true, false}};
    const auto prepared = osrm::partitioner::prepareEdgesForUsageInGraph<
        osrm::partitioner::DynamicEdgeBasedGraphEdge>(
        osrm::partitioner::splitBidirectionalEdges(edges));
    BOOST_REQUIRE_EQUAL(prepared.size(), 2);
    for (const auto &edge : prepared)
    {
        BOOST_CHECK_EQUAL(edge.data.turn_id, 1);
        BOOST_CHECK(edge.data.weight == EdgeWeight{15});
        BOOST_CHECK(edge.data.duration == 20);
        BOOST_CHECK(edge.data.distance == EdgeDistance{1000});
    }
}

BOOST_AUTO_TEST_SUITE_END()
