#include "contractor/graph_contractor_adaptors.hpp"

#include "extractor/edge_based_edge.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

namespace
{
void checkTurn(const osrm::contractor::ContractorGraph &graph,
               const NodeID source,
               const NodeID target,
               const bool forward,
               const NodeID turn_id,
               const EdgeWeight weight,
               const EdgeDuration duration,
               const EdgeDistance distance)
{
    unsigned matches = 0;
    for (const auto edge : graph.GetAdjacentEdgeRange(source))
    {
        const auto &data = graph.GetEdgeData(edge);
        if (graph.GetTarget(edge) == target && (forward ? data.forward : data.backward))
        {
            ++matches;
            BOOST_CHECK_EQUAL(data.id, turn_id);
            BOOST_CHECK(data.weight == weight);
            BOOST_CHECK(data.duration == duration);
            BOOST_CHECK(data.distance == distance);
        }
    }
    BOOST_CHECK_EQUAL(matches, 1);
}
} // namespace

BOOST_AUTO_TEST_SUITE(graph_contractor_adaptors)

BOOST_AUTO_TEST_CASE(asymmetric_turns_preserve_directional_metadata)
{
    std::vector<osrm::extractor::EdgeBasedEdge> edges{{0, 1, 7, {15}, {16}, {17}, true, false},
                                                      {1, 0, 9, {25}, {26}, {27}, true, false}};
    for (unsigned order = 0; order < 2; ++order)
    {
        const auto graph = osrm::contractor::toContractorGraph(2, edges);
        BOOST_REQUIRE_EQUAL(graph.GetNumberOfEdges(), 4);
        checkTurn(graph, 0, 1, true, 7, {15}, {16}, {17});
        checkTurn(graph, 0, 1, false, 9, {25}, {26}, {27});
        checkTurn(graph, 1, 0, true, 9, {25}, {26}, {27});
        checkTurn(graph, 1, 0, false, 7, {15}, {16}, {17});
        std::reverse(edges.begin(), edges.end());
    }
}

BOOST_AUTO_TEST_CASE(equal_cost_opposite_turns_keep_distinct_ids)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 7, {15}, {16}, {17}, true, false}, {1, 0, 9, {15}, {16}, {17}, true, false}};
    const auto graph = osrm::contractor::toContractorGraph(2, edges);
    BOOST_REQUIRE_EQUAL(graph.GetNumberOfEdges(), 4);
    checkTurn(graph, 0, 1, true, 7, {15}, {16}, {17});
    checkTurn(graph, 0, 1, false, 9, {15}, {16}, {17});
}

BOOST_AUTO_TEST_CASE(shared_bidirectional_metadata_can_stay_merged)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 7, {15}, {16}, {17}, true, true}};
    const auto graph = osrm::contractor::toContractorGraph(2, edges);
    BOOST_REQUIRE_EQUAL(graph.GetNumberOfEdges(), 2);
    checkTurn(graph, 0, 1, true, 7, {15}, {16}, {17});
    checkTurn(graph, 0, 1, false, 7, {15}, {16}, {17});
    checkTurn(graph, 1, 0, true, 7, {15}, {16}, {17});
    checkTurn(graph, 1, 0, false, 7, {15}, {16}, {17});
}

BOOST_AUTO_TEST_CASE(parallel_turns_keep_the_selected_edges_metrics)
{
    const std::vector<osrm::extractor::EdgeBasedEdge> edges{
        {0, 1, 7, {10}, {100}, {1000}, true, false},
        {0, 1, 8, {20}, {20}, {200}, true, false},
        {0, 1, 6, {10}, {50}, {1200}, true, false}};
    const auto graph = osrm::contractor::toContractorGraph(2, edges);
    BOOST_REQUIRE_EQUAL(graph.GetNumberOfEdges(), 2);
    checkTurn(graph, 0, 1, true, 6, {10}, {50}, {1200});
    checkTurn(graph, 1, 0, false, 6, {10}, {50}, {1200});
}

BOOST_AUTO_TEST_SUITE_END()
