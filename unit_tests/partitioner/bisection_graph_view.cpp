#include "partitioner/bisection_graph_view.hpp"
#include "partitioner/graph_generator.hpp"
#include "partitioner/recursive_bisection_state.hpp"

#include "util/debug.hpp"

#include <algorithm>
#include <climits>
#include <random>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace osrm::partitioner;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(graph_view)

namespace
{
void shuffle(std::vector<EdgeWithSomeAdditionalData> &grid_edges)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(grid_edges.begin(), grid_edges.end(), rng);
}
} // namespace

BOOST_AUTO_TEST_CASE(separate_top_bottom)
{
    // 40 entries of left/right edges
    double step_size = 0.01;
    int rows = 2;
    int cols = 4;
    const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);

    auto grid_edges = makeGridEdges(rows, cols, 0);

    shuffle(grid_edges);
    groupEdgesBySource(grid_edges.begin(), grid_edges.end());

    auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    RecursiveBisectionState bisection_state(graph);
    std::vector<bool> partition(8, false);
    partition[4] = partition[5] = partition[6] = partition[7] = true;

    const auto center = bisection_state.ApplyBisection(graph.Begin(), graph.End(), 0, partition);
    BisectionGraphView left(graph, graph.Begin(), center);
    BisectionGraphView right(graph, center, graph.End());

    BOOST_CHECK_EQUAL(left.NumberOfNodes(), 4);
    for (const auto &node : left.Nodes())
    {
        auto id = left.GetID(node);
        const auto compare = makeCoordinate(id, 0, step_size);
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < left.NumberOfNodes());
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id), 0);

        for (const auto &edge : left.Edges(node))
            BOOST_CHECK(edge.target < left.NumberOfNodes());
    }

    BOOST_CHECK_EQUAL(right.NumberOfNodes(), 4);
    for (const auto &node : right.Nodes())
    {
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id),
                          1 << (sizeof(BisectionID) * CHAR_BIT - 1));
        auto id = right.GetID(node);
        const auto compare = makeCoordinate(id, 1, step_size);
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < right.NumberOfNodes());
        for (const auto &edge : right.Edges(node))
            BOOST_CHECK(edge.target < right.NumberOfNodes());
    }
}

BOOST_AUTO_TEST_CASE(separate_top_bottom_copy)
{
    // 40 entries of left/right edges
    double step_size = 0.01;
    int rows = 2;
    int cols = 4;
    const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);

    auto grid_edges = makeGridEdges(rows, cols, 0);

    shuffle(grid_edges);
    groupEdgesBySource(grid_edges.begin(), grid_edges.end());

    auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    RecursiveBisectionState bisection_state(graph);
    std::vector<bool> partition(8, false);
    partition[4] = partition[5] = partition[6] = partition[7] = true;

    const auto center = bisection_state.ApplyBisection(graph.Begin(), graph.End(), 0, partition);
    BisectionGraphView total(graph, graph.Begin(), graph.End());

    BisectionGraphView left(total, total.Begin(), center);
    BisectionGraphView right(total, center, total.End());

    BOOST_CHECK_EQUAL(left.NumberOfNodes(), 4);
    for (const auto &node : left.Nodes())
    {
        auto id = left.GetID(node);
        const auto compare = makeCoordinate(id, 0, step_size);
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < left.NumberOfNodes());
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id), 0);

        for (const auto &edge : left.Edges(id))
            BOOST_CHECK(edge.target < left.NumberOfNodes());
    }

    BOOST_CHECK_EQUAL(right.NumberOfNodes(), 4);
    for (NodeID id = 0; id < right.NumberOfNodes(); ++id)
    {
        const auto &node = right.Node(id);
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id),
                          1 << (sizeof(BisectionID) * CHAR_BIT - 1));
        const auto compare = makeCoordinate(id, 1, step_size);
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < right.NumberOfNodes());
        for (const auto &edge : right.Edges(id))
            BOOST_CHECK(edge.target < right.NumberOfNodes());
    }
}

BOOST_AUTO_TEST_CASE(separate_left_right)
{
    // 40 entries of left/right edges
    double step_size = 0.01;
    int rows = 2;
    int cols = 4;
    const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);

    auto grid_edges = makeGridEdges(rows, cols, 0);

    shuffle(grid_edges);
    groupEdgesBySource(grid_edges.begin(), grid_edges.end());

    auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    // separate Left 2 nodes from rest
    RecursiveBisectionState bisection_state(graph);
    std::vector<bool> partition(8, true);
    partition[0] = partition[4] = false;

    const auto center = bisection_state.ApplyBisection(graph.Begin(), graph.End(), 0, partition);
    BisectionGraphView left(graph, graph.Begin(), center);
    BisectionGraphView right(graph, center, graph.End());

    BOOST_CHECK_EQUAL(left.NumberOfNodes(), 2);
    std::vector<Coordinate> left_coordinates;
    left_coordinates.push_back(makeCoordinate(0, 0, step_size));
    left_coordinates.push_back(makeCoordinate(0, 1, step_size));
    auto left_compare = left_coordinates.begin();
    for (const auto &node : left.Nodes())
    {
        auto id = left.GetID(node);
        const auto compare = *left_compare++;
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < left.NumberOfNodes());
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id), 0);

        for (const auto &edge : left.Edges(node))
            BOOST_CHECK(edge.target < left.NumberOfNodes());
    }

    BOOST_CHECK_EQUAL(right.NumberOfNodes(), 6);
    std::vector<Coordinate> right_coordinates;
    right_coordinates.push_back(makeCoordinate(1, 0, step_size));
    right_coordinates.push_back(makeCoordinate(2, 0, step_size));
    right_coordinates.push_back(makeCoordinate(3, 0, step_size));
    right_coordinates.push_back(makeCoordinate(1, 1, step_size));
    right_coordinates.push_back(makeCoordinate(2, 1, step_size));
    right_coordinates.push_back(makeCoordinate(3, 1, step_size));
    auto right_compare = right_coordinates.begin();
    for (const auto &node : right.Nodes())
    {
        BOOST_CHECK_EQUAL(bisection_state.GetBisectionID(node.original_id),
                          1 << (sizeof(BisectionID) * CHAR_BIT - 1));
        auto id = right.GetID(node);
        const auto compare = *right_compare++;
        BOOST_CHECK_EQUAL(compare, node.coordinate);
        BOOST_CHECK(id < right.NumberOfNodes());
        for (const auto &edge : right.Edges(node))
            BOOST_CHECK(edge.target < right.NumberOfNodes());
    }
}

BOOST_AUTO_TEST_SUITE_END()
