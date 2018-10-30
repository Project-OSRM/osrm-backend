#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "partitioner/renumber.hpp"

#include "../common/range_tools.hpp"

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
        EdgeWeight weight;
        bool forward;
        bool backward;
    };
    using InputEdge = DynamicEdgeBasedGraph::InputEdge;
    std::vector<InputEdge> edges;
    std::size_t max_id = 0;
    for (const auto &m : mock_edges)
    {
        max_id = std::max<std::size_t>(max_id, std::max(m.start, m.target));

        edges.push_back(InputEdge{
            m.start, m.target, EdgeBasedGraphEdgeData{SPECIAL_NODEID, 1, 1, 1, true, false}});
        edges.push_back(InputEdge{
            m.target, m.start, EdgeBasedGraphEdgeData{SPECIAL_NODEID, 1, 1, 1, false, true}});
    }
    std::sort(edges.begin(), edges.end());
    return DynamicEdgeBasedGraph(max_id + 1, edges);
}
}

BOOST_AUTO_TEST_SUITE(renumber_tests)

BOOST_AUTO_TEST_CASE(unsplitable_case)
{
    // node:                 0  1  2  3  4  5  6  7  8  9  10 11
    // border:               x        x  x     x  x        x  x
    // permutation by cells: 0  1  2  5  6 10 11  7  8  9  3  4
    // order by cell:        0  1  2 10 11  3  4  7  8  9  5  6
    //                       x        x  x  x  x  x           x
    // border level:         3  3  3  2  2  1  1  0  0  0  0  0
    // order:                0 10 11  7  6  3  4  1  2  8  9  5
    // permutation:          0  7  8  5  6 11  4  3  9 10  1  2
    std::vector<CellID> l1{{0, 0, 1, 2, 3, 5, 5, 3, 4, 4, 1, 2}};
    std::vector<CellID> l2{{0, 0, 0, 1, 1, 3, 3, 1, 2, 2, 0, 1}};
    std::vector<CellID> l3{{0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    std::vector<MockEdge> edges = {
        // edges sorted into border/internal by level
        //  level:  (1) (2) (3) (4)
        {0, 1},  //  i   i   i   i
        {2, 10}, //  i   i   i   i
        {10, 7}, //  b   b   b   i
        {11, 0}, //  b   b   b   i
        {11, 3}, //  i   i   i   i
        {3, 4},  //  b   i   i   i
        {4, 11}, //  b   i   i   i
        {4, 7},  //  i   i   i   i
        {7, 6},  //  b   b   i   i
        {8, 9},  //  i   i   i   i
        {9, 8},  //  i   i   i   i
        {5, 6},  //  i   i   i   i
        {6, 5}   //  i   i   i   i
    };

    auto graph = makeGraph(edges);
    std::vector<Partition> partitions{l1, l2, l3, l4};

    auto permutation = makePermutation(graph, partitions);
    CHECK_EQUAL_RANGE(permutation, 0, 7, 8, 5, 6, 11, 4, 3, 9, 10, 1, 2);
}

BOOST_AUTO_TEST_SUITE_END()
