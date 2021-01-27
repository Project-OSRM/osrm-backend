#include "partitioner/multi_level_graph.hpp"
#include "util/typedefs.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <random>
#include <vector>

using namespace osrm;
using namespace osrm::partitioner;

namespace
{
struct MockEdge
{
    NodeID source;
    NodeID target;
};

auto makeGraph(const MultiLevelPartition &mlp, const std::vector<MockEdge> &mock_edges)
{
    struct EdgeData
    {
        bool forward;
        bool backward;
    };
    using Edge = util::static_graph_details::SortableEdgeWithData<EdgeData>;
    std::vector<Edge> edges;
    std::size_t max_id = 0;
    for (const auto &m : mock_edges)
    {
        max_id = std::max<std::size_t>(max_id, std::max(m.source, m.target));
        edges.push_back(Edge{m.source, m.target, true, false});
        edges.push_back(Edge{m.target, m.source, false, true});
    }
    std::sort(edges.begin(), edges.end());
    return MultiLevelGraph<EdgeData, osrm::storage::Ownership::Container>(mlp, max_id + 1, edges);
}
} // namespace

BOOST_AUTO_TEST_SUITE(multi_level_graph)

BOOST_AUTO_TEST_CASE(check_edges_sorting)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11 12 13
    std::vector<CellID> l1{{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6}};
    std::vector<CellID> l2{{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4}};
    std::vector<CellID> l3{{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1}};
    MultiLevelPartition mlp{{l1, l2, l3, l4}, {7, 5, 3, 2}};

    std::vector<MockEdge> edges = {
        // edges sorted into border/internal by level
        //  level:  (1) (2) (3) (4)
        {0, 1},   //  i   i   i   i
        {2, 3},   //  i   i   i   i
        {3, 7},   //  b   b   b   i
        {4, 0},   //  b   b   b   i
        {4, 5},   //  i   i   i   i
        {5, 6},   //  b   i   i   i
        {6, 4},   //  b   i   i   i
        {6, 7},   //  i   i   i   i
        {7, 11},  //  b   b   i   i
        {8, 9},   //  i   i   i   i
        {9, 8},   //  i   i   i   i
        {10, 11}, //  i   i   i   i
        {11, 10}, //  i   i   i   i
        {11, 12}, //  b   b   b   b
        {12, 13}  //  i   i   i   i
    };

    auto graph = makeGraph(mlp, edges);

    for (auto from : util::irange(0u, graph.GetNumberOfNodes()))
    {
        LevelID level = 0;
        for (auto edge : graph.GetAdjacentEdgeRange(from))
        {
            auto to = graph.GetTarget(edge);
            BOOST_CHECK(mlp.GetHighestDifferentLevel(from, to) >= level);
            level = mlp.GetHighestDifferentLevel(from, to);
        }
    }

    // on level 0 every edge is a border edge
    for (auto node : util::irange<NodeID>(0, 13))
        CHECK_EQUAL_COLLECTIONS(graph.GetBorderEdgeRange(0, node),
                                graph.GetAdjacentEdgeRange(node));

    // on level 0 there are no internal edges
    for (auto node : util::irange<NodeID>(0, 13))
        CHECK_EQUAL_COLLECTIONS(graph.GetInternalEdgeRange(0, node), util::irange<EdgeID>(0, 0));

    // the union of border and internal edge needs to equal the adjacent edges
    for (auto level : util::irange<LevelID>(0, 4))
    {
        for (auto node : util::irange<NodeID>(0, 14))
        {
            const auto adjacent = graph.GetAdjacentEdgeRange(node);
            const auto border = graph.GetBorderEdgeRange(level, node);
            const auto internal = graph.GetInternalEdgeRange(level, node);
            std::vector<EdgeID> merged;
            std::copy(internal.begin(), internal.end(), std::back_inserter(merged));
            std::copy(border.begin(), border.end(), std::back_inserter(merged));
            CHECK_EQUAL_COLLECTIONS(adjacent, merged);
        }
    }

    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 0).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 1).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 2).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 3).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 4).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 5).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 6).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 7).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 8).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 9).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 10).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 11).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 12).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 13).size(), 0);

    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 0).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 1).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 2).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 3).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 4).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 5).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 6).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 7).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 8).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 9).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 10).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 11).size(), 2);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 12).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 13).size(), 0);

    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 0).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 1).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 2).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 3).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 4).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 5).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 6).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 7).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 8).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 9).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 10).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 11).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 12).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 13).size(), 0);

    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 0).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 1).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 2).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 3).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 4).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 5).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 6).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 7).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 8).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 9).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 10).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 11).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(3, 12).size(), 1);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(4, 13).size(), 0);

    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 0), graph.FindEdge(0, 4));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 3), graph.FindEdge(3, 7));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 4), graph.FindEdge(4, 6), graph.FindEdge(4, 0));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 5), graph.FindEdge(5, 6));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 6), graph.FindEdge(6, 4), graph.FindEdge(6, 5));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 7), graph.FindEdge(7, 11), graph.FindEdge(7, 3));
    CHECK_EQUAL_RANGE(
        graph.GetBorderEdgeRange(1, 11), graph.FindEdge(11, 7), graph.FindEdge(11, 12));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(1, 12), graph.FindEdge(12, 11));

    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(2, 0), graph.FindEdge(0, 4));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(2, 3), graph.FindEdge(3, 7));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(2, 4), graph.FindEdge(4, 0));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(2, 7), graph.FindEdge(7, 11), graph.FindEdge(7, 3));
    CHECK_EQUAL_RANGE(
        graph.GetBorderEdgeRange(2, 11), graph.FindEdge(11, 7), graph.FindEdge(11, 12));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(2, 12), graph.FindEdge(12, 11));

    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 0), graph.FindEdge(0, 4));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 3), graph.FindEdge(3, 7));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 4), graph.FindEdge(4, 0));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 7), graph.FindEdge(7, 3));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 11), graph.FindEdge(11, 12));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(3, 12), graph.FindEdge(12, 11));

    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(4, 11), graph.FindEdge(11, 12));
    CHECK_EQUAL_RANGE(graph.GetBorderEdgeRange(4, 12), graph.FindEdge(12, 11));
}

BOOST_AUTO_TEST_CASE(check_last_internal_edge)
{
    //                      a--b--c--d
    std::vector<CellID> l1{{0, 0, 1, 1}};
    std::vector<CellID> l2{{0, 0, 1, 1}};
    MultiLevelPartition mlp{{l1, l2}, {2, 2}};

    std::vector<MockEdge> edges = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3}, {3, 2}};

    auto graph = makeGraph(mlp, edges);

    auto all_edges = graph.GetAdjacentEdgeRange(3);
    CHECK_EQUAL_COLLECTIONS(graph.GetBorderEdgeRange(0, 3), all_edges);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(1, 3).size(), 0);
    BOOST_CHECK_EQUAL(graph.GetBorderEdgeRange(2, 3).size(), 0);

    BOOST_CHECK_EQUAL(graph.GetInternalEdgeRange(0, 3).size(), 0);
    CHECK_EQUAL_COLLECTIONS(graph.GetInternalEdgeRange(1, 3), all_edges);
    CHECK_EQUAL_COLLECTIONS(graph.GetInternalEdgeRange(2, 3), all_edges);
}

BOOST_AUTO_TEST_SUITE_END()
