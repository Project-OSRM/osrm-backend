#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "extractor/edge_based_node_segment.hpp"
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
            m.start, m.target, EdgeBasedGraphEdgeData{SPECIAL_NODEID, {1}, {1}, {1}, true, false}});
        edges.push_back(InputEdge{
            m.target, m.start, EdgeBasedGraphEdgeData{SPECIAL_NODEID, {1}, {1}, {1}, false, true}});
    }
    std::sort(edges.begin(), edges.end());
    return DynamicEdgeBasedGraph(max_id + 1, edges);
}
} // namespace

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

// The segments an open area stores per vertex name edge-based nodes the way the r-tree
// leaves do, and go through the same renumbering.  A direction that is not there is left
// alone, and the enabled bits survive.
BOOST_AUTO_TEST_CASE(renumber_open_area_vertex_segments)
{
    using extractor::EdgeBasedNodeSegment;
    std::vector<EdgeBasedNodeSegment> segments{
        EdgeBasedNodeSegment{SegmentID{0, true}, SegmentID{1, true}, 10, 11, 0, true},
        EdgeBasedNodeSegment{SegmentID{2, true}, SegmentID{3, false}, 11, 12, 1, true},
        EdgeBasedNodeSegment{SegmentID{3, true}, SegmentID{0, true}, 12, 10, 2, false}};
    // old id -> new id
    const std::vector<std::uint32_t> permutation{3, 2, 1, 0};

    renumber(segments, permutation);

    BOOST_CHECK_EQUAL(segments[0].forward_segment_id.id, 3u);
    BOOST_CHECK_EQUAL(segments[0].reverse_segment_id.id, 2u);
    BOOST_CHECK(segments[0].forward_segment_id.enabled && segments[0].reverse_segment_id.enabled);

    BOOST_CHECK_EQUAL(segments[1].forward_segment_id.id, 1u);
    BOOST_CHECK_EQUAL(segments[1].reverse_segment_id.id, 3u); // disabled, so untouched
    BOOST_CHECK(!segments[1].reverse_segment_id.enabled);

    BOOST_CHECK_EQUAL(segments[2].forward_segment_id.id, 0u);
    BOOST_CHECK_EQUAL(segments[2].reverse_segment_id.id, 3u);
    // what is not an id is not touched
    BOOST_CHECK_EQUAL(segments[2].u, 12u);
    BOOST_CHECK_EQUAL(segments[2].v, 10u);
    BOOST_CHECK_EQUAL(segments[2].fwd_segment_position, 2u);
    BOOST_CHECK(!segments[2].is_startpoint);
}

BOOST_AUTO_TEST_SUITE_END()
