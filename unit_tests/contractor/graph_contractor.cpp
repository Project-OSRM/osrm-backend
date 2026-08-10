#include "contractor/graph_contractor.hpp"

#include "contractor/contractor_graph.hpp"
#include "contractor/graph_contractor_adaptors.hpp"
#include "contractor/query_edge.hpp"
#include "helper.hpp"
#include "util/exception.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <tbb/global_control.h>
#include <tuple>
#include <vector>

using namespace osrm;
using namespace osrm::contractor;
using namespace osrm::unit_test;

#define HAS(a, b) BOOST_CHECK(query_graph.FindEdge(a, b) != SPECIAL_EDGEID);
#define NOT(a, b) BOOST_CHECK(query_graph.FindEdge(a, b) == SPECIAL_EDGEID);

namespace
{
// QueryEdge orders by (source, target) alone and toEdges() sorts with an unstable sort, so
// parallel edges between the same pair of nodes come out in an arbitrary order. Impose a total
// order on top of that before comparing the edge lists of two contraction runs.
bool byAllFields(const QueryEdge &lhs, const QueryEdge &rhs)
{
    const auto as_tuple = [](const QueryEdge &edge)
    {
        return std::make_tuple(edge.source,
                               edge.target,
                               edge.data.weight,
                               static_cast<std::uint32_t>(edge.data.duration),
                               edge.data.distance,
                               static_cast<std::uint32_t>(edge.data.turn_id),
                               static_cast<bool>(edge.data.shortcut),
                               static_cast<bool>(edge.data.forward),
                               static_cast<bool>(edge.data.backward));
    };
    return as_tuple(lhs) < as_tuple(rhs);
}

// A grid is the smallest thing that makes the contractor's edge list grow far enough for a
// compaction pass to have something to reclaim.
std::vector<TestEdge> makeGridEdges(const unsigned width, const unsigned height)
{
    const auto id = [width](const unsigned x, const unsigned y) { return y * width + x; };

    std::vector<TestEdge> edges;
    for (const auto y : util::irange(0u, height))
    {
        for (const auto x : util::irange(0u, width))
        {
            if (x + 1 < width)
                edges.push_back({id(x, y), id(x + 1, y), 1});
            if (y + 1 < height)
                edges.push_back({id(x, y), id(x, y + 1), 1});
        }
    }
    return edges;
}
} // namespace

BOOST_AUTO_TEST_SUITE(graph_contractor)

BOOST_AUTO_TEST_CASE(contract_graph)
{
    {
        /*
         *  0 - 1
         *
         *  1
         *  |
         *  0
         */
        const ContractorGraph g = makeGraph({{0, 1, 1}}); // start, target, weight

        auto query_graph = g;
        contractGraph(query_graph);

        HAS(0, 1)
        NOT(1, 0)
    }

    {
        /*
         *  0 - 1 - 2
         *
         *    1
         *   / \
         *  0   2
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1}});

        auto query_graph = g;
        contractGraph(query_graph);

        HAS(0, 1)
        HAS(2, 1)

        NOT(1, 0)
        NOT(1, 2)
        NOT(2, 0)
        NOT(0, 2)
    }

    {
        /*
         *  0 - 1
         *   \ /
         *    2
         *
         *    2
         *   /|
         *  1 |
         *   \|
         *    0
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {0, 2, 1}});

        auto query_graph = g;
        contractGraph(query_graph);

        HAS(0, 1)
        HAS(0, 2)
        HAS(1, 2)

        NOT(1, 0)
        NOT(2, 0)
        NOT(2, 1)
    }

    {
        /*
         *  0 - 1
         *  |   |
         *  3 - 2
         *
         *      3
         *    / |
         *  1   |
         *  | X |
         *  0   2
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {2, 3, 1},
                                             {3, 0, 1}});

        auto query_graph = g;
        contractGraph(query_graph);

        HAS(0, 1)
        HAS(0, 3)
        HAS(2, 1)
        HAS(2, 3)
        HAS(1, 3)

        HAS(3, 3) // self-loops
        HAS(1, 1)
        NOT(0, 0)
        NOT(2, 2)

        NOT(1, 0)
        NOT(3, 0)
        NOT(1, 2)
        NOT(3, 2)
        NOT(3, 1)

        NOT(0, 2)
        NOT(2, 0)
    }
}

BOOST_AUTO_TEST_CASE(contract_excludable_graph)
{
    {
        /*
         *  Same as above but 0 is uncontractible
         *
         *  0 - 1
         *  |   |
         *  3 - 2
         *
         *  0
         *  | \
         *  |   2
         *  | X |
         *  1   3
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {2, 3, 1},
                                             {3, 0, 1}});

        auto [query_graph, ignore] =
            contractExcludableGraph(g, {{true, true, true, true}, {false, true, true, true}});

        HAS(1, 0)
        HAS(1, 2)
        HAS(3, 0)
        HAS(3, 2)
        HAS(2, 0)
        HAS(0, 2)

        NOT(0, 1)
        NOT(2, 1)
        NOT(0, 3)
        NOT(2, 3)

        NOT(1, 3)
        NOT(3, 1)
    }
}

BOOST_AUTO_TEST_CASE(compaction_leaves_the_contracted_graph_unchanged)
{
    // The contraction order decides which shortcuts are needed, so pin it to compare two runs.
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, 1);

    const auto graph = makeGraph(makeGridEdges(20, 20));

    // On this graph the edge list starts at 1520 slots, grows to 5859, and never holds more than
    // 3722 live edges. A threshold in between compacts repeatedly over the run and always finds
    // enough dead slots to get back under it.
    constexpr std::size_t COMPACTION_THRESHOLD = 4500;

    auto compacted = graph;
    contractGraph(compacted, {}, {}, 1.0, COMPACTION_THRESHOLD);

    auto reference = graph;
    contractGraph(reference);

    auto compacted_edges = toEdges<QueryEdge>(std::move(compacted));
    auto reference_edges = toEdges<QueryEdge>(std::move(reference));
    std::sort(compacted_edges.begin(), compacted_edges.end(), byAllFields);
    std::sort(reference_edges.begin(), reference_edges.end(), byAllFields);

    BOOST_CHECK_EQUAL(compacted_edges.size(), reference_edges.size());
    BOOST_CHECK(compacted_edges == reference_edges);
}

BOOST_AUTO_TEST_CASE(compaction_that_cannot_free_enough_space_throws)
{
    // A threshold below the live edge count: no amount of compacting gets back under it, and
    // contracting on would only repeat the pass for nothing.
    BOOST_CHECK_THROW(
        {
            auto graph = makeGraph({{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {3, 0, 1}});
            contractGraph(graph, {}, {}, 1.0, 1);
        },
        util::exception);
}

BOOST_AUTO_TEST_SUITE_END()
