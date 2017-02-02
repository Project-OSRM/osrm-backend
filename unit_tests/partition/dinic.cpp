#include "partition/dinic_max_flow.hpp"
#include "partition/graph_generator.hpp"
#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <algorithm>
#include <vector>

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

using namespace osrm::partition;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(dinic_algorithm)

BOOST_AUTO_TEST_CASE(horizontal_cut_between_two_grids)
{
    // 40 entries of left/right edges
    const double step_size = 0.01;
    const int rows = 10;
    const int cols = 10;

    // build a small grid (10*10) and a (100 * 10) below (to make the different steps unique)
    auto graph = [&]() {
        std::vector<Coordinate> grid_coordinates;
        std::vector<EdgeWithSomeAdditionalData> grid_edges;

        const auto connect = [&grid_edges](const NodeID from, const NodeID to) {
            grid_edges.push_back({from, to, 1});
            grid_edges.push_back({to, from, 1});
        };

        // 10 rows of large components, interrupted by small disconnected components
        const auto small_coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);
        grid_coordinates.insert(
            grid_coordinates.end(), small_coordinates.begin(), small_coordinates.end());

        // connect the grid edges, starting with i * (rows * cols + 1) as first id (0,11,22...)
        const auto small_edges = makeGridEdges(rows, cols, 0);
        grid_edges.insert(grid_edges.end(), small_edges.begin(), small_edges.end());

        const auto large_coordinates =
            makeGridCoordinates(10 * rows, cols, step_size, 0, rows * step_size);
        grid_coordinates.insert(
            grid_coordinates.end(), large_coordinates.begin(), large_coordinates.end());
        const auto large_edges = makeGridEdges(10 * rows, cols, (rows * cols));
        grid_edges.insert(grid_edges.end(), large_edges.begin(), large_edges.end());

        connect(45, 1001);
        connect(55, 800);
        connect(65, 600);
        connect(75, 200);

        groupEdgesBySource(grid_edges.begin(), grid_edges.end());
        return makeBisectionGraph(grid_coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    }();

    RecursiveBisectionState bisection_state(graph);
    GraphView view(graph);

    DinicMaxFlow::SourceSinkNodes sources, sinks;

    for (int i = 0; i < 10; ++i)
    {
        sources.insert(static_cast<NodeID>(i));
        sinks.insert(static_cast<NodeID>(1000 + i));
    }

    DinicMaxFlow flow;
    const auto cut = flow(view, sources, sinks);
    BOOST_CHECK(cut.num_edges == 4);
}

BOOST_AUTO_TEST_SUITE_END()
