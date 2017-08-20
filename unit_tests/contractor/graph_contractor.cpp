#include "contractor/graph_contractor.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <tbb/task_scheduler_init.h>

using namespace osrm;
using namespace osrm::contractor;

BOOST_AUTO_TEST_SUITE(graph_contractor)

using TestEdge = std::tuple<unsigned, unsigned, int>;
ContractorGraph makeGraph(const std::vector<TestEdge> &edges)
{
    std::vector<ContractorEdge> input_edges;
    auto id = 0u;
    auto max_id = 0u;
    for (const auto &edge : edges)
    {
        unsigned start;
        unsigned target;
        int weight;
        std::tie(start, target, weight) = edge;
        max_id = std::max(std::max(start, target), max_id);
        input_edges.push_back(ContractorEdge{
            start, target, ContractorEdgeData{weight, weight * 2, id++, 0, false, true, false}});
        input_edges.push_back(ContractorEdge{
            target, start, ContractorEdgeData{weight, weight * 2, id++, 0, false, false, true}});
    }
    std::sort(input_edges.begin(), input_edges.end());

    return ContractorGraph{max_id + 1, std::move(input_edges)};
}

BOOST_AUTO_TEST_CASE(contract_graph)
{
    tbb::task_scheduler_init scheduler(1);
    /*
     *                 <--1--<
     * (0) >--3--> (1) >--3--> (3)
     *  v          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    ^        \       /
     *      >(5)          > (4) >
     */
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3},
                                   TestEdge{0, 5, 1},
                                   TestEdge{1, 3, 3},
                                   TestEdge{1, 4, 1},
                                   TestEdge{3, 1, 1},
                                   TestEdge{4, 3, 1},
                                   TestEdge{5, 1, 1}};
    auto reference_graph = makeGraph(edges);

    auto contracted_graph = reference_graph;
    std::vector<bool> core = contractGraph(contracted_graph, {1, 1, 1, 1, 1, 1});

    // This contraction order is dependent on the priority caculation in the contractor
    // but deterministic for the same graph.
    CHECK_EQUAL_RANGE(core, false, false, false, false, false, false);

    /* After contracting 0 and 2:
     *
     * Deltes edges 5 -> 0, 1 -> 0
     *
     *                 <--1--<
     * (0) ---3--> (1) >--3--> (3)
     *  \          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    ^        \       /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(5, 0);
    reference_graph.DeleteEdgesTo(1, 0);

    /* After contracting 5:
     *
     * Deletes edges 1 -> 5
     *
     *                 <--1--<
     * (0) ---3--> (1) >--3--> (3)
     *  \          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    /        \       /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(5, 0);
    reference_graph.DeleteEdgesTo(1, 0);

    /* After contracting 3:
     *
     * Deletes edges 1 -> 3
     * Deletes edges 4 -> 3
     * Insert edge 4 -> 1
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(1, 3);
    reference_graph.DeleteEdgesTo(4, 3);
    // Insert shortcut
    reference_graph.InsertEdge(4, 1, {2, 4, 3, 0, true, true, false});

    /* After contracting 4:
     *
     * Delete edges 1 -> 4
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          \ (4) >
     */
    reference_graph.DeleteEdgesTo(1, 4);

    /* After contracting 1:
     *
     * Delete no edges.
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          \ (4) >
     */

    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(0), 2);
    BOOST_CHECK(contracted_graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(contracted_graph.FindEdge(0, 5) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(1), 0);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(2), 0);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(3), 3);
    BOOST_CHECK(contracted_graph.FindEdge(3, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(contracted_graph.FindEdge(3, 4) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(4), 2);
    BOOST_CHECK(contracted_graph.FindEdge(4, 1) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(5), 1);
    BOOST_CHECK(contracted_graph.FindEdge(5, 1) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_SUITE_END()
