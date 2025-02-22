#include "partitioner/recursive_bisection.hpp"
#include "partitioner/graph_generator.hpp"
#include "partitioner/recursive_bisection_state.hpp"

#include <algorithm>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace osrm::partitioner;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(recursive_bisection)

BOOST_AUTO_TEST_CASE(dividing_four_grid_cells)
{
    // 40 entries of left/right edges
    const double step_size = 0.01;
    const int rows = 10;
    const int cols = 10;
    const int cut_edges = 4;

    auto graph = [&]()
    {
        std::vector<Coordinate> grid_coordinates;
        std::vector<EdgeWithSomeAdditionalData> grid_edges;

        const auto connect = [&grid_edges](int min_left, int max_left, int min_right, int max_right)
        {
            const NodeID source = (rand() % (max_left - min_left)) + min_left;
            const NodeID target = (rand() % (max_right - min_right)) + min_right;

            grid_edges.push_back({source, target, 1});
            grid_edges.push_back({target, source, 1});
        };

        // generate 10 big components
        for (int i = 0; i < 4; ++i)
        {
            // 10 rows of large components, interrupted by small disconnected components
            const auto coordinates = makeGridCoordinates(
                rows, cols, step_size, cols * (i % 2), (i * rows / 2) * step_size);
            grid_coordinates.insert(grid_coordinates.end(), coordinates.begin(), coordinates.end());

            // connect the grid edges, starting with i * (rows * cols + 1) as first id (0,11,22...)
            const auto edges = makeGridEdges(rows, cols, i * (rows * cols));
            grid_edges.insert(grid_edges.end(), edges.begin(), edges.end());
        }

        // add cut edges between neighboring cells
        int n = rows * cols;
        for (int i = 0; i < cut_edges; ++i)
        {
            // left/right
            connect(0, n, n, 2 * n);
            connect(2 * n, 3 * n, 3 * n, 4 * n);
            // top/bottom
            connect(0, n, 2 * n, 3 * n);
            connect(n, 2 * n, 3 * n, 4 * n);
        }
        groupEdgesBySource(grid_edges.begin(), grid_edges.end());
        return makeBisectionGraph(grid_coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    }();

    RecursiveBisection bisection(graph, 120, 1.1, 0.25, 10, 1);

    const auto &result = bisection.BisectionIDs();
    // all same IDs withing a group
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < rows * cols; ++j)
            BOOST_CHECK(result[i * (rows * cols)] == result[i * (rows * cols) + j]);

    // different IDs for all four groups
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            BOOST_CHECK(i == j || result[i * (rows * cols)] != result[j * (rows * cols)]);
}

BOOST_AUTO_TEST_SUITE_END()
