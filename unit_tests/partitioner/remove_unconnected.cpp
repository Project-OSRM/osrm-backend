#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "partitioner/remove_unconnected.hpp"

#include "util/static_graph.hpp"

#define CHECK_SIZE_RANGE(range, ref) BOOST_CHECK_EQUAL(range.end() - range.begin(), ref)
#define CHECK_EQUAL_RANGE(range, ref)                                                              \
    do                                                                                             \
    {                                                                                              \
        const auto &lhs = range;                                                                   \
        const auto &rhs = ref;                                                                     \
        BOOST_CHECK_EQUAL_COLLECTIONS(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());             \
    } while (0)

using namespace osrm;
using namespace osrm::partitioner;

namespace
{
struct MockEdge
{
    NodeID start;
    NodeID target;
};

auto makeGraph(const std::vector<MockEdge> &mock_edges)
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
        max_id = std::max<std::size_t>(max_id, std::max(m.start, m.target));
        edges.push_back(Edge{m.start, m.target, true, false});
        edges.push_back(Edge{m.target, m.start, false, true});
    }
    std::sort(edges.begin(), edges.end());
    return util::StaticGraph<EdgeData>(max_id + 1, edges);
}
}

BOOST_AUTO_TEST_SUITE(remove_unconnected_tests)

BOOST_AUTO_TEST_CASE(remove_minimum_border_eges)
{

    // node:                0  1  2  3  4  5  6  7  8  9 10 11 12
    std::vector<CellID> l1{{0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 3, 3}};
    std::vector<CellID> l2{{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2}};
    std::vector<CellID> l3{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1}};

    /*
    0---1  4---5  8---7   9
     \ /    \ /    \ /
      2      3------6
      |
      10
      | \
      11-12
    */
    std::vector<MockEdge> edges = {
        // first clique
        {0, 1},
        {1, 2},
        {2, 0},

        // second clique
        {3, 4},
        {4, 5},
        {5, 3},

        // third clique
        {6, 7},
        {7, 8},
        {8, 6},

        // fourth clique
        {10, 11},
        {11, 12},
        {12, 11},

        // connection 3 to thrid clique
        {3, 6},
        // connect 10 to first clique
        {2, 10},
    };

    // 10 is going to be unconnected on level 2 and 1
    // 3 is going to be unconnected only on level 1

    auto graph = makeGraph(edges);
    std::vector<Partition> partitions = {l1, l2, l3};
    std::vector<Partition> reference_partitions = partitions;
    // 3 get's merged into cell 1 on first level
    reference_partitions[0][3] = 1;
    reference_partitions[1][3] = 0;
    // 10 get's merged into cell 0 on both levels and not with 11 (different parent cell)
    // even though there are would be less boundary edges
    reference_partitions[0][10] = 0;
    reference_partitions[1][10] = 0;

    auto num_unconnected = removeUnconnectedBoundaryNodes(graph, partitions);
    BOOST_CHECK_EQUAL(num_unconnected, 2);

    CHECK_EQUAL_RANGE(partitions[0], reference_partitions[0]);
    CHECK_EQUAL_RANGE(partitions[1], reference_partitions[1]);
    CHECK_EQUAL_RANGE(partitions[2], reference_partitions[2]);
}

BOOST_AUTO_TEST_SUITE_END()
