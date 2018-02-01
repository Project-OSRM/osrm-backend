#include "partitioner/bisection_graph_view.hpp"
#include "partitioner/graph_generator.hpp"
#include "partitioner/recursive_bisection_state.hpp"

#include <algorithm>
#include <vector>

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

using namespace osrm::partitioner;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(scc_integration)

BOOST_AUTO_TEST_CASE(graph_views_on_components)
{
    // 40 entries of left/right edges
    const double step_size = 0.01;
    const int rows = 1;
    const int cols = 10;

    const int num_components = 10;

    auto graph = [&]() {
        std::vector<Coordinate> grid_coordinates;
        std::vector<EdgeWithSomeAdditionalData> grid_edges;

        // generate 10 big components
        for (int i = 0; i < num_components; ++i)
        {
            // 10 rows of large components, interrupted by small disconnected components
            const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, i * step_size);
            grid_coordinates.insert(grid_coordinates.end(), coordinates.begin(), coordinates.end());

            // add a single disconnected node to have ids between large components
            if (i + 1 < num_components)
                grid_coordinates.push_back(
                    makeCoordinate(1, 1, 0.5 * step_size, 0, (i + 1) * step_size));

            // connect the grid edges, starting with i * (rows * cols + 1) as first id (0,11,22...)
            const auto edges = makeGridEdges(rows, cols, i * (rows * cols + 1));
            grid_edges.insert(grid_edges.end(), edges.begin(), edges.end());
        }

        groupEdgesBySource(grid_edges.begin(), grid_edges.end());
        return makeBisectionGraph(grid_coordinates, adaptToBisectionEdge(std::move(grid_edges)));
    }();

    RecursiveBisectionState bisection_state(graph);

    auto views = bisection_state.PrePartitionWithSCC(2);
    BOOST_CHECK_EQUAL(views.size(), num_components + 1); // big components + 1 small one

    for (std::size_t comp = 0; comp < num_components; ++comp)
    {
        const auto &view = views[comp];
        BOOST_CHECK(views[comp].NumberOfNodes() == 10);

        const auto to_component_id = [&](const auto &node) {
            return node.original_id / (rows * cols + 1);
        };

        std::size_t expected_component_id = to_component_id(view.Node(0));
        BOOST_CHECK(std::all_of(view.Begin(), view.End(), [&](const auto &node) {
            return to_component_id(node) == expected_component_id;
        }));

        for (const auto &node : view.Nodes())
        {
            for (const auto &edge : view.Edges(node))
            {
                BOOST_CHECK(edge.target < view.NumberOfNodes());
            }
        }
    }
    BOOST_CHECK(views.back().NumberOfNodes() == 9);
}

BOOST_AUTO_TEST_SUITE_END()
