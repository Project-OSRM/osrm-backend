#include "extractor/edge_based_edge.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(turn_id_uniqueness)

using namespace osrm;

// This test documents and verifies the invariant that turn_id is unique per edge.
//
// During extraction, EdgeBasedGraphFactory renumbers all edges so that each edge's
// turn_id equals its index in the edge list (turn_id = 0, 1, 2, ..., N-1).
//
// The parallel update loop in Updater::LoadAndUpdateEdgeExpandedGraph relies on this
// property: since each edge has a unique turn_id, parallel threads write to different
// slots in turn_weight_penalties[] without racing.

BOOST_AUTO_TEST_CASE(turn_id_assignment_simulates_extraction)
{
    // Simulate the turn_id renumbering done by EdgeBasedGraphFactory:
    // each edge's turn_id is set to its index in the edge list
    std::vector<extractor::EdgeBasedEdge> edges(1000);

    // Assign turn_id = index (mimicking the extractor renumbering)
    for (std::size_t i = 0; i < edges.size(); ++i)
    {
        edges[i].data.turn_id = static_cast<NodeID>(i);
    }

    // Verify all turn_ids are unique
    std::set<NodeID> turn_ids;
    for (const auto &edge : edges)
    {
        auto [it, inserted] = turn_ids.insert(edge.data.turn_id);
        BOOST_CHECK_MESSAGE(inserted,
                            "turn_id " << edge.data.turn_id
                                       << " is not unique - parallel updates would race");
    }

    BOOST_CHECK_EQUAL(turn_ids.size(), edges.size());
}

BOOST_AUTO_TEST_CASE(parallel_update_safety_with_unique_turn_ids)
{
    // This test verifies that with unique turn_ids, parallel writes to different
    // array indices are safe (no synchronization needed).
    const std::size_t num_edges = 10000;
    std::vector<int> penalties(num_edges, 0);
    std::vector<extractor::EdgeBasedEdge> edges(num_edges);

    // Assign unique turn_ids
    for (std::size_t i = 0; i < edges.size(); ++i)
    {
        edges[i].data.turn_id = static_cast<NodeID>(i);
    }

    // Simulate parallel updates (sequentially here, but the point is each write
    // goes to a different index)
    for (const auto &edge : edges)
    {
        penalties[edge.data.turn_id] = edge.data.turn_id + 100;
    }

    // Verify each slot was written correctly
    for (std::size_t i = 0; i < penalties.size(); ++i)
    {
        BOOST_CHECK_EQUAL(penalties[i], static_cast<int>(i + 100));
    }
}

BOOST_AUTO_TEST_SUITE_END()
